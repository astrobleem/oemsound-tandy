COMMDLG30 0.1 - bounded Win16 file dialogs for Windows 3.0 real mode
================================================================

This is an ORIGINAL 8086 implementation of four historical COMMDLG APIs.
It is a deliberately small compatibility subset, not the complete Windows
3.1 common-dialog library and not COMDLG32. Do not replace a working
Windows 3.1 COMMDLG.DLL with it.

APIs and ABI
------------
COMMDLG.DLL is a Windows 3.0-marked NE library. It exports the historical
uppercase names and ordinals used by the existing Microsoft Win16 import
library:
  1   GetOpenFileName       BOOL FAR PASCAL (OPENFILENAME FAR *)
  2   GetSaveFileName       BOOL FAR PASCAL (OPENFILENAME FAR *)
  26  CommDlgExtendedError  DWORD FAR PASCAL (void)
  27  GetFileTitle          int FAR PASCAL (LPCSTR, LPSTR, UINT)

Win16 handles, BOOL and int are 16 bits; pointers are segmented FAR
pointers; DWORD is 32 bits. OPENFILENAME is the historical 72-byte Win16
structure, not the modern Win32 structure. The independent caller checks
its size and offsets and links the original COMMDLG.LIB, not a substitute
import library generated from this DLL. WEP and two internal dialog
callbacks are implementation exports, not additional compatibility APIs.

Supported subset
----------------
- One local ASCII DOS 8.3 file at a time, with full-path output.
- Drive and directory browsing, typed paths, initial directory/file,
  custom caption, and a default extension of up to three characters.
- Up to 16 named filters. Labels may have 39 characters; masks may have
  63. Semicolon-separated masks, '*' and '?' are supported. Filter indexes
  are one-based; zero/out-of-range input selects the first normal filter.
- A compact native EDIT/LISTBOX/COMBOBOX/BUTTON layout at 160 pixels, and
  wider layouts at 320 and 640 pixels. Error and confirmation dialogs also
  fit 160 pixels. Native controls provide Tab, Enter, Escape and button mnemonics.
  Final manual keyboard/mouse verification is not yet completed.
- READONLY, OVERWRITEPROMPT, HIDEREADONLY, NOCHANGEDIR, PATHMUSTEXIST,
  FILEMUSTEXIST, CREATEPROMPT and NOREADONLYRETURN. EXTENSIONDIFFERENT is
  updated as an output flag. An empty default extension is treated like a
  NULL default in this subset; extensionless output clears that flag.
  Overwrite/create questions default to No. CREATEPROMPT validates the
  parent directory and can permit creation even with FILEMUSTEXIST.
- Success returns TRUE and error zero. Cancel returns FALSE and error zero.
  Required selected-file capacity includes NUL. A short file buffer returns
  FNERR_BUFFERTOOSMALL; when capacity is at least two bytes, its first WORD
  receives the required byte count. No write exceeds the declared capacity.
- Optional file-title output is NUL-terminated and safely truncated to its
  declared capacity. nFileOffset refers to the full path. With no extension
  and no default, nFileExtension points at output NUL; a user-typed final
  period suppresses the default extension and returns extension offset 0.
- GetFileTitle returns 0 on success, a positive NUL-inclusive required size
  for a short destination, or -1 for an invalid bounded DOS filename. A
  successful call clears this task's extended error, as the 1992 SDK says.
- NOCHANGEDIR restores the original drive and every changed drive's working
  directory on success, Cancel and failure. Without it, browsing and initial
  directory selection can change the current directory.
- A new Save selection probes directory writability with an exclusively
  created CDxxxx.TMP file, then attempts to close and delete it. Open with
  NOREADONLYRETURN also verifies file/directory writability. Failed temporary
  cleanup is retried before return; persistent failure reports
  CDERR_INITIALIZATION and names the temporary file. An unrecoverable I/O
  failure can leave that file behind; remove only that identified file after
  closing the application. Fault-injected close/delete failures are not
  covered by the native suite.
  The caller remains responsible for creating/writing the selected file.
  Existing files are opened for a write-access check, never truncated.

Explicit boundaries
-------------------
No hooks, templates, custom-filter buffers, multiselect, help callbacks,
UNC paths, long names, non-ASCII filename conversion, network UI, sharing
callbacks or other common dialogs. Requests for unsupported flags or a
custom-filter buffer fail with CDERR_INITIALIZATION (2); there are no
pretend-success stubs. An app importing any unimplemented common-dialog
entry point will not load against this subset.

Paths and input filenames are bounded to 127 characters plus NUL; captions
to 63 plus NUL. Caller pointers must point to valid Win16 memory; declared
buffers must stay within a segment. DOS device names are rejected. The
error table holds 32 task-handle identities while this DLL remains loaded;
new file-dialog requests fail closed on exhaustion. Two caller tasks and their separate error states
are tested; simultaneous interactive dialogs in many tasks are not a
separate compatibility certification.

Try it without changing Windows
------------------------------
1. Back up your normal Windows installation. Keep any existing COMMDLG.DLL.
2. Make a NEW C:\CD30 directory and copy the files from BIN into it.
3. On the tested Tandy installation, exit Windows and run C:\CD30\TEST.BAT.
   It checks for the existing C:\TANDY88\RESERVE.COM helper, reserves video
   memory, and starts C:\WINDOWS\WIN.COM /R with the test app. It aborts if
   these prerequisites are missing. It does not edit SYSTEM.INI or replace
   drivers, launchers, sound components, or system DLLs.
4. Use the Test menu: Save round-trip, Read round-trip, Open, and Cancel.
   Results are written to CDTEST.LOG next to the test executable. The
   round-trip actions write only the file you select.

The DLL should remain application-local. For another otherwise-compatible
8086 Win16 app, test an isolated copy with COMMDLG.DLL in its working
folder. That app must use only supported APIs/flags and be Windows 3.0-
marked; an unmarked Windows 2.x-style executable can use old module lookup
and fail to find a DLL despite the file being present. This library cannot
make a protected-mode, 286/386, or Win32 application run on an 8088.

Rollback: exit Windows and remove the new C:\CD30 test directory (preserve
any documents you saved there first). No system configuration rollback is
needed because the test installation changes none. If you experimented
with another application's folder, restore its original DLL from your
backup before restarting that app.

Build and verify
----------------
No compiler, import library, Windows media, or Microsoft COMMDLG source is
included. Use your existing Microsoft C/MASM/Windows SDK tool tree.
See BUILDING.TXT. Source and evidence are included, with full logs rather
than a claim that the four functions replace the entire Microsoft DLL.

Verification limits
-------------------
Tests use Windows 3.0 real mode from owned media in DOSBox-X, normal 640 KB,
enforced 8086_prefetch semantics, and no EMS/XMS/UMB. This is emulator
validation, not physical Tandy hardware validation. No pre-existing third-
party application has been certified; compatibility is demonstrated with
an independently written caller using the historical import library.
See EVIDENCE/SUMMARY.JSON for the final per-profile results and hashes.

Primary ABI references (not redistributed)
-----------------------------------------
Microsoft Windows 3.1 SDK, Programmer's Reference Volume 2 (April 1992):
OPENFILENAME at printed p.413; GetFileTitle at pp.373-374;
CommDlgExtendedError at printed p.64.
https://www.bitsavers.org/pdf/microsoft/windows_3.1/Microsoft_Windows_3.1_SDK_1992/PC28916-0492_Programmers_Reference_Volume_2_199204.pdf

Independent Win16 header cross-check:
https://raw.githubusercontent.com/open-watcom/open-watcom-v2/master/bld/w16api/include/commdlg.mh

The SDK NEWEXE.INC defines ne_expver at NE+0x3e. The resource-free builder
sets that field to 0x0300, the Windows 3.0 marking normally supplied by RC
/30. Native dialogs are generated in memory; no resource compiler is
required. This metadata is in addition to, not a substitute for, /G0
instruction generation and real-mode runtime tests.
