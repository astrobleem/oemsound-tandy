# Tested runtime manifest

No compiled binaries are committed. The source reconstructs this release:

- MINIPIAN.EXE: 15327 bytes
- SHA256: 22cab48dce6baba821a72689fcaee848b3bd3fe4e9b9da659f470047abe6b4fc
- Compiler: Microsoft C6, /G0 /Gw /AS /W3 /Od
- Linker: Microsoft LINK4
- Target: Windows 3.0 real mode, 8088-compatible Tandy EX/HX

Existing dependency, not duplicated or changed:

- MIDIMAP.DRV from ../BEATS: 3715 bytes
- SHA256: 4b960b2918561b077c5b1deb6bf8461ac36851b6023f226145c15d4d7bb5dd81
- App-local ABI: MIDIMESSAGE, version 0x0101, three melodic voices

Publication contains only C/H/DEF/RSP/BAT/PY source and MD documentation.
No EXE/OBJ/DRV, audio, images, raw logs, test guests, toolchains, local
receipts or machine paths are required for the proposed repository change.
Runtime tokens and behavior match the tested release. Source leading
indentation/CRLF and portable test-file selection are the only source-tool
presentation changes; see BUILD.MD for the line-grouping exception.
