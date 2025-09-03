@echo off
rem Build psgtest.exe with Microsoft C 6.0
rem Assumes your C600 BIN is on PATH (as in your AUTOEXEC.BAT)
cl /O /AS /Fepsgtest.exe src\psgtest.c > output.txt
link /NOI /TINY src\psgtest.obj, psgtest.exe,,;    >link.txt
