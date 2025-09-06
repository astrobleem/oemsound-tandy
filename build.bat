@echo off
set PATH=C:\BIN;C:\WINDOWS;C:\DOS
set LIB=C:\LIB;C:\INCLUDE;C:\DDK\286\LIB
set INCLUDE=C:\INCLUDE;C:\DDK\286\INC
cl /nologo /c /O /AS src\PSGTEST.C > output.txt
echo. | link /NOI PSGTEST.OBJ,PSGTEST.EXE,,; > link.txt