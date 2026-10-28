#!/usr/bin/env python3
"""Scoped 8088 MZ audit; executable/code bounds from MZ and LINK map."""
import argparse,collections,hashlib,json,pathlib,re,struct,subprocess
p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);a=p.parse_args();b=a.exe.read_bytes();assert b[:2]==b'MZ'
header=struct.unpack_from('<14H',b);image=header[4]*16;entry=image+header[11]*16+header[10];mp=a.exe.with_suffix('.MAP').read_text()
segments=[]
for seg,off,size in re.findall(r'^\S+\s+CODE\s+\S+\s+([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+([0-9A-Fa-f]+)',mp,re.M):
 lo=int(seg,16)*16+int(off,16);segments.append((lo,lo+int(size,16)))
assert segments,'Expected an Open Watcom DOS map with CODE segment sizes'
roots=sorted(set(image+int(s,16)*16+int(o,16) for s,o in re.findall(r'^([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})[+* ]+\S',mp,re.M)))
def code_end(address):
 for lo,hi in segments:
  if image+lo<=address<image+hi:return image+hi
 return None
queue=collections.deque([entry]+[x for x in roots if code_end(x)]);decoded={};suspects=[];indirect=[];far=[]
post8088=r'\b(pusha\w*|popa\w*|enter\w*|leave\w*|bound|ins[bw]|outs[bw]|arpl|movzx|movsx|bsf|bsr|lss|lfs|lgs|set\w+|shld|shrd|ud2|cpuid|rdtsc|clts|lldt|ltr|smsw|lmsw|lgdt|lidt|sgdt|sidt|lar|lsl|verr|verw)\b|\b(eax|ebx|ecx|edx|esp|ebp|esi|edi|fs|gs)\b|^push\s+\$|^(shl|shr|sar|rol|ror|rcl|rcr)\s+\$(?!1,)|^imul\s+\$'
while queue:
 start=queue.popleft();end=code_end(start)
 if start in decoded or not end:continue
 out=subprocess.check_output(['objdump','-D','-b','binary','-m','i8086',f'--start-address={start}',f'--stop-address={end}',str(a.exe)],text=True)
 for row in out.splitlines():
  m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*([^\t]+)',row)
  if not m:continue
  address=int(m[1],16);instruction=m[3].strip()
  if address in decoded:break
  decoded[address]=instruction
  encoding=bytes.fromhex(m[2]);opcode=0
  while opcode<len(encoding) and encoding[opcode] in (0x26,0x2e,0x36,0x3e,0xf0,0xf2,0xf3):opcode+=1
  newer=opcode<len(encoding) and encoding[opcode] in (0x0f,0x64,0x65,0x66,0x67,0x68,0x6a,0xc0,0xc1)
  if newer or re.search(post8088,instruction) or re.match(r'^imul\s+.*?,',instruction) or '(bad)' in instruction:suspects.append({'image_offset':hex(address-image),'instruction':instruction,'encoding':encoding.hex()})
  if re.match(r'^(j\w+|loop\w*|call)\s+0x',instruction):queue.append(int(re.search(r'0x([0-9a-f]+)',instruction)[1],16))
  if '*' in instruction and instruction.startswith(('call','jmp','lcall','ljmp')):indirect.append({'image_offset':hex(address-image),'instruction':instruction})
  if instruction.startswith(('lcall','ljmp')):far.append({'image_offset':hex(address-image),'instruction':instruction})
  if instruction.startswith(('ret','lret','iret','hlt','jmp ','ljmp ')):break
io=[{'image_offset':hex(address-image),'instruction':s} for address,s in decoded.items() if re.match(r'^(out|in|cli|sti|popf|pushf|int)\b',s)]
result={'exe':a.exe.name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'code_segments':segments,'entry':hex(entry-image),'code_symbol_roots':sum(bool(code_end(x)) for x in roots),'reachable_instructions_checked':len(decoded),'post8088_suspects':suspects,'indirect_branch_sites':indirect,'far_transfer_sites':far,'io_and_interrupt_flag_sites':io,'MZ_minimum_extra_paragraphs':header[5],'MZ_maximum_extra_paragraphs':header[6],'static_image_bss_stack_bytes':header[7]*16+header[8],'scope':'Control-flow reachability from MZ entry and all public CODE symbols including handlers, direct near calls and conditional branches. Runtime indirect targets, CRT dispatch tables, external BIOS/DOS instructions and unreachable bytes excluded. Build C16 -0 and WASM -0 complement this scoped audit; not hardware certification.'}
print(json.dumps(result,indent=2));raise SystemExit(bool(suspects))
