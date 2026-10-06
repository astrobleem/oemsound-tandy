#!/usr/bin/env python3
"""Read-only scoped 8088 disassembly audit of an NE executable, using objdump."""
import argparse,collections,hashlib,json,pathlib,re,struct,subprocess
ap=argparse.ArgumentParser();ap.add_argument('exe',type=pathlib.Path);a=ap.parse_args()
p=a.exe;b=p.read_bytes();ne=struct.unpack_from('<I',b,0x3c)[0]
assert b[ne:ne+2]==b'NE'
tab=ne+struct.unpack_from('<H',b,ne+0x22)[0]
shift=struct.unpack_from('<H',b,ne+0x32)[0]
sec,size,flags,minimum=struct.unpack_from('<4H',b,tab)
base=sec<<shift;end=base+size
entry=struct.unpack_from('<H',b,ne+0x14)[0]
assert struct.unpack_from('<H',b,ne+0x16)[0]==1
excluded_data=['__wflags']
roots=sorted(set(int(m[1],16) for row in p.with_suffix('.MAP').read_text().splitlines() if (m:=re.search(r'0001:([A-Fa-f0-9]+)\s+(\S+)',row)) and m[2] not in excluded_data))
queue=collections.deque([base+entry]+[base+x for x in roots]);decoded={};suspects=[];indirect=[]
post8088=r'\b(pusha\w*|popa\w*|enter\w*|leave\w*|bound|ins[bw]|outs[bw]|arpl|movzx|movsx|bsf|bsr|lss|lfs|lgs|set\w+|shld|shrd|ud2|cpuid|rdtsc|clts|lldt|ltr|smsw|lmsw|lgdt|lidt|sgdt|sidt|lar|lsl|verr|verw)\b|\b(eax|ebx|ecx|edx|esp|ebp|esi|edi|fs|gs)\b|^push\s+\$|^(shl|shr|sar|rol|ror|rcl|rcr)\s+\$(?!1,)|^imul\s+\$'
while queue:
    start=queue.popleft()
    if start in decoded or not base<=start<end:continue
    out=subprocess.check_output(['objdump','-D','-b','binary','-m','i8086',f'--start-address={start}',f'--stop-address={end}',str(p)],text=True)
    for row in out.splitlines():
        m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*([^\t]+)',row)
        if not m:continue
        address=int(m[1],16);instruction=m[3].strip()
        if address in decoded:break
        decoded[address]=instruction
        if re.search(post8088,instruction) or '(bad)' in instruction:
            suspects.append({'offset':hex(address-base),'instruction':instruction})
        if re.match(r'^(j\w+|loop\w*|call)\s+0x',instruction):
            queue.append(int(re.search(r'0x([0-9a-f]+)',instruction)[1],16))
        if instruction.startswith(('ret','lret','iret','hlt','jmp ')):
            if '*' in instruction:indirect.append({'offset':hex(address-base),'instruction':instruction})
            break
io=[{'offset':hex(address-base),'instruction':s} for address,s in decoded.items() if re.match(r'^(out|in|cli|sti|popf|pushf|int)\b',s)]
result={'executable':p.name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'code_bytes':size,'NE_entry_offset':hex(entry),'symbol_roots':len(roots),'excluded_public_data_symbols':excluded_data,'reachable_instructions_checked':len(decoded),'post8088_suspects':suspects,'indirect_branch_sites':indirect,'io_or_interrupt_flag_instructions':io,'scope':'Direct control-flow reachability from NE entry and public segment-1 symbols, including near calls and conditional branches. Imported Windows routines, unresolved indirect continuations and unreachable bytes excluded; not exhaustive ISA or hardware certification.'}
print(json.dumps(result,indent=2))
raise SystemExit(bool(suspects))
