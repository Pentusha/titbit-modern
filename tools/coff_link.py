"""Small auditable x86 COFF reader and relocation writer for reconstruction.
No executable bytes are supplied by the original target. Symbols and section
addresses must have separate evidence in the reconstruction's binding map.
"""
import struct,pathlib,re,hashlib
from coff_tools import without_trailing_alignment

def artifact_symbol_name(name):
 """Keep C++ decorated symbols usable and distinct on Windows filesystems."""
 if re.search(r'[<>:"/\\|?*\x00-\x1f]',name) or len(name)>128:
  return re.sub(r'[^A-Za-z0-9_.$-]','_',name)[:120]+'-'+hashlib.sha256(name.encode()).hexdigest()[:12]
 return name.replace('@','_')

class CoffObject:
 def __init__(self,path,data=None):
  self.path=pathlib.Path(path);self.data=self.path.read_bytes() if data is None else bytes(data);b=self.data
  machine,nsec,_,symoff,nsym,opt,_=struct.unpack_from('<HHLLLHH',b)
  if machine!=0x14c or opt:raise ValueError('Expected ordinary x86 COFF object')
  string_start=symoff+18*nsym;string_length=struct.unpack_from('<I',b,string_start)[0]
  strings=b[string_start:string_start+string_length]
  def string(at):return strings[at:strings.index(0,at)].decode('ascii')
  self.symbols={};i=0
  while i<nsym:
   raw,value,sec,typ,storage,aux=struct.unpack_from('<8sLhHBB',b,symoff+i*18)
   name=string(struct.unpack_from('<I',raw,4)[0]) if raw[:4]==b'\0'*4 else raw.split(b'\0')[0].decode('ascii')
   self.symbols[i]={'index':i,'name':name,'value':value,'section':sec,'type':typ,'storage':storage,'aux':aux};i+=1+aux
   if storage==105:
    if aux!=1 or sec!=0:raise ValueError('Malformed weak external symbol')
    fallback,search=struct.unpack_from('<LL',b,symoff+(i-aux)*18)
    self.symbols[i-1-aux].update(weakFallbackIndex=fallback,weakSearch=search)
  self.sections={}
  for i in range(nsec):
   raw,_,_,size,offset,reloc_offset,_,nreloc,_,flags=struct.unpack_from('<8sLLLLLLHHL',b,20+i*40)
   name=raw.split(b'\0')[0].decode('ascii');name=string(int(name[1:])) if name.startswith('/') else name
   relocs=[]
   for j in range(nreloc):
    at,symbol,kind=struct.unpack_from('<LLH',b,reloc_offset+j*10)
    if at+4>size:raise ValueError('Relocation outside section')
    relocs.append({'offset':at,'symbol':self.symbols[symbol],'kind':kind})
   self.sections[i+1]={'index':i+1,'name':name,'size':size,'bytes':b[offset:offset+size] if offset else bytes(size),'relocations':relocs,'flags':flags}
  self.by_name={}
  for s in self.symbols.values():
   old=self.by_name.get(s['name'])
   # VC6 can append an undefined declaration after a real section definition
   # with the same external name. The declaration must not hide the function.
   if old is None or not(old['section']>0 and s['section']<=0):self.by_name[s['name']]=s
  for s in self.symbols.values():
   if 'weakFallbackIndex' in s:
    if s['weakFallbackIndex'] not in self.symbols:raise ValueError('Invalid weak external fallback')
    s['weakFallback']=self.symbols[s['weakFallbackIndex']]['name']

 def function(self,name):
  symbol=self.by_name[name]
  if symbol['section']<=0 or not symbol['type']&0x20:raise ValueError('Not a defined function: '+name)
  section=self.sections[symbol['section']]
  if symbol['value']!=0:raise ValueError('Function is not isolated at COMDAT offset zero')
  others=[s for s in self.symbols.values() if s['section']==symbol['section'] and s['type']&0x20 and s['name']!=name]
  if others:raise ValueError('COMDAT has multiple functions')
  return section

 def linked_function(self,name,address,bindings,section_bases=None):
  return self.linked_section(self.function(name)['index'],address,bindings,section_bases)

 def linked_section(self,index,address,bindings,section_bases=None):
  section=self.sections[index];code=bytearray(section['bytes']);evidence=[];section_bases=section_bases or {}
  def resolve(s,seen=()):
   if s['name'] in seen:raise ValueError('Cyclic weak external fallback')
   if s['section']==section['index']:return address+s['value']
   if s['name'] in bindings:return bindings[s['name']]
   if s['section'] in section_bases:return section_bases[s['section']]+s['value']
   if 'weakFallback' in s and s['weakSearch'] in (1,2,3):return resolve(self.by_name[s['weakFallback']],(*seen,s['name']))
   raise KeyError('Unbound COFF symbol '+s['name'])
  for reloc in section['relocations']:
   s=reloc['symbol'];at=reloc['offset'];addend=struct.unpack_from('<I',code,at)[0]
   target=resolve(s)
   if reloc['kind']==0x6:value=target+addend
   elif reloc['kind']==0x14:value=target+addend-(address+at+4)
   else:raise ValueError('Unsupported x86 relocation '+hex(reloc['kind']))
   struct.pack_into('<I',code,at,value&0xffffffff)
   evidence.append({'offset':at,'symbol':s['name'],'kind':hex(reloc['kind']),'symbolVa':hex(target),'addend':hex(addend),'written':hex(value&0xffffffff)})
  return bytes(code),evidence

 def code_without_alignment(self,name):
  section=self.function(name)
  return without_trailing_alignment(section['bytes'])

def listing_code_extents(path):
 """Read code ends from compiler /FAcs listings, excluding trailing npad/DD.
 The target binary and its function lengths are never inputs to this parser.
 Embedded switch tables remain in the full object for relocation and linking.
 """
 result={};name=None;records=[];current=None
 def flush():
  nonlocal current
  if current is not None:records.append(current);current=None
 for line in pathlib.Path(path).read_text(encoding='utf-8').splitlines():
  start=re.match(r'^(\S+)\s+PROC\s+NEAR',line)
  if start:name=start[1];records=[];current=None;continue
  if name and re.match(re.escape(name)+r'\s+ENDP',line):
   flush();instructions=[r for r in records if r['statement'] and r['statement'].split()[0].lower() not in ('dd','db','dw','dq','dt','npad','align')]
   if instructions:result[name]=max(r['offset']+r['size'] for r in instructions)
   name=None;continue
  if not name:continue
  fields=line.split('\t')
  if len(fields)<2:continue
  first,raw=fields[0].strip(),fields[1].strip()
  if not re.fullmatch(r'[0-9a-fA-F]{2}(?: +[0-9a-fA-F]{2})*',raw):continue
  statement=' '.join(fields[2:]).strip();size=len(raw.split())
  if re.fullmatch(r'[0-9a-fA-F]{5,8}',first):
   flush();current={'offset':int(first,16),'size':size,'statement':statement}
  elif not first and current is not None:
   current['size']+=size;current['statement']+=' '+statement
 return result

def listing_string_extents(path):
 """Count literal DB bytes emitted by the compiler, excluding following ORG
 alignment. Embedded NUL bytes are retained; no target length is consulted.
 """
 def count(body):
  i=0;n=0
  while i<len(body):
   if body[i].isspace() or body[i]==',':i+=1;continue
   if body[i]==';':break
   if body[i] in ('\'', '"'):
    quote=body[i];i+=1
    while i<len(body):
     if body[i]==quote:
      if i+1<len(body) and body[i+1]==quote:n+=1;i+=2;continue
      i+=1;break
     n+=1;i+=1
    else:raise ValueError('Unterminated assembler literal')
   else:
    repeat=re.match(r'([0-9][0-9a-fA-F]*[Hh]|[0-9]+)\s+DUP\s*\(\s*(?:\?|[0-9][0-9a-fA-F]*[Hh]|[0-9]+)\s*\)',body[i:],re.I)
    if repeat:
     value=repeat[1];n+=int(value[:-1],16) if value.lower().endswith('h') else int(value);i+=len(repeat[0]);continue
    number=re.match(r'(?:[0-9][0-9a-fA-F]*[Hh]|[0-9]+)',body[i:])
    if not number:raise ValueError('Unsupported DB expression: '+body[i:])
    i+=len(number[0]);n+=1
  return n
 result={};active=None
 for line in pathlib.Path(path).read_text(encoding='utf-8').splitlines():
  start=re.match(r'^(\$SG\d+|\?\?_C@\S+)\s+DB\s+(.*)$',line)
  continuation=re.match(r'^\s+DB\s+(.*)$',line)
  if start:active=start[1];result[active]=count(start[2])
  elif continuation and active:result[active]+=count(continuation[1])
  elif line.strip() and not line.lstrip().startswith(';'):active=None
 return result

def listing_data_extents(path):
 """Size compiler DB/DW/DD/DQ/DT declarations, excluding following ORG.
 Named C variables may share a section without sharing their final placement.
 These extents come only from the compiler listing, never target-file lengths.
 """
 def terms(text):
  parts=[];start=0;depth=0;quote=None;i=0
  while i<len(text):
   c=text[i]
   if quote:
    if c==quote:
     if i+1<len(text) and text[i+1]==quote:i+=2;continue
     quote=None
   elif c in ('\'', '"'):quote=c
   elif c=='(':depth+=1
   elif c==')':depth-=1
   elif c==';':text=text[:i];break
   elif c==',' and depth==0:parts.append(text[start:i].strip());start=i+1
   i+=1
  if quote or depth:raise ValueError('Unbalanced compiler data declaration')
  if text[start:].strip():parts.append(text[start:].strip())
  return parts
 def size(body,width):
  total=0
  for item in terms(body):
   repeat=re.fullmatch(r'([0-9][0-9a-fA-F]*[Hh]|[0-9]+)\s+DUP\s*\((.*)\)',item,re.I)
   if repeat:
    n=repeat[1];n=int(n[:-1],16) if n.lower().endswith('h') else int(n);total+=n*size(repeat[2],width)
   elif width==1 and item[:1] in ('\'', '"'):
    if item[-1:]!=item[:1]:raise ValueError('Unterminated compiler DB literal')
    total+=len(item[1:-1].replace(item[0]*2,item[0]))
   else:total+=width
  return total
 result={};active=None;scale={'DB':1,'DW':2,'DD':4,'DQ':8,'DT':10}
 for line in pathlib.Path(path).read_text(encoding='utf-8').splitlines():
  start=re.match(r'^(\S+)\s+(DB|DW|DD|DQ|DT)\s+(.*)$',line,re.I)
  continuation=re.match(r'^\s+(DB|DW|DD|DQ|DT)\s+(.*)$',line,re.I)
  if start:active=start[1];result[active]=size(start[3],scale[start[2].upper()])
  elif continuation and active:result[active]+=size(continuation[2],scale[continuation[1].upper()])
  elif line.strip() and not line.lstrip().startswith(';'):active=None
 return result
