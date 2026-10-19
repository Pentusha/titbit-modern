"""Inventory historical SDK archive members; never classify them reconstructed.
These are vendor binary reference objects, not source-compiled game artifacts.
"""
import collections,hashlib,json,pathlib,struct
base=pathlib.Path(__file__).resolve().parents[1];root=base.parent
def members(data):
 if data[:8]!=b'!<arch>\n':raise ValueError('Not an ar archive')
 at=8;longnames=b''
 while at<len(data):
  header=data[at:at+60]
  if len(header)!=60 or header[58:60]!=b'`\n':raise ValueError('Malformed archive header')
  name=header[:16].decode('ascii').strip();size=int(header[48:58]);payload=data[at+60:at+60+size]
  if len(payload)!=size:raise ValueError('Truncated member')
  if name=='//':longnames=payload
  elif name.startswith('/') and name[1:].isdigit():
   pos=int(name[1:]);end=longnames.find(b'\0',pos)
   if end<0:end=longnames.find(b'/\n',pos)
   if end<0:raise ValueError('Unterminated long member name')
   name=longnames[pos:end].decode('ascii');yield name,payload
  elif name!='/':yield name.rstrip('/'),payload
  at+=60+size+(size&1)
def main():
 reports=[]
 for filename in ('LIBC.LIB','LIBCMT.LIB','MSVCRT.LIB'):
  path=base/'toolchain/msvc6-sdk-initial'/filename;raw=path.read_bytes();rows=[]
  for name,payload in members(raw):
   if len(payload)<20 or payload[:2]!=b'\x4c\x01':continue
   _,nsec,_,symoff,nsym,opt,_=struct.unpack_from('<HHLLLHH',payload)
   if opt or not symoff or symoff+nsym*18+4>len(payload):continue
   string_start=symoff+nsym*18;strings=payload[string_start:];symbols=[];identity=None;i=0
   while i<nsym:
    rawname,value,section,typ,storage,aux=struct.unpack_from('<8sLhHBB',payload,symoff+i*18)
    if rawname[:4]==b'\0'*4:
     pos=struct.unpack_from('<I',rawname,4)[0];symbol=strings[pos:strings.index(0,pos)].decode('ascii')
    else:symbol=rawname.split(b'\0')[0].decode('ascii')
    if symbol=='@comp.id':identity={'product':hex(value>>16),'build':value&65535}
    if section>0 and storage==2:symbols.append({'name':symbol,'section':section,'offset':value,'functionType':bool(typ&0x20)})
    i+=1+aux
   rows.append({'name':name,'bytes':len(payload),'sha256':hashlib.sha256(payload).hexdigest(),'sections':nsec,'compilerIdentity':identity,'symbols':symbols})
  report={'path':path.relative_to(root).as_posix(),'sha256':hashlib.sha256(raw).hexdigest(),'classification':'vendor SDK binary reference; not reconstructed source or dashboard Exact','members':rows};reports.append(report)
  counts=collections.Counter((r['compilerIdentity']['product'],r['compilerIdentity']['build']) for r in rows if r['compilerIdentity']);hits={s['name']:r['name'] for r in rows for s in r['symbols'] if s['name'] in ('_malloc','_free','_fprintf','__setjmp3','_WinMainCRTStartup')}
  print(json.dumps({'library':filename,'coffMembers':len(rows),'compilerIdentities':[{'product':k[0],'build':k[1],'count':v} for k,v in counts.items()],'selectedSymbols':hits},indent=2))
 (base/'analysis/crt-sdk-archive-inventory.json').write_text(json.dumps(reports,indent=2)+'\n',encoding='utf-8')

if __name__=="__main__":main()
