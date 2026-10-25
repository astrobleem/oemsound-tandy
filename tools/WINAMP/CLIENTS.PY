"""Pin the current native Windows XT clients for the one sound installer."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import zlib

SPECS = (('TSHELL.EXE','W','tshell'), ('TCHIME.EXE','X','chime'),
         ('XPCHIME.EXE','X','xpchime'), ('TEXIT.EXE','X','texit'),
         ('TP.EXE','X','tp'), ('FIXXT.EXE','X',None),
         ('TSINPUT.DLL','W','unchanged'))


def pin(data):
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('native','fixxt','display','workspace','out','header'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--client-suffix',required=True)
    p.add_argument('--tshell-build',type=Path)
    a=p.parse_args()
    if a.out.exists():p.error('Output must be new')
    entries={}; sources={}; table=[]
    a.out.mkdir(parents=True)
    for name,kind,folder in SPECS:
        if folder=='unchanged':
            path=a.display/'tools/XR1PACK/PAYLOAD'/name
            data=path.read_bytes()
            rel=path.relative_to(a.workspace).as_posix()
            entries[name]={'kind':kind,'runtime':pin(data),'crc32':'%08x'%zlib.crc32(data),
                'current_baseline':pin(data),'baseline_crc32':'%08x'%zlib.crc32(data),
                'sources':{rel:pin(data)},'qualification':'unchanged current Windows XT dependency'}
            (a.out/name).write_bytes(data)
            target=a.out/'SOURCE'/Path(name).stem/name
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(data)
            component=a.display/'examples/TSHELL/IDLE'
            for source in component.iterdir():
                if source.is_file() and source.suffix.upper() in ('.C','.H','.ASM','.DEF','.RSP','.BAT','.MD'):
                    entries[name]['sources'][source.relative_to(a.workspace).as_posix()]=pin(source.read_bytes())
                    (target.parent/source.name).write_bytes(source.read_bytes())
            table.append('    {"%s",\'%s\',%dUL,0x%08xUL,%dUL,0x%08xUL}'%
                (name,kind,len(data),zlib.crc32(data),len(data),zlib.crc32(data)))
            continue
        build=a.native/(folder+a.client_suffix) if folder else a.fixxt
        if name=='TSHELL.EXE' and a.tshell_build:
            build=a.tshell_build
        record=json.loads((build/'BUILD.JSON').read_bytes())
        data=(build/name).read_bytes()
        wanted=record['runtime'][name] if folder else record['runtime']
        if pin(data)!=wanted:raise ValueError('Native binary changed: '+name)
        process=json.loads((build/'PROCESS.JSON').read_bytes())
        if process.get('return')!=0 or not process.get('owned_child_stopped') or not process.get('lease_released'):
            raise ValueError('Native build did not complete: '+name)
        if folder:
            paths={n:a.workspace/n for n in record['source_pins']}
            pins=record['source_pins']
        else:
            paths={n:a.display/'tools/SOUNDOWN'/n for n in record['sources']}
            pins=record['sources']
        snapshot={}
        for n,path in paths.items():
            if pin(path.read_bytes())!=pins[n]:raise ValueError('Current client source changed: '+str(path))
            staged=build/path.name
            if pin(staged.read_bytes())!=pins[n]:raise ValueError('Staged client source changed')
            rel=path.relative_to(a.workspace).as_posix()
            snapshot[rel]=pins[n]
            target=a.out/'SOURCE'/Path(name).stem/path.name
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(staged,target)
        old=(a.display/'tools/XR1PACK/PAYLOAD'/name).read_bytes()
        entries[name]={'kind':kind,'runtime':pin(data),'crc32':'%08x'%zlib.crc32(data),
                       'current_baseline':pin(old),'baseline_crc32':'%08x'%zlib.crc32(old),
                       'sources':snapshot}
        (a.out/name).write_bytes(data)
        (a.out/(Path(name).stem+'.JSN')).write_text(json.dumps(record,indent=2)+'\n')
        table.append('    {"%s",\'%s\',%dUL,0x%08xUL,%dUL,0x%08xUL}'%
                     (name,kind,len(old),zlib.crc32(old),len(data),zlib.crc32(data)))
    (a.out/'PINS.JSN').write_text(json.dumps(entries,indent=2)+'\n')
    text=('/* Generated current Windows XT consumer identities. */\n'
          'static const struct XTClient xtclients[] = {\n'+',\n'.join(table)+
          '\n};\n#define XTCLIENT_COUNT %d\n'%len(table))
    a.header.write_bytes(text.replace('\n','\r\n').encode('ascii'))
    print('PASS six native clients plus pinned TSINPUT dependency; generated XTCLIENT.H')


if __name__=='__main__':main()
