"""Build the public sources with an installed VC98 CRT/STL, without an original PE."""
import argparse
import collections
import hashlib
import json
import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys

from coff_link import CoffObject
from inspect_crt_archive import members
from source_data import append_label, expose_field
from source_leaf import relocate
from source_initializers import bind_static_callbacks, exclude_automatic_tables
from source_comdat import equivalent_comdat_providers
import match

BASE = pathlib.Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def vc98():
    value = os.environ.get('TITBIT_VC6')
    if not value:
        raise ValueError('Set TITBIT_VC6 to your installed VC98 directory (Bin, Include, Lib, ATL).')
    root = pathlib.Path(value).resolve()
    required = ['Bin/CL.EXE','Bin/LINK.EXE','Bin/LIB.EXE','Bin/CVTRES.EXE','Bin/ML.EXE',
                'Include/stdio.h','ATL/Include/atlbase.h','Lib/LIBC.LIB','Lib/LIBCP.LIB']
    missing = [p for p in required if not (root/p).is_file()]
    if missing:
        raise ValueError('Incomplete TITBIT_VC6: '+', '.join(missing))
    return root


def aliases_object(path, aliases):
    targets = sorted(set(aliases.values()))
    indices = {name:i for i,name in enumerate(targets)}
    strings, symbols = bytearray(b'\0'*4), bytearray()
    def symbol(name, storage, aux):
        raw=name.encode('ascii')
        if len(raw)>8:
            field=struct.pack('<II',0,len(strings)); strings.extend(raw+b'\0')
        else:
            field=raw.ljust(8,b'\0')
        return struct.pack('<8sIhHBB',field,0,0,0,storage,aux)
    for name in targets: symbols.extend(symbol(name,2,0))
    for name,target in sorted(aliases.items()):
        symbols.extend(symbol(name,105,1)); symbols.extend(struct.pack('<II10x',indices[target],3))
    struct.pack_into('<I',strings,0,len(strings))
    path.write_bytes(struct.pack('<HHIIIHH',0x14c,0,0,20,len(symbols)//18,0,0)+symbols+strings)


def library_symbols(path):
    result=set()
    for _,data in members(path.read_bytes()):
        if data[:4]==b'\0\0\xff\xff':
            raw=data[20:].split(b'\0')[0].decode('ascii')
            result.update((raw,'__imp_'+raw)); continue
        try:
            obj=CoffObject(path,data=data)
        except (ValueError,KeyError,struct.error,IndexError):
            continue
        result.update(s['name'] for s in obj.symbols.values()
                      if s['storage']==2 and (s['section']>0 or s['value']>0))
    return result


def main(default_compat=False):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compat',action='store_true',default=default_compat)
    parser.add_argument('--original',action='store_true')
    parser.add_argument('--out',type=pathlib.Path)
    parser.add_argument('--check',action='store_true',help='validate compiler and all public source inputs only')
    parser.add_argument('--fresh',action='store_true',help='compile every source instead of reusing hash-checked compiler objects')
    args=parser.parse_args()
    compat=args.compat and not args.original
    sdk=vc98()
    config=read(BASE/'config/source-build.json')
    out=(args.out or BASE/'build'/('modern' if compat else 'original')).resolve()
    out.mkdir(parents=True,exist_ok=True)
    objdir=out/'obj';objdir.mkdir(exist_ok=True)
    env=os.environ.copy()
    env['PATH']=str(sdk/'Bin')+os.pathsep+env.get('PATH','')
    env['INCLUDE']=str(sdk/'Include')+';'+str(sdk/'ATL/Include')
    env['LIB']=str(sdk/'Lib')
    def run(command,log,cwd=BASE):
        result=subprocess.run(command,cwd=cwd,env=env,capture_output=True)
        log.write_bytes(result.stdout+result.stderr)
        if result.returncode:
            print(log.read_text(encoding='cp1251',errors='replace')[-12000:],flush=True)
            raise RuntimeError('Command failed; see '+str(log))
    def response(tool,name,arguments):
        rsp=out/(name+'.rsp')
        def short(arg):
            for root in (BASE,out,sdk): arg=str(arg).replace(str(root),match.short_path(root))
            return arg
        rsp.write_text('\n'.join('"'+short(a)+'"' for a in arguments)+'\n',encoding='ascii')
        run([str(tool),'@'+match.short_path(rsp)],out/(name+'.log'))
    units=list(config['recipes'])
    # Authored, compiler-proved storage of the game's shared template guard.
    # A weak alias to VC6 COMMON would otherwise become image-base + sizeof(byte).
    units.append({'source':'src/lanes/codex/data/template_guards.asm','flags':['/coff'],
                  'includes':[],'symbolAddresses':{},'priority':0})
    if compat:
        units += [{'source':p.relative_to(BASE).as_posix(),'flags':['/O2','/G5','/Gy','/Zl','/GR-'],
                   'includes':[],'symbolAddresses':{},'priority':0}
                  for p in sorted((BASE/'src/compat').glob('compat_*.cpp'))]+[
                    {'source':'src/compat/source_entry.cpp','flags':['/O2','/G5','/Gy','/Zl','/GR-'],
                     'includes':[],'symbolAddresses':{},'priority':0}]
    libs=[sdk/'Lib'/name for name in config['importLibraries']]+[sdk/'Lib/LIBC.LIB',sdk/'Lib/LIBCP.LIB']
    missing=[str(BASE/r['source']) for r in units if not (BASE/r['source']).is_file()]
    missing += [str(p) for p in libs if not p.is_file()]
    if missing: raise FileNotFoundError('Missing build inputs: '+'; '.join(missing))
    if args.check:
        print(json.dumps({'sourceUnits':len(units),'vc98':str(sdk),'sourcesPresent':True},indent=2));return 0
    # Keep immutable compiler output apart from objects whose COFF visibility is adapted for LINK.
    rawdir=out/'compiler-obj';rawdir.mkdir(exist_ok=True)
    inputs=[p for root in (BASE/'src',BASE/'third_party',sdk/'Include',sdk/'ATL/Include')
            for p in root.rglob('*') if p.is_file()]
    inputs += [sdk/'Bin'/name for name in ('CL.EXE','C1.DLL','C1XX.DLL','C2.DLL','ML.EXE')]
    fingerprint=hashlib.sha256(''.join(str(p)+digest(p) for p in sorted(inputs)).encode('utf-8')).hexdigest()
    cachepath=out/'compiler-cache.json'
    cache=read(cachepath) if cachepath.exists() and not args.fresh else {}
    if cache.get('fingerprint')!=fingerprint:cache={'fingerprint':fingerprint,'objects':{}}
    compiled=[]
    for number,row in enumerate(units):
        src=BASE/row['source']; actual=src
        obj=objdir/('%03d_%s.obj'%(number,src.stem))
        if row['source']=='src/native/game_audio.cpp':
            text=src.read_text(encoding='utf-8')
            actual=out/'generated/game_audio.cpp';actual.parent.mkdir(exist_ok=True)
            actual.write_text(text.replace('ActiveSoundEntry* active_sounds;\nint active_sound_count;',
                'extern ActiveSoundEntry* active_sounds;\nextern int active_sound_count;'),encoding='utf-8')
        if row.get('relocatableLeaf'):
            actual=out/'generated'/src.name;actual.parent.mkdir(exist_ok=True)
            actual.write_text(relocate(src.read_text(encoding='utf-8'),config['imageMetadata']['dataRanges'])[0],encoding='utf-8')
        flags=[f for f in row['flags'] if f.lower() not in ('/nologo','/c','/gr','/gr-','/d_mt','/ml','/mt','/md')]
        asm=src.suffix.lower()=='.asm'
        if not asm:
            flags+=['/GR' if src.suffix.lower() in ('.cpp','.cxx') else '/GR-','/ML','/DTITBIT_SOURCE_LINK']
            if compat: flags+=['/DTITBIT_COMPAT']
            flags+=['/I'+match.short_path(src.parent),'/I'+match.short_path(BASE/'src/lanes/include')]
            flags+=['/I'+match.short_path(BASE/inc) for inc in row['includes']]
            if src.name=='dom_parser.cpp':
                flags+=['/I'+match.short_path(BASE/'src/lanes/engine41/cpxml43/xmlincl'),
                        '/I'+match.short_path(BASE/'src/lanes/engine41/cpxml43/xmlincl/SAX')]
            flags+=['/I'+match.short_path(sdk/'Include'),'/I'+match.short_path(sdk/'ATL/Include')]
        tool=sdk/'Bin'/('ML.EXE' if asm else 'CL.EXE')
        raw=rawdir/obj.name
        command=[str(tool),'/nologo','/c',*flags,'/Fo'+match.short_path(rawdir)+'/'+obj.name,match.short_path(actual)]
        previous=cache['objects'].get(row['source'],{})
        if previous.get('command')!=command or not raw.exists() or previous.get('sha256')!=digest(raw):
            run(command,obj.with_suffix('.log'),src.parent)
            cache['objects'][row['source']]={'command':command,'sha256':digest(raw)}
            cachepath.write_text(json.dumps(cache),encoding='utf-8')
        shutil.copy2(raw,obj)
        compiled.append({**row,'object':str(obj),'command':command,'sourceSha256':digest(src),'objectSha256':digest(obj)})
        if (number+1)%25==0:print('compiled',number+1,'/',len(units),flush=True)
    compiled=[r for r in compiled if not r['source'].endswith('/00498de0-dinput_thunk.asm')]
    bindings={k:int(v,16) for k,v in config['bindings'].items()}
    match.IMPORTS.update(config['imageMetadata']['imports'])
    def address(name,local=None):
        merged=bindings.copy()
        for key,value in (local or {}).items():
            merged[match.UNIT+':'+key]=int(value,16) if isinstance(value,str) else value
        return match.address_of(name,merged)
    # Preserve the source-owned game callback order; VC6 owns its own CRT/STL startup.
    initializers={}; initializer_order=set(config['initializerOrder'])
    layout=read(BASE/'src/lanes/codex/link_units.json')
    definitions=set();candidates=collections.defaultdict(list);undefined=collections.defaultdict(set)
    objects={r['object']:CoffObject(r['object']) for r in compiled}
    for row in compiled:
        src=BASE/row['source'];obj=objects[row['object']]
        match.UNIT=src.stem
        local=dict(row.get('symbolAddresses',{}))
        sidecar=src.with_suffix('.bindings.json')
        spec=read(sidecar) if sidecar.exists() else {}
        local.update(spec.get('bindings',{}))
        for name in spec.get('unboundSymbols',[]): local[name]=None
        static=layout['staticInitializers'].get('decompilation/'+row['source'])
        if static:local.update(bind_static_callbacks(obj,static))
        # Export independently identified per-TU static callbacks and data names.
        for symbol in list(obj.symbols.values()):
            a=address(symbol['name'],local)
            section=obj.sections.get(symbol['section'])
            if symbol['storage'] in (2,3) and section and section['flags']&0x20 and a in initializer_order and a not in initializers:
                label='_public_initializer_%08x'%a
                obj=append_label(obj,label,symbol['section'],symbol['value'],symbol_type=0x20)
                initializers.setdefault(a,label)
        obj,_=exclude_automatic_tables(obj)
        data=bytearray(obj.data);offset=struct.unpack_from('<I',data,8)[0]
        for symbol in obj.symbols.values():
            sec=obj.sections.get(symbol['section'])
            if (symbol['storage']==3 and sec and not sec['flags']&0x20 and not symbol['type']&0x20
                    and ('@?1?' in symbol['name'] or src.parent.name=='data') and symbol['name'] in local):
                data[offset+18*symbol['index']+16]=2
        pathlib.Path(row['object']).write_bytes(data);obj=CoffObject(row['object']);objects[row['object']]=obj
        for sym in obj.symbols.values():
            if sym['storage']!=2:continue
            a=address(sym['name'],local)
            if sym['section']>0 or sym['value']>0:
                definitions.add(sym['name'])
                if a is not None:
                    candidates[a].append((row['priority'],sym['name']))
            elif a is not None:undefined[sym['name']].add(a)
        row['_fieldSpec']=spec
    # Existing typed field locators prove offsets within actual source storage.
    for row in compiled:
        for name,spec in row.pop('_fieldSpec').get('fieldAliases',{}).items():
            owners=[r for r in compiled if spec['owner'] in objects[r['object']].by_name
                    and objects[r['object']].by_name[spec['owner']]['section']>0]
            if len(owners)!=1:raise ValueError('Missing field owner '+spec['owner'])
            owner=owners[0];obj,proof=expose_field(objects[owner['object']],objects[row['object']],name,spec,
                                                  address(spec['owner']),address(name))
            pathlib.Path(owner['object']).write_bytes(obj.data);objects[owner['object']]=obj
            definitions.add(name)
            a=address(name)
            if a is not None:candidates[a].append((owner['priority'],name))
    for name in layout['identicalComdatProviders']:
        for path,(obj,proof) in equivalent_comdat_providers(list(objects.values()),name).items():
            pathlib.Path(path).write_bytes(obj.data);objects[path]=obj
    runtime_symbols=set()
    match.UNIT=''
    for lib in libs:
        symbols=library_symbols(lib);runtime_symbols.update(symbols)
        for name in symbols:
            a=address(name)
            if a is not None:candidates[a].append((-10,name))
    for name,a in config['imageMetadata']['imports'].items():
        for symbol in runtime_symbols:
            if symbol.startswith('__imp_') and symbol[len('__imp_'):].lstrip('_@').split('@')[0]==name:
                candidates[a].append((-10,symbol))
    aliases={}
    for name,addresses in undefined.items():
        if name in definitions or name in runtime_symbols or len(addresses)!=1:continue
        choices=candidates.get(next(iter(addresses)),[])
        if choices:
            target=min(choices)[1]
            if target!=name:aliases[name]=target
    for record in config['aliases']:
        if record['alias'] not in definitions and record['alias'] not in runtime_symbols:
            aliases.setdefault(record['alias'],record['provider'])
    commons={s['name'] for obj in objects.values() for s in obj.symbols.values()
             if s['storage']==2 and s['section']==0 and s['value']>0}
    section_names={s['name'] for obj in objects.values() for s in obj.symbols.values()
                   if s['storage']==2 and s['section']>0}
    invalid=set(aliases.values()) & (commons-section_names)
    if invalid:raise ValueError('Weak alias to COMMON has no source section owner: '+', '.join(sorted(invalid)))
    aliases_dir=out/'aliases';aliases_dir.mkdir(exist_ok=True)
    alias_members=[]
    for i,(name,target) in enumerate(sorted(aliases.items())):
        path=aliases_dir/('%04d.obj'%i);aliases_object(path,{name:target});alias_members.append(str(path))
    # CRT invokes this ordered game list after its own registered initializers.
    init=out/'generated/game_initializers.c';init.parent.mkdir(exist_ok=True)
    labels=[initializers[a] for a in config['initializerOrder'] if a in initializers]
    init.write_text('\n'.join('extern void __cdecl '+name[1:]+'(void);' for name in labels)+
        '\n#pragma data_seg(".CRT$XCV")\nvoid (__cdecl *public_game_initializers[])(void) = {\n'+
        ',\n'.join(name[1:] for name in labels)+'\n};\n',encoding='ascii')
    init_obj=objdir/'game_initializers.obj'
    run([str(sdk/'Bin/CL.EXE'),'/nologo','/c','/ML','/Fo'+match.short_path(objdir)+'/'+init_obj.name,match.short_path(init)],out/'initializers.log')
    response(sdk/'Bin/LIB.EXE','sources',['/nologo','/OUT:'+str(out/'sources.lib'),*[r['object'] for r in compiled]])
    response(sdk/'Bin/LIB.EXE','aliases',['/nologo','/OUT:'+str(out/'aliases.lib'),*alias_members])
    from rc_compile import compile_rc
    from project_icon import write_icon
    res=out/'game.res';resource=out/'game_resources.obj'
    # Keep resource IDs/dialogs, with our own artwork rather than disc ICOs.
    resources=out/'generated/resources';resources.mkdir(parents=True,exist_ok=True)
    write_icon(resources/'titbit-modern.ico')
    text=(BASE/'src/lanes/codex/resources/titbit.rc').read_text(encoding='utf-8')
    text=text.replace('icon_202_1049.ico','titbit-modern.ico').replace('icon_128_1033.ico','titbit-modern.ico')
    (resources/'titbit.rc').write_text(text,encoding='utf-8')
    res.write_bytes(compile_rc(resources/'titbit.rc'))  # same bytes as Windows SDK rc.exe
    run([str(sdk/'Bin/CVTRES.EXE'),'/NOLOGO','/MACHINE:IX86','/OUT:'+match.short_path(out)+'/'+resource.name,match.short_path(res)],out/'cvtres.log')
    exe=out/('TitbitModern.exe' if compat else 'titbit.exe')
    response(sdk/'Bin/LINK.EXE','link',['/nologo','/NODEFAULTLIB','/MACHINE:IX86','/SUBSYSTEM:WINDOWS,4.0',
        '/ENTRY:'+('SourceCompatStartup' if compat else 'WinMainCRTStartup'),'/INCREMENTAL:NO','/OPT:REF',
        '/INCLUDE:_public_game_initializers','/OUT:'+str(exe),'/MAP:'+str(exe.with_suffix('.map')),
        str(resource),str(init_obj),*[r['object'] for r in compiled if r['source'].endswith('/template_guards.asm')],
        str(out/'sources.lib'),str(out/'aliases.lib'),*map(str,libs)])
    if compat:
        launch=BASE/'src/compat/launcher.cpp';launch_obj=objdir/'launcher.obj'
        run([str(sdk/'Bin/CL.EXE'),'/nologo','/c','/O2','/ML','/Fo'+match.short_path(objdir)+'/'+launch_obj.name,
             '/I'+match.short_path(sdk/'Include'),match.short_path(launch)],out/'launcher-compile.log')
        response(sdk/'Bin/LINK.EXE','launcher',['/nologo','/SUBSYSTEM:WINDOWS','/ENTRY:WinMainCRTStartup','/NODEFAULTLIB',
                 '/OUT:'+str(out/'TitbitSettings.exe'),str(launch_obj),*[str(sdk/'Lib'/x) for x in ('KERNEL32.LIB','USER32.LIB','GDI32.LIB')]])
        (out/'titbit_compat.ini').write_text('[compat]\ndisplay=fullscreen\nresolution=1024x768\naspect=fit\nsetup=modern\ncd=folder\nvideo=auto\nreplay=off\n',encoding='ascii')
    manifest={'linked':True,'compat':compat,'linkExit':0,'sourceUnits':len(compiled),'executable':str(exe),
              'sha256':digest(exe),'objects':[{k:v for k,v in r.items() if not k.startswith('_')} for r in compiled],
              'runtime':'installed VC98 LIBC.LIB + LIBCP.LIB','vc98':str(sdk),
              'libraries':[{'name':p.name,'sha256':digest(p)} for p in libs],
              'gameInitializers':len(labels),'aliases':aliases}
    (out/'build-info.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('LINK OK:',exe,'SHA256',digest(exe),flush=True)
    return 0


if __name__=='__main__':
    sys.exit(main())
