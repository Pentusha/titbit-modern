"""Translate Clang MSVC C++ EH funclets into native DWARF landing pads.

This is a source-build compiler bridge, not a binary translator. Its input is
LLVM IR emitted from project C++ sources. Keep the Microsoft record/vtable ABI
with an i386-pc-windows-msvc-elf backend; link the exception adapter separately.
The original Windows executable is never read.
"""
import argparse
from dataclasses import dataclass
from pathlib import Path
import re

LOCAL = r'%([-A-Za-z$._0-9]+)'
LABEL = r'([-A-Za-z$._0-9]+)'
PAIR = '{ ptr, i32 }'


@dataclass
class Block:
    name: str
    lines: list


def normalise_numbers(header, body):
    args = [int(x) for x in re.findall(r'%(\d+)',header)]
    entry = max(args,default=-1)+1
    header = re.sub(r'%(\d+)\b',r'%v\1',header)
    body = [re.sub(r'%(\d+)\b',r'%v\1',line) for line in body]
    body = [re.sub(r'^(\d+):',r'v\1:',line) for line in body]
    first = next((line for line in body if line.strip() and not line.lstrip().startswith(';')), '')
    if not re.match(r'^'+LABEL+r':',first):
        body.insert(0,f'v{entry}:')
    return header,body


def split_blocks(body):
    blocks=[]
    for line in body:
        match=re.match(r'^'+LABEL+r':',line)
        if match:
            blocks.append(Block(match.group(1),[]))
        elif blocks:
            blocks[-1].lines.append(line)
    # An invoke is printed on two lines by LLVM. Folding it simplifies edge edits.
    for block in blocks:
        lines=[]
        for line in block.lines:
            if line.lstrip().startswith(('to label ','unwind label ')) and lines:
                lines[-1]+=' '+line.strip()
            else:
                lines.append(line)
        block.lines=lines
    return blocks


def convert_function(header,body):
    if not any(re.search(r'\b(catchswitch|catchpad|cleanuppad)\b',line) for line in body):
        return header,body,False
    header,body=normalise_numbers(header,body)
    header=re.sub(r'personality ptr @[^ ]+', 'personality ptr @__gxx_personality_v0',header)
    blocks=split_blocks(body)
    pads={}
    token={}
    for block in blocks:
        for line in block.lines:
            found=re.search(LOCAL+r' = (catchswitch|catchpad|cleanuppad) within (none|'+LOCAL+r')',line)
            if not found:
                continue
            name,kind,within,_=found.groups()
            parent=within[1:] if within!='none' else None
            info={'token':name,'kind':kind,'parent':parent,'block':block.name,'line':line}
            pads[block.name]=info
            token[name]=info
    def catch_stack(name):
        stack=[]
        seen=set()
        while name is not None:
            if name in seen or name not in token:
                raise ValueError('Invalid funclet parent '+str(name))
            seen.add(name)
            info=token[name]
            if info['kind']=='catchpad':stack.append(name)
            name=info['parent']
        return list(reversed(stack))
    def close_catches(source,target):
        old,new=catch_stack(source),catch_stack(target)
        count=0
        while count<min(len(old),len(new)) and old[count]==new[count]:count+=1
        if old[:count]!=new[:count] or len(new)>count:
            raise ValueError('Cannot enter a catch scope while unwinding')
        return ['  call void @__cxa_end_catch()' for _ in old[count:]]
    incoming={name:[] for name,p in pads.items() if p['kind']!='catchpad'}
    extra=[]
    phi_edges={}
    cleanup_targets={}
    for block in blocks:
        for line in block.lines:
            m=re.search(r'cleanupret from '+LOCAL+r' unwind (?:label %'+LABEL+r'|to caller)',line)
            if m:cleanup_targets.setdefault(m.group(1),[]).append(m.group(2))
    def has_catch(target,seen=None):
        if not target:return False
        seen=set() if seen is None else seen
        if target in seen:return False
        seen.add(target)
        info=pads[target]
        if info['kind']=='catchswitch':return True
        return any(has_catch(t,seen.copy()) for t in cleanup_targets.get(info['token'],[]))
    def forward(lines,source,target,pair,origin,restart=False,from_block=None):
        parent=token[pads[target]['token']]['parent'] if target else None
        lines.extend(close_catches(source,parent))
        if target:
            incoming[target].append((pair,origin))
            if from_block and from_block!=origin:phi_edges[(target,from_block)]=origin
            lines.append(f'  br label %{target}')
        elif restart:
            # A catch-all LSDA stopped the phase-one search in this frame. If
            # our MS type matcher rejects it, restarting the search is required;
            # a bare _Unwind_Resume can repeatedly select this same catch-all.
            number=len(extra)
            exc=f'%titbit_rethrow_exc_{number}'
            begin=f'%titbit_rethrow_begin_{number}'
            unwind=f'titbit_rethrow_cleanup_{number}'
            dead=f'titbit_rethrow_dead_{number}'
            raised=f'%titbit_rethrow_pair_{number}'
            lines.extend([f'  {exc} = extractvalue {PAIR} {pair}, 0',
                          f'  {begin} = call ptr @__cxa_begin_catch(ptr {exc})',
                          f'  invoke void @__cxa_rethrow() to label %{dead} unwind label %{unwind}'])
            extra.extend([Block(unwind,[f'  {raised} = landingpad {PAIR} cleanup',
                                        '  call void @__cxa_end_catch()',
                                        f'  resume {PAIR} {raised}']),
                          Block(dead,['  unreachable'])])
        else:
            lines.append(f'  resume {PAIR} {pair}')
    for block in blocks:
        info=pads.get(block.name)
        current=info['token'] if info else None
        output=[]
        for line in block.lines:
            bundle=re.search(r'"funclet"\(token '+LOCAL+r'\)',line)
            active=bundle.group(1) if bundle else current
            line=re.sub(r'\s*\[\s*"funclet"\(token '+LOCAL+r'\)\s*\]','',line)
            if 'invoke ' in line and 'unwind label ' in line:
                target=re.search(r'unwind label %'+LABEL,line).group(1)
                if target not in incoming:
                    raise ValueError('Invoke unwinds into a non-pad '+target)
                bridge=f'titbit_unwind_{len(extra)}'
                pair=f'%titbit_pair_{bridge}'
                line=re.sub(r'unwind label %'+re.escape(target)+r'\b','unwind label %'+bridge,line)
                output.append(line)
                clause='catch ptr null' if has_catch(target) else 'cleanup'
                landing=Block(bridge,[f'  {pair} = landingpad {PAIR} {clause}'])
                forward(landing.lines,active,target,pair,bridge,from_block=block.name)
                extra.append(landing)
                continue
            if re.search(r'\bcatchswitch within\b',line):
                handlers=re.findall(r'label %'+LABEL,line.split('unwind')[0])
                unwind=re.search(r'unwind label %'+LABEL,line)
                fallback=unwind.group(1) if unwind else None
                pair=f'%titbit_pair_{block.name}'
                exception=f'%titbit_exc_{block.name}'
                wrapped=f'%titbit_wrapped_{block.name}'
                output.extend([f'  {exception} = extractvalue {PAIR} {pair}, 0',
                               f'  {wrapped} = call ptr @__cxa_get_exception_ptr(ptr {exception})'])
                for number,handler in enumerate(handlers):
                    h=pads[handler]
                    match=re.search(r'\[ptr (.+?), i32 (-?\d+), ptr (.+?)\]',h['line'])
                    if not match:raise ValueError('Unknown catchpad '+h['line'])
                    typ,flags,dest=match.groups()
                    if typ=='null':
                        output.append(f'  br label %{handler}')
                        break
                    next_block=f'titbit_match_{block.name}_{number}'
                    matched=f'%titbit_matched_{block.name}_{number}'
                    output.extend([f'  {matched} = call i1 @titbit_eh_match(ptr {wrapped}, ptr {typ}, i32 {flags}, ptr {dest})',
                                   f'  br i1 {matched}, label %{handler}, label %{next_block}',
                                   next_block+':'])
                else:
                    origin=next_block if handlers else block.name
                    forward(output,info['parent'],fallback,pair,origin,restart=not fallback,from_block=block.name)
                continue
            if re.search(r'\bcatchpad within\b',line):
                dispatch=token[info['parent']]['block']
                output.append(f'  %{info["token"]} = call ptr @__cxa_begin_catch(ptr %titbit_exc_{dispatch})')
                continue
            if re.search(r'\bcleanuppad within\b',line):
                # The token is only used by funclet bundles and cleanupret.
                continue
            catchret=re.search(r'catchret from '+LOCAL+r' to label %'+LABEL,line)
            if catchret:
                source,target=catchret.groups()
                output.extend(close_catches(source,token[source]['parent']))
                output.append('  br label %'+target)
                continue
            cleanupret=re.search(r'cleanupret from '+LOCAL+r' unwind (?:label %'+LABEL+r'|to caller)',line)
            if cleanupret:
                source,target=cleanupret.groups()
                owner=token[source]['block']
                forward(output,source,target,f'%titbit_pair_{owner}',block.name,restart=not target)
                continue
            output.append(line)
        block.lines=output
    for block in blocks:
        if block.name in incoming:
            for n,line in enumerate(block.lines):
                if ' = phi ' not in line:continue
                for (target,old),new in phi_edges.items():
                    if target==block.name:
                        line=re.sub(r', %'+re.escape(old)+r'\s*\]',', %'+new+' ]',line)
                block.lines[n]=line
            edges=incoming[block.name]
            if not edges:
                # A Windows catch-all catchswitch still names an unwind parent.
                # Our catch-all branch is unconditional, so that parent can
                # become unreachable and will be pruned below with its PHIs.
                continue
            values=', '.join(f'[ {pair}, %{pred} ]' for pair,pred in edges)
            block.lines.insert(0,f'  %titbit_pair_{block.name} = phi {PAIR} {values}')
    result=[]
    for block in blocks+extra:
        result.append(block.name+':')
        result.extend(block.lines)
    reparsed=split_blocks(result)
    by_name={block.name:block for block in reparsed}
    reachable=set()
    queue=[reparsed[0].name]
    while queue:
        name=queue.pop()
        if name in reachable:continue
        reachable.add(name)
        if name not in by_name:raise ValueError('Unknown branch block '+name)
        for line in by_name[name].lines:
            queue.extend(re.findall(r'label %'+LABEL,line))
    result=[]
    for block in reparsed:
        if block.name not in reachable:continue
        result.append(block.name+':')
        for line in block.lines:
            if ' = phi ' in line:
                start=line.index(' = phi ')
                prefix,tail=line[:start+7],line[start+7:]
                first=tail.find('[')
                if first!=-1:
                    kept=[]
                    depth=0;begin=0
                    for n,ch in enumerate(tail[first:],first):
                        if ch=='[':
                            if depth==0:begin=n
                            depth+=1
                        elif ch==']':
                            depth-=1
                            if depth==0:
                                entry=tail[begin:n+1]
                                pred=re.search(r', %'+LABEL+r'\s*\]$',entry)
                                if not pred or pred.group(1) in reachable:kept.append(entry)
                    line=prefix+tail[:first]+', '.join(kept)
            result.append(line)
    return header,result,True


def convert(text):
    lines=text.splitlines()
    out=[]
    changed=False
    i=0
    while i<len(lines):
        if lines[i].startswith('define '):
            header=lines[i]
            i+=1
            body=[]
            while lines[i]!='}':
                body.append(lines[i]);i+=1
            header,body,did=convert_function(header,body)
            changed|=did
            out.extend([header,*body,'}'])
        else:
            out.append(lines[i])
        i+=1
    if changed:
        declarations='''
declare i32 @__gxx_personality_v0(...)
declare ptr @__cxa_get_exception_ptr(ptr)
declare ptr @__cxa_begin_catch(ptr)
declare void @__cxa_end_catch()
declare void @__cxa_rethrow()
declare i1 @titbit_eh_match(ptr, ptr, i32, ptr)
'''
        out.extend(declarations.strip().splitlines())
    text='\n'.join(out)+'\n'
    # The native adapter uses cdecl consistently, unlike Clang's Windows runtime
    # declaration (stdcall) paired with its generated throw call (cdecl).
    text=text.replace('@_CxxThrowException','@titbit_CxxThrowException')
    text=re.sub(r'declare (?:dso_local )?x86_stdcallcc void @titbit_CxxThrowException',
                'declare void @titbit_CxxThrowException',text)
    return text


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('source',type=Path)
    p.add_argument('dest',type=Path)
    args=p.parse_args()
    args.dest.write_text(convert(args.source.read_text()))
