#!/usr/bin/env python3
"""Independent exact host ownership oracle for production PIANO.C."""
import ctypes as C, json, random, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parent
class Event(C.Structure): _fields_=[('status',C.c_ubyte),('note',C.c_ubyte),('velocity',C.c_ubyte)]
class Out(C.Structure): _fields_=[('count',C.c_ubyte),('event',Event*14)]
class State(C.Structure): _fields_=[('octave',C.c_ubyte),('velocity',C.c_ubyte),('source',C.c_ubyte*14),('refs',C.c_ubyte*128),('held',C.c_ubyte)]
with tempfile.TemporaryDirectory(prefix='piano-core-') as d:
 so=Path(d)/'piano.so'
 subprocess.run(['cc','-x','c','-std=c89','-pedantic-errors','-Wall','-Wextra','-Wconversion','-Werror','-fPIC','-shared',str(root/'PIANO.C'),'-o',str(so)],check=True)
 lib=C.CDLL(str(so));s=State();o=Out();lib.pi_init(C.byref(s));src=[0]*14;octave=4;cases=0;events=0
 def run(action,a=0,b=0):
  global octave,cases,events
  expected=[]
  def release(i):
   if src[i]:
    n=src[i];src[i]=0
    if n not in src:expected.append((128,n,0))
  if action=='press':
   if 0<=a<14 and 0<=b<13:
    n=(octave+1)*12+b
    if src[a]!=n:
     release(a)
     if n not in src:expected.append((144,n,100))
     src[a]=n
   lib.pi_press(C.byref(s),C.c_uint(a),C.c_uint(b),C.byref(o))
  elif action=='release':
   if 0<=a<14:release(a)
   lib.pi_release(C.byref(s),C.c_uint(a),C.byref(o))
  elif action in ('panic','octave'):
   for i in range(14):release(i)
   if action=='panic':lib.pi_panic(C.byref(s),C.byref(o))
   else:
    octave=max(3,min(6,octave+a));lib.pi_octave(C.byref(s),C.c_int(a),C.byref(o))
  got=[(e.status,e.note,e.velocity) for e in o.event[:o.count]]
  assert got==expected,(cases,action,a,b,got,expected)
  assert list(s.source)==src
  assert list(s.refs)==[src.count(n) if n else 0 for n in range(128)]
  assert s.held==sum(bool(n) for n in src) and s.octave==octave
  assert o.count<=14
  cases+=1;events+=len(got)
 # Exact overlap and repeated ownership, chord, octave, glissando, bounds.
 for act,a,b in [('press',0,0),('press',0,0),('press',13,0),('release',0,0),('press',13,3),('release',13,0)]:run(act,a,b)
 for i in range(13):run('press',i,i)
 run('press',13,0);run('octave',1)
 for i in range(14):run('release',i)
 for k,scan in enumerate([0x1e,0x11,0x1f,0x12,0x20,0x21,0x14,0x22,0x15,0x23,0x16,0x24,0x25]):assert lib.pi_scan(scan)==k
 assert sum(lib.pi_scan(i)>=0 for i in range(256))==13
 rng=random.Random(8088)
 for i in range(100000):
  r=rng.randrange(20)
  if r<12:run('press',rng.randrange(17),rng.randrange(16))
  elif r<18:run('release',rng.randrange(17))
  elif r==18:run('panic')
  else:run('octave',rng.randrange(-6,7))
 run('panic')
 print(json.dumps({'passed':True,'cases':cases,'matched_events':events,'state_bytes_host':C.sizeof(s),'output_bytes_host':C.sizeof(o),'seed':8088,'scope':'Production strict C89 ownership core against independent Python reference; no native keyboard or hardware claim.'},indent=2))
