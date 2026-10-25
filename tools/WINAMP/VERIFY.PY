"""Verify the finished native Setup package and its native/source receipts."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import zipfile
import zlib

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]


def ident(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def verify(files, current=True):
    require(all(n.startswith('TWXT2/') and '\\' not in n and
            '..' not in n.split('/') for n in files), 'Unsafe ZIP member')
    files = {n[6:]: d for n,d in files.items()}
    manifest = json.loads(files['MANIFEST.JSN'])
    require(set(manifest) == set(files)-{'MANIFEST.JSN'}, 'Manifest member set differs')
    for name,pin in manifest.items():
        require(ident(files[name]) == pin, 'Manifest identity differs: '+name)
    require([n for n in files if '/' not in n and n.endswith('.DRV')] == ['SOUND.DRV'],
            'Expected one consumer sound driver')
    builds = json.loads(files['BUILD.JSN'])['native_builds']
    for label,source,filename in (
            ('driver','src/win30/DRIVER','SOUND.DRV'),
            ('raw','experimental/WINLIB30/SOURCE','RAWCLK.DLL'),
            ('launch','src/win30/LAUNCH','TPLAY.EXE'),
            ('setup','src/win30/SETUP','SETUP.EXE')):
        record = builds[label]
        require(ident(files[filename]) == record['runtime'], 'Unlinked native binary')
        for name,pin in record['sources'].items():
            require(ident(files['SOURCE/'+label.upper()+'/'+name]) == pin,
                    'Unlinked native source: '+name)
            if current:
                path = REPO/source/name
                if label == 'setup' and name == 'SOUNDID.H':
                    path = REPO/'src/win30/DRIVER/SOUNDID.H'
                elif label == 'setup' and name == 'TPLAYPOL.H':
                    path = REPO/'src/win30/LAUNCH/TPLAYPOL.H'
                require(ident(path.read_bytes()) == pin, 'Current source differs: '+str(path))
    sound = json.loads(files['BUILD.JSN'])['canonical_sound']
    require(ident(files['SOUND.DRV']) == {k:sound[k] for k in ('bytes','sha256')},
            'Canonical sound identity differs')
    require('%08x'%zlib.crc32(files['SOUND.DRV']) == sound['crc32'], 'Sound CRC differs')
    for label in ('LAUNCH','SETUP'):
        require(files['SOURCE/'+label+'/SOUNDID.H'] ==
                (REPO/'src/win30/DRIVER/SOUNDID.H').read_bytes(),
                'Consumers use another sound expectation')
    tests = json.loads(files['TESTS.JSN'])
    require(tests.get('status') == 'PASS', 'Incomplete native acceptance')
    require(tests.get('release_readiness', {}).get('status') == 'READY',
            'Current published baseline reconciliation is incomplete')
    for key,name in (('sound','SOUND.DRV'),('rawclk','RAWCLK.DLL'),
                     ('setup','SETUP.EXE'),('launcher','TPLAY.EXE')):
        require(tests[key] == ident(files[name]), 'Native tests cover another '+name)
    rows = files['PAYLOAD.LST'].decode('ascii').splitlines()
    require(rows.pop(0) == 'TWSETUP1', 'Wrong native Setup manifest')
    selected = set()
    for row in rows:
        kind,src,dst,size,crc = row.split('|')
        require(src == dst and src not in selected and '/' not in src, 'Unsafe/duplicate payload')
        clients=json.loads(files['EVIDENCE/CLIENTS.JSN'])
        wanted=clients[src]['kind'] if src in clients else ('S' if src=='SOUND.DRV' else 'A')
        require(kind == wanted, 'Wrong destination kind')
        require(len(files[src]) == int(size) and '%08x'%zlib.crc32(files[src]) == crc,
                'Native Setup payload identity differs')
        selected.add(src)
    required = {'SETUP.EXE','SOUND.DRV','TPLAY.EXE','RAWCLK.DLL','MINIPLAY.EXE',
                'MIDICLK.DLL','MCITNDY.DLL','LWFILE.DLL','TBRIDGE.EXE','RESERVE.COM'}
    require(required <= selected, 'Missing required installer payload')
    clients=json.loads(files['EVIDENCE/CLIENTS.JSN'])
    require(len(clients)==7, 'Missing current Windows XT consumer/repair files')
    require(tests['clients']=={n:r['runtime'] for n,r in clients.items()}, 'Unlinked client tests')
    for name,row in clients.items():
        require(ident(files[name])==row['runtime'], 'Unlinked client binary')
        for path,pin in row['sources'].items():
            source='SOURCE/CLIENTS/'+Path(name).stem+'/'+Path(path).name
            require(ident(files[source])==pin, 'Unlinked client source')
            parts=Path(path).parts
            require('..' not in parts, 'Unsafe current client source path')
            if current and parts[:2] == ('unified','oemsound-tandy'):
                require(ident((REPO/Path(*parts[2:])).read_bytes())==pin,
                        'Current client source differs: '+path)
    if current:
        for n in ('INSTALL.TXT','README.TXT'):
            require(files[n] == (HERE/n).read_bytes(), 'Current guide differs')
    return len(files)


def verify_sfx(data, files):
    tests=json.loads(files['TWXT2/TESTS.JSN'])
    require(ident(data)==tests.get('installer'),
            'Native tests cover another self-contained installer')
    require(len(data)>=32, 'Truncated native installer')
    magic,body,size,count,tablecrc,crc,reserved=struct.unpack_from('<8s6I',data,len(data)-32)
    require(magic==b'TWINSFX1' and reserved==0 and 1<=count<=64, 'Invalid native footer')
    require(body+size==len(data)-32 and size<=4*1024*1024, 'Invalid native overlay bounds')
    require(data[:body]==files['TWXT2/SETUP.EXE'], 'Native extractor differs from tested Setup')
    bundle=data[body:-32];table=bundle[:count*32]
    require(zlib.crc32(table)==tablecrc and zlib.crc32(bundle)==crc, 'Native overlay checksum')
    expected={'PAYLOAD.LST':files['TWXT2/PAYLOAD.LST']}
    for row in expected['PAYLOAD.LST'].decode('ascii').splitlines()[1:]:
        name=row.split('|')[1];expected[name]=files['TWXT2/'+name]
    names=set();offset=count*32
    for i in range(count):
        raw,at,n,want,zero=struct.unpack_from('<13s3x4I',bundle,i*32)
        name=raw.split(b'\0',1)[0].decode('ascii')
        require(name in expected and name not in names and at==offset and zero==0,
                'Unexpected native overlay entry')
        value=bundle[at:at+n]
        require(value==expected[name] and zlib.crc32(value)==want, 'Native overlay payload differs')
        names.add(name);offset+=n
    require(names==set(expected) and offset==len(bundle), 'Native overlay member set differs')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',type=Path,default=REPO/'artifacts/win30/TWXT2.ZIP')
    p.add_argument('--installer',type=Path,default=REPO/'artifacts/win30/TWSETUP.EXE')
    p.add_argument('--self-test',action='store_true')
    a = p.parse_args()
    release = json.loads((HERE/'RELEASE.JSN').read_bytes())
    pin=release['package']
    require(ident(a.package.read_bytes()) == pin, 'Published archive pin differs')
    with zipfile.ZipFile(a.package) as z:
        names = z.namelist()
        require(len(names) == len(set(names)), 'Duplicate ZIP members')
        files = {n:z.read(n) for n in names}
    count = verify(files)
    exe=a.installer.read_bytes()
    require(ident(exe)==release['installer'], 'Native installer archive pin differs')
    verify_sfx(exe,files)
    from importlib.machinery import SourceFileLoader
    pit = SourceFileLoader('current_pit_packets',str(HERE/'PITPACK.PY')).load_module()
    for kit in ('MML3AUTH','PITEXPR'):
        path=a.package.parent/(kit+'.ZIP')
        require(ident(path.read_bytes())==release['assets'][path.name],
                'Instrument packet archive pin differs: '+kit)
        pit.verify(path,kit)
        with zipfile.ZipFile(path) as archive:
            require(archive.read(kit+'/TWSETUP.EXE')==exe,
                    'Instrument packet uses another installer')
            require(archive.read(kit+'/TWXT2.ZIP')==a.package.read_bytes(),
                    'Instrument packet uses another shared driver package')
    print('PASS both preserved instrument packets use the same tested installer')
    print('PASS native Setup package, %d identities, exact native/current source pins'%count)
    if a.self_test:
        changes = (('SOUND.DRV',b'wrong'),('RESERVE.COM',b'wrong'),
                   ('SETUP.EXE',b'wrong'),('PAYLOAD.LST',b'wrong'),
                   ('SOURCE/DRIVER/TSOUND.C',b'wrong'))
        for name,data in changes:
            broken = dict(files);broken['TWXT2/'+name]=data
            try:
                verify(broken)
            except (ValueError,KeyError):
                continue
            raise ValueError('Corrupt package accepted: '+name)
        print('PASS five corrupt-package refusals')
        for wrong in (exe[:-1],exe[:-32]+b'broken'+exe[-26:],exe[:len(exe)//2]):
            try:
                verify_sfx(wrong,files)
            except (ValueError,KeyError,struct.error):
                continue
            raise ValueError('Corrupt native installer accepted')
        print('PASS three corrupt-native-installer refusals')


if __name__ == '__main__':
    main()
