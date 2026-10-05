import pathlib,struct,subprocess,re,json,collections,sys,hashlib
p=pathlib.Path(sys.argv[1]);b=p.read_bytes();h=struct.unpack_from('<14H',b);base=h[4]*16;entry=h[11]*16+h[10];m=p.with_suffix('.MAP').read_text()
ranges=[(int(a,16),int(z,16)+1) for a,z in re.findall(r'^ ([0-9A-F]+)H ([0-9A-F]+)H.* CODE\s*$',m,re.M)]
roots=[int(a,16)*16+int(z,16) for a,z in re.findall(r'^ ([0-9A-F]{4}):([0-9A-F]{4})       ',m,re.M) if any(lo<=int(a,16)*16+int(z,16)<hi for lo,hi in ranges)]
q=collections.deque([entry]+roots);seen={};bad=[]
pat=r'\b(pusha\w*|popa\w*|enter\w*|leave\w*|bound|ins[bw]|outs[bw]|arpl|movzx|movsx|bsf|bsr|lss|lfs|lgs|set\w+|shld|shrd|ud2|cpuid|rdtsc|clts|lldt|ltr|smsw|lmsw|lgdt|lidt|sgdt|sidt|lar|lsl|verr|verw)\b|\b(eax|ebx|ecx|edx|esp|ebp|esi|edi|fs|gs)\b|^push\s+\$|^(shl|shr|sar|rol|ror|rcl|rcr)\s+\$(?!1,)|^imul\s+\$'
while q:
 a=q.popleft()
 if a in seen:continue
 rr=next(((lo,hi) for lo,hi in ranges if lo<=a<hi),None)
 if not rr:continue
 out=subprocess.check_output(['objdump','-D','-b','binary','-m','i8086',f'--start-address={base+a}',f'--stop-address={base+rr[1]}',str(p)],text=True)
 for line in out.splitlines():
  mm=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*([^\t]+)',line)
  if not mm:continue
  addr=int(mm[1],16)-base;ins=mm[3].strip()
  if addr in seen:break
  seen[addr]=ins
  if re.search(pat,ins) or '(bad)' in ins:bad.append([hex(addr),ins])
  if re.match(r'^(j\w+|loop\w*|call)\s+0x',ins):q.append(int(re.search(r'0x([0-9a-f]+)',ins)[1],16)-base)
  if ins.startswith(('ret','lret','iret','hlt','jmp ')):break
result={'exe':p.name,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'code_segments':ranges,'entry':hex(entry),'code_symbol_roots':len(roots),'reachable_instructions_checked':len(seen),'post8088_suspects':bad,'scope':'Scoped direct control flow from MZ entry and code MAP symbols; indirect targets and external hardware behavior not certified.'}
print(json.dumps(result,indent=2));sys.exit(bool(bad))
