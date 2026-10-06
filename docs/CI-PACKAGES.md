# Verified sound download packages

GitHub Actions creates downloadable packages from existing qualified artifacts.
**This is packaging, not a clean native source rebuild.** CI performs fresh hash,
inventory, exclusion and package regression checks. Build logs, ISA audits and
emulator results inside the kits remain historical evidence. Physical Tandy
audio, timing and joystick acceptance remain separate.

## Downloads

On a PR, main push, or manual run, open the successful **Verify sound download
packages** run and download its `sound-packages-<exact commit>` artifact. Extract
that Actions wrapper first, then select one of these independent ZIPs:

| Asset | Scope |
| --- | --- |
| `PSGBASE.ZIP` | Unchanged `artifacts/win30/MILESTONE.ZIP`: PSGPLAY, TCHIME and the restricted TSOUND experiment, source and original evidence. |
| `BEATS.ZIP` | Unchanged `artifacts/win30/BEATS/MILESTONE.ZIP`: baseline-source Beats/MIDIMAP preview, source and original evidence. |
| `WININST12.ZIP` | Unchanged experimental expressive Windows instruments kit. |
| `BUDENV02.ZIP` | Unchanged experimental media-free DOS Buddy overlay; requires the separately obtained exact BUDCAP04 baseline. |
| `SOUNDP01.ZIP` | Ownership checkpoint: source and historical evidence only; no runtime or binary fixtures. |
| `SOUNDP02.ZIP` | Interface checkpoint: source and historical evidence only; no runtime or binary fixtures. |

The earlier baseline-source milestones are themselves bounded experimental
previews, not a claim of stable or hardware-qualified drivers. Keep all kits in
separate folders and read their original guides. App-local MIDIMAP.DRV stays
beside its matching clients, outside WINDOWS/SYSTEM. CI makes no SYSTEM.INI,
startup, CF, user installation or default-runtime changes.

The source-only ZIPs include `SCOPE.TXT` and `FILES.JSON` describing the selected
publication. Their preserved original `MANIFEST.JSON` files describe complete
historical kits, including deliberately omitted runtimes and fixtures. These
ZIPs cannot run native QA without matching original fixtures and lawful tools.

`BUILD.JSON` records the exact checkout commit, package kind, sizes and SHA-256
of every archive and member, and explicit `native_compilation: false` and
`fresh_native_qa: false`. `SHA256SUMS` covers every asset except itself. Original
archive bytes are unchanged; the source-only ZIPs use fixed metadata and stored
members for identical output on Windows and Linux.

## Source-build prerequisites inspected

At main commit `7b994c5d0072a69840767f1898aecbaea675dc7f`, component `BUILD.BAT`
and `BUILD.PY` scripts require Microsoft C, the period Windows linker and
Windows libraries in a DOS environment. The repository contains older `BIN`,
`INCLUDE`, `LIB`, `DDK` and `DOS` directories, but **the required
`BIN/LINK4.EXE`, `LIB/SLIBCEW.LIB` and `LIB/LIBW.LIB` are absent**. The Windows
response files reference those libraries explicitly. Root `BUILD.BAT` builds
only an earlier DOS test, not all these downloads.

Open Watcom was found on the development host, but these Microsoft-specific
component builds have no validated Watcom port. A successful unrelated DOS test
would not establish a rebuild of the Windows instruments. Hosted CI therefore
does not run the legacy tools or fetch compilers, SDKs or OS images from mirrors.

A future native build needs lawful matching Microsoft C/linker/Windows
headers/libraries, a DOSBox-X build environment, clean per-component output,
native binary verification and matching QA fixtures/guest files. It also needs
an authorized way to provision those licensed inputs. This workflow adds no
credentials, licensed-tool uploads or self-hosted runner registration.

## Draft prereleases

Only an explicit new tag matching `sound-vMAJOR.MINOR.PATCH-prerelease` can
create a release. For example, choose an unused `sound-v0.0.0-ci.YYYYMMDD.N`
tag at the reviewed exact commit and push it. Once the workflow is on main,
you can also run it manually at that same commit with an existing matching
`release_tag`; a blank input creates only Actions artifacts.

The tag must already exist and resolve to the exact event commit. The release
job checks this again, includes drafts when checking for existing releases,
and refuses all updates or overwrites. It creates a **draft, prerelease,
non-latest** release only. It never publishes, promotes or replaces a stable
release. New release tags are never forced or reused.

PR CI and ordinary pushes have `contents: read`. Only the gated release job
has `contents: write`; it uses a fresh runner, never checks out or executes
repository/artifact code, and independently verifies a fixed set of six pinned
ZIP hashes plus the complete nine-asset allowlist and provenance. Actions are
pinned to full commit IDs. The package job sparsely checks out only the release
scripts and allowed artifact/source directories, leaving proprietary tool/OS
directories out of its working tree. No recursive repository archive is uploaded.

## Local verification and maintenance

Use Python 3.10 or later and a checkout preserving exact Git bytes
(`core.autocrlf=false`; do not normalize existing qualified source files):

```text
python tools/RELEASE/TESTPKG.PY
python tools/RELEASE/PACK.PY --out NEW_OUTPUT --source-sha EXACT_40_CHARACTER_COMMIT
python tools/RELEASE/PACK.PY --verify --out NEW_OUTPUT --source-sha EXACT_40_CHARACTER_COMMIT
```

Tests cover deterministic bytes, unchanged original archives, source inventories,
wrong provenance, corruption, extra output assets, unsafe paths, duplicate ZIP
members, symlinks and unqualified runtime files. Package verification additionally
checks original baseline manifests, experimental internal checksum lists,
all source-publication hashes/Git blob IDs
and an exact runtime allowlist. It rejects compiler/SDK/OS directories and
library/object/disk/nested-archive file types.

`tools/RELEASE/INPUTS.JSON` pins the reviewed archives and runtime identities.
Updating inputs requires deliberate review of provenance, licenses and exclusions,
then updating both that inventory and the release job's independent ZIP pins.
Do not simply regenerate pins to accept an unexplained hash mismatch. Existing
component licenses and third-party arrangement attribution remain authoritative.
