@echo off
rem Open Watcom must be on PATH. Run from SOURCE.
wcc -0 -ms -os -s -w3 -ecc -i=..\libtandy\include DOSPLAY.C
if errorlevel 1 goto fail
wcc -0 -ms -os -s -w3 -ecc -i=..\libtandy\include AUDIO.C
if errorlevel 1 goto fail
wcc -0 -ms -os -s -w3 -ecc -i=..\libtandy\include ..\libtandy\src\lt_tick_dos.c
if errorlevel 1 goto fail
wcc -0 -ms -os -s -w3 -ecc -i=..\libtandy\include ..\libtandy\src\lt_chip.c
if errorlevel 1 goto fail
wasm -0 -ms HARDWARE.ASM
if errorlevel 1 goto fail
wlink system dos name DOSPLAY.EXE option stack=4096 option map file DOSPLAY.obj,AUDIO.obj,lt_tick_dos.obj,lt_chip.obj,HARDWARE.obj
if errorlevel 1 goto fail
echo PASS
goto end
:fail
echo FAIL
:end
