"""Native launcher regressions in a disposable copy of an owned Windows profile."""
from pathlib import Path
import argparse
import json
import shutil
from RUNDOS import run


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('windows', 'packet', 'dosbox', 'lease', 'out'):
        p.add_argument('--' + name, type=Path, required=True)
    a = p.parse_args()
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    c = out / 'CROOT'
    shutil.copytree(a.packet, c / 'TWXT1')
    (c / 'CASES').mkdir()
    boot = ('[boot]\nsystem.drv=SYSTEM.DRV\nsound.drv=SOUND.DRV\n'
            'display.drv=HPXY.DRV\n')
    commands = ['@ECHO OFF']
    cases = []

    def case(name, ini=boot, mutate=None, accepted=False):
        d = c / 'CASES' / name
        (d / 'SYSTEM').mkdir(parents=True)
        for n in ('SYSTEM/SYSTEM.DRV', 'SYSTEM/SOUND.DRV', 'WIN.COM'):
            shutil.copyfile(a.windows / n, d / n)
        (d / 'SYSTEM.INI').write_text(ini)
        if mutate:
            mutate(d)
        cases.append({'name': name, 'expected': 0 if accepted else 1})
        commands.extend([
            f'C:\\TWXT1\\TPLAY C:\\CASES\\{name} /CHECK > {name}.TXT',
            f'IF ERRORLEVEL 1 GOTO E{name}', f'ECHO 0 > {name}.RET',
            f'GOTO N{name}', f':E{name}', f'ECHO 1 > {name}.RET', f':N{name}'])

    names = ('HPXY.DRV', 'hpxy.drv', 'CGA.DRV', 'TR51616.DRV',
             'TR53216.DRV', 'TR53204.DRV', 'TR56402.DRV', 'TR56404.DRV',
             'TANDY88.DRV', 'TNDY160.DRV', 'TNDY3204.DRV',
             'TNDY6402.DRV', 'TNDY6404.DRV', 'XRSYNC.DRV', 'xrsync.drv')
    for i, name in enumerate(names):
        case('P%02u' % i, boot.replace('HPXY.DRV', name), accepted=True)
    for i, name in enumerate(('VGA.DRV', 'OTHER.DRV', 'TXTMODE.DRV', 'HPXY.DR_', 'XRSYNC2.DRV')):
        case('N%02u' % i, boot.replace('HPXY.DRV', name))
    case('DUPDISP', boot + 'display.drv=CGA.DRV\n')
    case('DUPSND', boot + 'sound.drv=SOUND.DRV\n')
    case('WRONGSND', boot.replace('SOUND.DRV', 'TSOUND.DRV'))
    case('WRONGSYS', boot.replace('SYSTEM.DRV', 'TANDYSYS.DRV'))
    case('BADTSND', mutate=lambda d: (d/'SYSTEM/SOUND.DRV').write_bytes(b'wrong'))
    case('BADSYS', mutate=lambda d: (d/'SYSTEM/SYSTEM.DRV').write_bytes(b'wrong'))
    case('BADWIN', mutate=lambda d: (d/'WIN.COM').write_bytes(b'wrong'))
    case('ROOTSND', mutate=lambda d: shutil.copyfile(d/'SYSTEM/SOUND.DRV', d/'SOUND.DRV'))
    commands += ['REN C:\\TWXT1\\RESERVE.COM RESERVE.SAV',
                 'C:\\TWXT1\\TPLAY C:\\CASES\\P00 /CHECK > NOHELP.TXT',
                 'IF ERRORLEVEL 1 ECHO PASS > NOHELP.RET',
                 'ECHO wrong > C:\\TWXT1\\RESERVE.COM',
                 'C:\\TWXT1\\TPLAY C:\\CASES\\P00 /CHECK > BADHELP.TXT',
                 'IF ERRORLEVEL 1 ECHO PASS > BADHELP.RET',
                 'DEL C:\\TWXT1\\RESERVE.COM',
                 'REN C:\\TWXT1\\RESERVE.SAV RESERVE.COM',
                 'C:\\TWXT1\\TBRIDGE > OWNER.TXT',
                 'C:\\TWXT1\\TPLAY C:\\CASES\\P00 /CHECK > OWNED.TXT',
                 'IF ERRORLEVEL 1 ECHO PASS > OWNED.RET',
                 'C:\\TWXT1\\TBRIDGE /U > UNOWNER.TXT', 'EXIT']
    (c / 'TEST.BAT').write_bytes(('\r\n'.join(commands)+'\r\n').encode())
    config = f'''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
allow more than 640kb base memory=false
memsize=1
working directory option=force
working directory default={out}
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 200000
fpu=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound=true
[midi]
mididevice=none
[autoexec]
@echo off
mount C "{c}"
C:
TEST.BAT
exit
'''
    (out / 'RUN.CNF').write_text(config)
    run(a.dosbox, out/'RUN.CNF', a.lease, 60)
    for case_result in cases:
        n = case_result['name']
        case_result['actual'] = int((c/(n+'.RET')).read_text())
        case_result['output'] = (c/(n+'.TXT')).read_text()
        assert case_result['actual'] == case_result['expected'], case_result
    for name in ('NOHELP', 'BADHELP', 'OWNED'):
        assert (c/(name+'.RET')).read_text().strip() == 'PASS'
    result = {'passed': len(cases)+3, 'cases': cases}
    (out/'RESULTS.JSON').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS', result['passed'], 'native launcher cases')


if __name__ == '__main__':
    main()
