# Tandy Winamp UI preview

This source preview adds a first-party Win3.0 file browser, a bounded 32-song native scrolling playlist, Remove, and the stable `Tandy Winamp` caption. Removing the active row preserves playback and its source file. See [behavior](WINAMP-UI.MD), [building](BUILDING-UI.MD), and [validation](VALIDATION-UI.MD).

The complete format player comes from the published [libwintandy format preview](https://github.com/astrobleem/libwintandy/tree/24ff4f932c0d4d4e2a18a3c49b0ea0a8ff90e71f/examples/FORMATS30). Its eight parser/file/format source units remain byte-identical. Audio clock, MCI and bridge binaries remain pinned dependencies. The application-local browser keeps the first-party implementation and ABI; its library name is LWFILE. Notices are retained.

Validation passes 149 portable C89 checks, 396 native UI checks across three bounded Win3.0 real-mode emulator runs, and the independent 88-check gzip/PSG event regression. Visible-modal playback progress, active-row removal, Stop, clean DOS return and ownership release pass. The PSG hash/count remain exact. Receipts and measured allocation costs are in EVIDENCE.

The source includes original fixture generators and build recipes. A private local binary/test pack accompanies this preview. Physical acceptance of the new UI remains pending; this draft is not a release candidate. Proprietary Windows/ROM/toolchain files and third-party media are not included.
