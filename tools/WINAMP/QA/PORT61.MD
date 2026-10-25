# Port61 observer correction

The first unified runtime completed normal Windows return and owned-bridge
cleanup. Its only underlying failed assertion was PITUnrelatedBits: the
observer compared IN61 masked with FC before and120ms after PIT output.
The values were20h and10h. All other functional checks passed; AllPass and
TOTAL_FAILURES consequently reported that failed assertion. The original
receipt stays unchanged and is not promoted to a clean pass.

DOSBox-X keyboard.cpp read_p61 returns only port_61_data&0Fh as stored
readback. It synthesizes20h from timer2 output/gate and10h from elapsed
time. In the Tandy model it clears40h. Therefore an elapsed-time equality
check of IN61&FCh included changing input status rather than just stored
control state. The observer now compares IN61&0Ch; existing separate
assertions still check low-two gate bits after set/suspend/release/stop.
The unchanged driver retains its current-read/modify/write and masked
low-two-bit changes. No driver write was added to coerce readback.

This mask describes the tested emulator register implementation. It does
not redefine physical Tandy hardware. Tandy's1986 1000EX Technical Reference
Manual, main-logic p53, lists port61 bits0/1 as gate/speaker enable, bits2/3
unused, bits4/5/6 as sound routing/control and bit7 as keyboard clear.
The physical timer2 output is port62 bit5. Physical upper control readback
is not established by this DOSBox-X fixture; those physical controls remain
preserved by the driver's existing immediate RMW path and host I/O tests.

Primary sources:
- DOSBox-X read_p61/write_p61, keyboard.cpp lines689-710:
  https://raw.githubusercontent.com/joncampbell123/dosbox-x/master/src/hardware/keyboard.cpp
- Tandy1000EX Technical Reference Manual, main logic p53, port61/62 table:
  https://archive.org/stream/tandy1000extechnicalreferencemanual/Tandy_1000EX_Technical_Reference_Manual_djvu.txt

TESTP61.PY checks every pair of synthesized10h/20h status values under
each stored lower-nibble state and both desired gate states. It also proves
that changing either stored unrelated bit2/3 still fails the new comparison.
The next native rebuild/run must produce fresh source/binary/runtime pins.
