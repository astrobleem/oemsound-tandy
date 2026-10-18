# Tandy Winamp UX3 captures

These are actual unmodified 640x400 PNG captures from the completed native Windows 3.0 real-mode, stock-CGA, Tandy HX 8088 emulator qualification (4.772728 MHz, 640 KiB). No scaling, compositing, recoloring or retouching was applied. They are emulator captures, not physical-machine photographs.

WAMPUI3.PNG is PLUILS3 FRAME18: the seven-entry list is scrolled beyond four songs. WAMPOP3.PNG is PLUIPB3 FRAME11: the Add song browser is visible during an original test study. The directory names refer to original synthetic test material; private studies and commercial songs are not in the public app archive. The helper window named Playlist belongs to the independent QA driver.

MINIPLAY.EXE and LWFILE.DLL are the same runtime bytes accepted physically in UX3. See ../../experimental/WINAMP30/RUNTIME.JSON for their pins and ../../experimental/WINAMP30/VALIDATION-UI.MD for the 396-check UI record. The owner's separate Tandy EX 7.16 MHz report is recorded in ../../experimental/WINAMP30/EVIDENCE/PHYSICAL.JSON.

```json
[
  {
    "public_path": "docs/SHOTS/WAMPUI3.PNG",
    "case": "PLUILS3",
    "frame": "FRAME18.PNG",
    "bytes": 3373,
    "sha256": "bb6d0090ef03a55ad75cffa48eeda2bae57266097fdefe3ee66c36ef221d90d6",
    "modifications": "none"
  },
  {
    "public_path": "docs/SHOTS/WAMPOP3.PNG",
    "case": "PLUIPB3",
    "frame": "FRAME11.PNG",
    "bytes": 4432,
    "sha256": "9014655c0febafc59186125e9455ce5d33dfe8cfb31f80c2b8dc39ce8c3a732b",
    "modifications": "none"
  }
]
```
