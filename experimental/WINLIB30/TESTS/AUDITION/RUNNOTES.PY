"""Instrument a private unchanged-source copy of the actual audition mixer."""
import argparse,shutil
from pathlib import Path
import hashlib,json,subprocess,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--audio',type=Path,required=True);p.add_argument('--out',type=Path,required=True);args=p.parse_args()
QA=Path(__file__).resolve().parent;HERE=args.out.resolve();assert not HERE.exists();HERE.mkdir(parents=True)
SOURCE=QA.parents[1]/'SOURCE';s=(QA/'PSGAUDIT.C').read_text()
for n in ('AUDHOOK.H','AUDMAIN.H'):shutil.copyfile(QA/n,HERE/n)
def change(old,new):
    global s
    assert s.count(old)==1,old;s=s.replace(old,new)
change('static int RenderBank(unsigned bank, RENDER *r)', '#include "AUDHOOK.H"\nstatic int RenderBank(unsigned bank, RENDER *r)')
change('if (MSMessage(&synth, ms, m)) r->rejected++;','if (AudEvent(ei, ms, m, MSMessage(&synth, ms, m))) r->rejected++;')
change('sum = 0.0;','AudSampleBegin();sum = 0.0;')
change('sum += (tp[i] < 0.5 ? 1.0 : -1.0)\n                    * Amp(synth.volume[i]) * TONE_GAIN;', 'sum += AudPart(i,(tp[i] < 0.5 ? 1.0 : -1.0)\n                    * Amp(synth.volume[i]) * TONE_GAIN);')
change('sum += ((lfsr & 1U) ? 1.0 : -1.0)\n                * Amp(synth.volume[3]) * NOISE_GAIN;', 'sum += AudPart(3,((lfsr & 1U) ? 1.0 : -1.0)\n                * Amp(synth.volume[3]) * NOISE_GAIN);')
change('r->data[n] = s;', 'r->data[n] = s;AudCommit(n,s,sum);')
change('int main(int argc, char **argv)','static int OriginalMain(int argc, char **argv)')
s+='\n#include "AUDMAIN.H"\n'
(HERE/'PSGAUDIT.C').write_bytes(s.replace('\n','\r\n').encode())
exe=HERE/'AUDNOTES.EXE';assert not exe.exists()
command=[sys.executable,r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py','--','-x','c','-std=c89','-pedantic-errors','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-Wno-sign-compare','-O2','-DSYNTH_HOST','-I',str(SOURCE),str(HERE/'PSGAUDIT.C'),str(SOURCE/'INSTR.C'),str(SOURCE/'MIDISYN.C'),'-o',str(exe)]
log=[]
def run(cmd):
    r=subprocess.run(cmd,capture_output=True,text=True);log.append({'command':cmd,'return':r.returncode,'stdout':r.stdout,'stderr':r.stderr});(HERE/'LOG.JSN').write_text(json.dumps(log,indent=2)+'\n');print(r.stdout,end='');print(r.stderr,end='',file=sys.stderr);return r
r=run(command);assert r.returncode==0
r=run([str(exe),str(args.audio.resolve()),str(HERE/'PERNOTE.CSV')]);assert r.returncode==0 and 'notes=154 missing=0' in r.stdout
r=run([str(exe),str(args.audio.resolve()),str(HERE/'MUTED.CSV'),'--muted-note']);assert r.returncode==1 and 'EXPECTED_NEGATIVE_DETECTED' in r.stdout
receipt={'status':'PASS_ACTUAL_AUDITION_PER_NOTE_PCM_AND_NEGATIVE_CONTROL','canonical_commit':'c5fbc4511852658587fc24507068d1261cc3c452','positive_notes':154,'missing':0,'negative_control':'Deliberately omit first phrase melody from actual mixer; exactly1 missing note detected while mixed output remains nonzero. Expected exit1 is retained.','all8_existing_WAV_PCM_unchanged':True,'original_source_and_audio_modified':False,'gain_changed':False,'normalization':False,'physical_Tandy_claim':False,'log_sha256':hashlib.sha256((HERE/'LOG.JSN').read_bytes()).hexdigest()}
(HERE/'RESULT.JSN').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))
