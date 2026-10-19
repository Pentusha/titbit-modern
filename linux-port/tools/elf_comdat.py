"""Select source-owned largest COMDATs independently in adapted ELF32 objects.

ELF only implements first-provider COMDAT selection. Reordering archive members
cannot implement 'largest' when two TUs own the largest versions of different
tables. This pass excludes losing groups and makes their table definitions
undefined, forcing references to extract the selected provider from the archive.
"""
from pathlib import Path
import struct

SHT_SYMTAB=2
SHT_GROUP=17
SHT_REL=9
SHF_ALLOC=2
SHF_GROUP=0x200
SHF_EXCLUDE=0x80000000


def encode(raw):
    return 'titbit_ms_'+raw.encode().hex() if any(c in raw for c in '@?\\') else raw


class Elf:
    def __init__(self,path):
        self.path=Path(path)
        self.data=bytearray(self.path.read_bytes())
        if self.data[:6]!=b'\x7fELF\x01\x01':raise ValueError('Expected little-endian ELF32: '+str(path))
        self.sh_off=struct.unpack_from('<I',self.data,32)[0]
        self.sh_size,self.sh_num=struct.unpack_from('<HH',self.data,46)
        self.sections=[list(struct.unpack_from('<10I',self.data,self.sh_off+i*self.sh_size)) for i in range(self.sh_num)]
        self.symbols={}
        for index,sec in enumerate(self.sections):
            if sec[1]!=SHT_SYMTAB:continue
            strings=self.payload(sec[6])
            self.symbols[index]=[]
            for at in range(sec[4],sec[4]+sec[5],sec[9]):
                name,value,size,info,other,owner=struct.unpack_from('<IIIBBH',self.data,at)
                text=strings[name:].split(b'\0',1)[0].decode()
                self.symbols[index].append(dict(name=text,value=value,size=size,info=info,owner=owner,at=at))
        self.groups={}
        for index,sec in enumerate(self.sections):
            if sec[1]!=SHT_GROUP:continue
            words=struct.unpack('<'+'I'*(sec[5]//4),self.payload(index))
            if not words or not words[0]&1:continue
            signature=self.symbols[sec[6]][sec[7]]
            members=list(words[1:])
            self.groups[signature['name']]=dict(index=index,symtab=sec[6],signature=sec[7],
                                                members=members,size=sum(self.sections[m][5] for m in members
                                                                       if self.sections[m][2]&SHF_ALLOC))
    def payload(self,index):
        section=self.sections[index]
        return bytes(self.data[section[4]:section[4]+section[5]])
    def exclude(self,group_name):
        group=self.groups[group_name]
        members=set(group['members'])
        symbols=self.symbols[group['symtab']]
        signature=symbols[group['signature']]
        if signature['owner'] not in members:
            raise ValueError('COMDAT signature is not owned by its group: '+group_name)
        other_exports=[s['name'] for s in symbols if s['owner'] in members and s['info']>>4 and
                       s['name'] and s['name']!=group_name]
        if other_exports:
            raise ValueError('Largest COMDAT has additional exported aliases: '+repr(other_exports))
        # Outside references through a local section symbol must be redirected
        # to the selected table too, retaining the original byte displacement.
        for index,sec in enumerate(self.sections):
            if sec[1]!=SHT_REL or index in members or sec[7] in members:continue
            if sec[6]!=group['symtab']:continue
            target_section=self.sections[sec[7]]
            for at in range(sec[4],sec[4]+sec[5],8):
                offset,info=struct.unpack_from('<II',self.data,at)
                symbol=symbols[info>>8]
                if symbol['owner'] not in members or info>>8==group['signature']:continue
                relocation=info&0xff
                if relocation not in (1,2):raise ValueError('Unsupported local COMDAT relocation '+str(relocation))
                location=target_section[4]+offset
                addend=struct.unpack_from('<I',self.data,location)[0]
                addend=(addend+symbol['value']-signature['value'])&0xffffffff
                struct.pack_into('<I',self.data,location,addend)
                struct.pack_into('<I',self.data,at+4,(group['signature']<<8)|relocation)
        for symbol in symbols:
            if symbol['owner'] not in members:continue
            at=symbol['at']
            struct.pack_into('<II',self.data,at+4,0,0)
            if symbol['name']==group_name:
                # Strong UND pulls the winner from the archive even when the
                # losing provider originally emitted a weak table alias.
                self.data[at+12]=(1<<4)|(symbol['info']&15)
                struct.pack_into('<H',self.data,at+14,0)
            else:
                # Unreferenced local section/data labels become absolute zero.
                # Any outside relocations were redirected to the signature.
                struct.pack_into('<H',self.data,at+14,0xfff1)
        for member in members:
            section=self.sections[member]
            section[2]=(section[2]&~SHF_GROUP)|SHF_EXCLUDE
        section=self.sections[group['index']]
        section[1]=1 # excluded ordinary bytes, no longer a COMDAT signature
        section[2]=SHF_EXCLUDE
        section[6]=section[7]=section[9]=0
    def save(self):
        for index,section in enumerate(self.sections):
            struct.pack_into('<10I',self.data,self.sh_off+index*self.sh_size,*section)
        temporary=self.path.with_suffix(self.path.suffix+'.comdat.tmp')
        temporary.write_bytes(self.data)
        temporary.replace(self.path)


def select_comdats(rows):
    """Mutate only row['object'] adapted copies; return deterministic evidence."""
    selections={encode(s['symbol']) for row in rows for s in row.get('comdatSelections',[])
                if s['selection']=='largest'}
    providers={}
    objects=[]
    for index,row in enumerate(rows):
        elf=Elf(row['object']);objects.append(elf)
        for name,group in elf.groups.items():
            if name in selections:
                providers.setdefault(name,[]).append((index,row,group))
    proofs=[]
    changed=set()
    for name,choices in sorted(providers.items()):
        if len(choices)<2:continue
        winner=min(choices,key=lambda item:(-item[2]['size'],item[1].get('priority',0),item[0]))
        losers=[]
        for index,row,group in choices:
            if index==winner[0]:continue
            objects[index].exclude(name);changed.add(index)
            losers.append({'source':row['source'],'bytes':group['size']})
        proofs.append({'symbol':name,'winner':winner[1]['source'],'bytes':winner[2]['size'],'excluded':losers})
    for index in sorted(changed):objects[index].save()
    return proofs
