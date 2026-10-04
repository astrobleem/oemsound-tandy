import os, subprocess
from pathlib import Path
p=Path(__file__).resolve().parent
subprocess.run(['gcc','-x','c','-DSMF_HOST','-std=c89','-pedantic-errors','-Wall','-Wextra','-Wconversion','-Werror','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','SMF.C','FUZZTEST.C','-o','fuzztest'],cwd=p,check=True)
e=os.environ.copy();e['ASAN_OPTIONS']='detect_leaks=0:halt_on_error=1';e['UBSAN_OPTIONS']='halt_on_error=1'
subprocess.run([str(p/'fuzztest'),'fixtures/TYPE1.MID'],cwd=p,env=e,check=True)
