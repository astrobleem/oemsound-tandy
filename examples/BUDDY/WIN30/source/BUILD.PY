"""Build app-local Buddy Win16 components using external Open Watcom.
No emulator is launched and the published SOUND/MIDI engine is unchanged.
"""
import argparse, hashlib, json, os, shutil, struct, subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--watcom',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();wc=a.watcom.resolve();out=a.out.resolve()
    source=Path(__file__).resolve().parent;repo=source.parents[3]
    engine=repo/'experimental/WINLIB30/SOURCE'
    for f in ('BINNT/wcc.exe','BINNT/wasm.exe','BINNT/wlink.exe','H/WIN/windows.h','LIB286/WIN/clibs.lib'):
        if not (wc/f).is_file():p.error('Missing external input: '+str(wc/f))
    if out.exists():p.error('--out must name a new directory')
    for f in ('WINHOOK.ASM','TBRIDGE.ASM'):
        if not (engine/f).is_file():p.error('Missing current engine input: '+str(engine/f))
    out.mkdir(parents=True);env=os.environ.copy()
    env.update(WATCOM=str(wc),INCLUDE=str(wc/'H')+';'+str(wc/'H/WIN'))
    env['PATH']=str(wc/'BINNT')+os.pathsep+env['PATH'];logs=[];inputs={}
    def copy(origin,folder):
        dest=out/folder/origin.name;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(origin,dest)
        inputs[origin.relative_to(repo).as_posix()]=hashlib.sha256(origin.read_bytes()).hexdigest()
    def run(folder,tool,*args):
        command=[str(wc/'BINNT'/(tool+'.exe')),*args]
        r=subprocess.run(command,cwd=out/folder,env=env,capture_output=True,text=True)
        logs.append(' '.join(command)+'\n'+r.stdout+r.stderr)
        (out/'BUILD.LOG').write_text('\n'.join(logs))
        if r.returncode:raise SystemExit('Build failed: '+str(out/'BUILD.LOG'))
    def compile_c(folder,name,dll=False):
        run(folder,'wcc','-zW','-0','-ms','-os','-s','-w3',*(['-zu'] if dll else []),name)
    for f in ('TINYVID.C','TINYVID.LNK','BQSCHED.C','BQSCHED.H','SPKAPI.H','SPEAKER.H'):
        copy(source/f,'APP')
    copy(source.parent/'DRIVER/MIDIAPI.H','APP')
    compile_c('APP','TINYVID.C');run('APP','wlink','@TINYVID.LNK')
    for folder,c,asm,link in (('CLOCK','SPKCLK.C','SPKENTRY.ASM','SPKCLK.LNK'),('FASTCLK','FASTCLK.C','FASTENT.ASM','SPKFAST.LNK')):
        for f in (c,asm,link,'SPKAPI.H'):copy(source/folder/f,folder)
        if folder=='FASTCLK':
            copy(engine/'WINHOOK.ASM',folder)
            f=out/folder/link
            f.write_text(f.read_text().replace('library C:\\WATCOM\\LIB286\\WIN\\clibs.lib',"library '"+str(wc/'LIB286/WIN/clibs.lib')+"'"))
        compile_c(folder,c,dll=True);run(folder,'wasm','-0',asm)
        if folder=='FASTCLK':run(folder,'wasm','-0','WINHOOK.ASM')
        run(folder,'wlink','@'+link)
    for f in ('MIDIMAP.C','MIDIAPI.H','PERIODS.H','ENTRY.ASM','MIDIMAP.LNK'):
        copy(source.parent/'DRIVER'/f,'PSG')
    compile_c('PSG','MIDIMAP.C',dll=True);run('PSG','wasm','-0','ENTRY.ASM');run('PSG','wlink','@MIDIMAP.LNK')
    copy(engine/'TBRIDGE.ASM','BROKER')
    (out/'BROKER/TBRIDGE.LNK').write_text('system dos\nname TBRIDGE.EXE\noption map=TBRIDGE.MAP\nfile TBRIDGE.obj\n')
    run('BROKER','wasm','-0','TBRIDGE.ASM');run('BROKER','wlink','@TBRIDGE.LNK')
    runtime=out/'RUNTIME';runtime.mkdir();binaries={}
    for folder,name in (('APP','TINYVID.EXE'),('CLOCK','SPKCLK.DLL'),('FASTCLK','SPKFAST.DLL'),('PSG','MIDIMAP.DRV'),('BROKER','TBRIDGE.EXE')):
        f=out/folder/name;data=f.read_bytes();entry=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest())
        if folder!='BROKER':
            ne=struct.unpack_from('<I',data,0x3c)[0];assert data[ne:ne+2]==b'NE',name
            count,table=(struct.unpack_from('<H',data,ne+x)[0] for x in (0x1c,0x22));segments=[]
            for i in range(count):
                off,size,flags,minimum=struct.unpack_from('<4H',data,ne+table+i*8)
                segments.append(dict(bytes=size or 65536,minimum=minimum or 65536,data=bool(flags&1),movable=bool(flags&16)))
            entry['segments']=segments
        shutil.copyfile(f,runtime/name);binaries[name]=entry
    result=dict(pass_build=True,compiler='Open Watcom / 8086 small model',scope='App-local Buddy only; published SOUND/MIDI engine unchanged',inputs=inputs,binaries=binaries)
    (out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(binaries,indent=2))

if __name__=='__main__':main()

