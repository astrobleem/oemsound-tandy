@echo off
set PATH=C:\BIN;C:\WINDOWS;C:\DOS
set LIB=C:\LIB;C:\INCLUDE;C:\DDK\286\LIB
set INCLUDE=C:\INCLUDE;C:\DDK\286\INC
cl /nologo /c /O /AS src\psgtest.c > output.txt
echo. | link /NOI psgtest.obj,psgtest.exe,,; > link.txt
