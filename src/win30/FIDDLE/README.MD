# Current Fiddle companion for DWA Beats

This maintained copy accompanies `../BEATLAB`, which owns the coordinated
native build and packet. Frozen `experimental/PITPUBLIC` source remains a
record of the previous shipped clients. Do not edit it as a second owner.

The bounded change admits MML3 P-part notes 26..96 (octave 1 allowed), with
low-P metadata validated before playback. Other melodic parts retain 45..96;
noise remains 35..81. Compatible mapper 1.x from 1.2 is accepted, but sounding
P notes below 45 require 1.3 before acquiring sound. The paired app-local
mapper is compiled from `BEATLAB/MIDIMAP.C`; both app folders receive the
same bytes. The single shared SOUND driver/service is unchanged.

Use `../BEATLAB/BUILD.PY` with the external MSC6/MASM/LINK4 toolchain and
normal shared emulator lease, not a separate unguarded builder. Matching
MIDIAPI.H copies are checked before staging. `../BEATLAB/HOSTTEST.PY`
tests this parser and the actual mapper, including exported low P notes.
`MMLFIDL /lowtest` exercises the preceding `BEATS /test` export at
C:\BEATS\NATIVE.MML in a disposable native fixture; ordinary users load
exported .MML through Fiddle's existing Open UI. See BEATLAB/VALIDATE.MD.
