from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent / "layout.c"
facade_source = source.parent / "facade.c"
if (root / "FACADE" / "LWAUDIO.H").read_bytes() != (root / "SOURCE" / "LWAUDIO.H").read_bytes():
    raise SystemExit("FACADE/LWAUDIO.H differs from SOURCE/LWAUDIO.H")
compiler = Path(r"C:\msys64\ucrt64\bin\cc.exe")
if not compiler.is_file():
    raise SystemExit("Required compiler not found: " + str(compiler))
with tempfile.TemporaryDirectory(prefix="audio-facade-layout-") as temp:
    exe = Path(temp) / "layout.exe"
    command = [
        str(compiler), "-x", "c", "-std=c89", "-pedantic", "-Wall",
        "-Wextra", "-Werror", "-fpack-struct=2",
        "-I", str(source.parent), "-I", str(root / "SOURCE"),
        str(source), "-o", str(exe),
    ]
    env = os.environ.copy()
    env["PATH"] = str(compiler.parent) + os.pathsep + env.get("PATH", "")
    subprocess.run(command, check=True, env=env, timeout=30)
    subprocess.run([str(exe)], check=True, env=env, timeout=30)
    facade_obj = Path(temp) / "facade.o"
    facade_command = [
        str(compiler), "-x", "c", "-std=c89", "-pedantic", "-Wall",
        "-Wextra", "-Werror", "-fpack-struct=2",
        "-I", str(facade_source.parent), "-I", str(root / "FACADE"),
        "-I", str(root / "SOURCE"), "-c", str(facade_source), "-o", str(facade_obj),
    ]
    subprocess.run(facade_command, check=True, env=env, timeout=30)
print("Win16 header layout fixture passed (host-only)")
