"""Read a single isolated x86 function COMDAT; reject unresolved relocations."""
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_GRP_JUMP,CS_GRP_RET
from capstone.x86 import X86_OP_IMM

def without_trailing_alignment(code):
 """Find the end from the compiled CFG, never from the original target length.
 Only unreachable trailing single-byte NOP/INT3 alignment may be discarded.
 """
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
 decoded={i.address:i for i in md.disasm(code,0)};pending=[0];visited=set();end=0
 while pending:
  address=pending.pop()
  if address in visited:continue
  if address not in decoded:raise ValueError('Compiled control flow exits isolated function')
  ins=decoded[address];visited.add(address);end=max(end,address+ins.size)
  if ins.group(CS_GRP_RET):continue
  if ins.group(CS_GRP_JUMP):
   if not ins.operands or ins.operands[0].type!=X86_OP_IMM:raise ValueError('Unresolved indirect branch')
   pending.append(ins.operands[0].imm)
   if ins.mnemonic=='jmp':continue
  pending.append(address+ins.size)
 if any(b not in (0x90,0xcc) for b in code[end:]):raise ValueError('Unreachable non-alignment bytes after compiled function')
 return code[:end]

def coff_function(path,name,trim_alignment=False):
 b=path.read_bytes();machine,nsec,_,symoff,nsym,opt,_=struct.unpack_from('<HHLLLHH',b)
 if machine!=0x14c:raise ValueError('Expected x86 COFF')
 sections=[struct.unpack_from('<8sLLLLLLHHL',b,20+opt+40*i) for i in range(nsec)]
 strings=b[symoff+nsym*18:];i=0
 while i<nsym:
  raw,value,sec,typ,storage,aux=struct.unpack_from('<8sLhHBB',b,symoff+i*18)
  symbol=(strings[struct.unpack_from('<L',raw,4)[0]:].split(b'\0')[0] if raw[:4]==b'\0'*4 else raw.split(b'\0')[0]).decode('ascii','replace')
  if name in symbol and sec>0 and typ&0x20:
   sh=sections[sec-1]
   if sh[7]:raise ValueError('Function has unresolved COFF relocations: '+name)
   if value:raise ValueError('Expected isolated function COMDAT at offset zero: '+name)
   code=b[sh[4]:sh[4]+sh[3]]
   return without_trailing_alignment(code) if trim_alignment else code,symbol
  i+=1+aux
 raise ValueError('Function symbol not found: '+name)
