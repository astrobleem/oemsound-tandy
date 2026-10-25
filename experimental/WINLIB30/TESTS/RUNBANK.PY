"""Exercise actual Windows bank APIs and live/file reset paths on host mocks."""
import argparse
from pathlib import Path
import hashlib,json,os,subprocess,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True);args=p.parse_args()
A=Path(__file__).resolve().parents[1];OUT=args.out.resolve();assert not OUT.exists();OUT.mkdir(parents=True)
CC=([sys.executable,r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py','--'] if os.name=='nt' else ['cc'])+['-x','c','-std=gnu99','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-unused-function','-Wno-misleading-indentation','-Wno-sign-compare','-O2','-DSYNTH_HOST','-DSMF_FAR=','-I',str(A/'TESTS/mdhost'),'-I',str(A/'SOURCE')]
log=[]
for name,test,synth in [('BANKMD','BANKMD.C',True),('BANKSC','BANKSC.C',False),('BANKLIVE','BANKLIVE.C',True),('DRIVER','mdhost/driver.c',True),('MDCTL','mdctl_host.c',True),('COREBANK','BANKTEST.C',True),('BTRACE','BTRACE.C',True)]:
    exe=OUT/(name+'.EXE');command=list(CC)
    if name in ('COREBANK','BTRACE'):command=[x.replace('-std=gnu99','-std=c89') for x in command]+['-pedantic-errors','-DWITH_BANK']
    command += [str(A/'TESTS'/test)]
    if synth:command += [str(A/'SOURCE/MIDISYN.C'),str(A/'SOURCE/INSTR.C')]
    command+=['-o',str(exe)]
    for cmd in (command,[str(exe)]):
        r=subprocess.run(cmd,capture_output=True,text=True)
        log.append({'test':name,'command':cmd,'return':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
        (OUT/'LOG.JSN').write_text(json.dumps(log,indent=2)+'\n');print(r.stdout,end='');print(r.stderr,end='',file=sys.stderr)
        if r.returncode:raise SystemExit(r.returncode)
(OUT/'RESULT.JSN').write_text(json.dumps({'status':'PASS_WINDOWS_BANK_API_LIVE_FILE_HOST_CHECKS','tests':[r['stdout'].strip() for r in log if len(r['command'])==1],'actual_source':True,'hardware':'mocked','native_qualified':False,'mandatory_GCC_wrapper':os.name=='nt','log_sha256':hashlib.sha256((OUT/'LOG.JSN').read_bytes()).hexdigest()},indent=2)+'\n')
print('PASS all actual Windows bank API and preserved MIDI/live host regressions')
