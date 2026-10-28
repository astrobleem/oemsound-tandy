#!/usr/bin/env python3
"""Repackage existing media plus a qualified executable; developer tool only."""
from pathlib import Path
import argparse,zipfile,hashlib
p=argparse.ArgumentParser();p.add_argument('--exe',type=Path,required=True);p.add_argument('--assets',type=Path,required=True,help='Extracted runtime directory, including LOWRES/');p.add_argument('--out',type=Path,required=True);a=p.parse_args()
assert not a.out.exists(),'Use a new output ZIP'
root=Path(__file__).resolve().parents[1];members={}
for folder in ['', 'LOWRES/']:
 for name in ['WEEZER.WZV','WEEZER.WZM','SPEAKER.WZM','WEEZER.CUE']:
  members[folder+name]=(a.assets/(folder+name)).read_bytes()
 members[folder+'DOSPLAY.EXE']=a.exe.read_bytes()
 for name in ['README.TXT','RESULTS.TXT','LICENSE','LIBTANDY.TXT','ATTRIB.TXT']:
  members[folder+name]=(a.assets/name).read_bytes()
 for part in folder.split('/'):
  assert len(part)<=8
manifest=''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\r\n' for name,data in sorted(members.items()))
members['SHA256.TXT']=manifest.encode('ascii')
a.out.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(a.out,'w',zipfile.ZIP_DEFLATED,9) as z:
 for name,data in sorted(members.items()):
  info=zipfile.ZipInfo(name,(2026,10,28,0,0,0));info.create_system=3;info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
print(dict(bytes=a.out.stat().st_size,sha256=hashlib.sha256(a.out.read_bytes()).hexdigest()))
