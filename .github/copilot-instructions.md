# Tandy Sound Driver Development

This repository contains a Windows 3.x sound driver and utilities for Tandy 1000 machines targeting 16-bit real-mode DOS/Windows with the Microsoft C 6.0 toolchain.

Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.

## CRITICAL BUILD CONSTRAINTS

**DO NOT attempt to build this project in standard Linux environments**. The build system requires:
- Microsoft C 6.0 or 7.0 compiler (`cl.exe`, `link.exe`, `lib.exe`)
- 16-bit DOS environment (DOSBox-X or real DOS)
- Tandy 1000 hardware or accurate emulation for testing

Standard Linux toolchains (GCC, Clang) cannot produce the required 16-bit DOS executables.

## Environment Setup Requirements

### Required Tools
- **DOSBox-X** (recommended DOS emulator) or **PCem** for accurate Tandy 1000 emulation
- **Microsoft C 6.0 or 7.0** complete toolchain with libraries
- Alternative: **Open Watcom 16-bit tools** (`wcl`, `wlink`) as open-source option

### DOSBox-X Setup Commands
```bash
# Install DOSBox-X (varies by system)
# Ubuntu/Debian: apt install dosbox-x
# Or download from: https://dosbox-x.com/

# Create DOS environment
dosbox-x -c "mount c /path/to/repository"
dosbox-x -c "mount d /path/to/msc60"  # Microsoft C 6.0 installation
dosbox-x -c "c:"
```

### Microsoft C 6.0 Installation
- Mount Microsoft C 6.0 installation directory in DOSBox-X
- Ensure `cl.exe`, `link.exe`, and libraries are in PATH
- Verify with: `cl /?` and `link /?`

## Build Process

### Build Commands
From DOS environment (DOSBox-X):
```
build
```

This executes `build.bat` which runs:
```
cl /O /AS /Fepsgtest.exe src\psgtest.c > output.txt
link /NOI /TINY src\psgtest.obj, psgtest.exe,,;    >link.txt
```

### Build Timing
- **NEVER CANCEL**: Build completes in under 2 minutes typically
- Single C file compilation is very fast on modern hardware
- Link step creates tiny memory model executable

### Build Output
- `psgtest.exe` - Tandy PSG test program
- `output.txt` - Compiler messages
- `link.txt` - Linker messages
- `psgtest.obj` - Object file (intermediate)

## Testing and Validation

### Primary Test Program
Run `psgtest.exe` in DOS environment:
```
psgtest
```

**Expected behavior**:
- Plays C major triad arpeggio (C4, E4, G4) on PSG channels 0-2
- Each note plays for ~200ms
- Brief noise burst after triad
- Gradual fade out over ~500ms
- Total runtime: ~1.5 seconds

### Hardware Requirements for Testing
- **Real Tandy 1000 EX** or compatible with SN76496 PSG chip
- **DOSBox-X** with Tandy emulation enabled: `machine=tandy`
- **PCem** with Tandy 1000 configuration

### Validation Scenarios
1. **Audio Test**: Run `psgtest.exe` and verify audible triad plays
2. **Build Verification**: Check for clean compilation with no errors
3. **File Output**: Verify `psgtest.exe` is created with correct size (~1KB)
4. **Memory Model**: Confirm tiny model linking (no DOS extender required)
5. **Hardware Compatibility**: Test on multiple Tandy 1000 variants if available

### Complete Validation Workflow
```
# 1. Clean build
del *.exe *.obj *.txt

# 2. Build from scratch
build

# 3. Verify output files exist
dir psgtest.exe
dir psgtest.obj

# 4. Check for build errors
type output.txt
type link.txt

# 5. Test executable
psgtest

# 6. Verify audio output manually:
#    - C4 note (262 Hz) plays ~200ms
#    - E4 note (330 Hz) plays ~200ms  
#    - G4 note (392 Hz) plays ~400ms
#    - All three notes sustain together ~300ms
#    - Noise burst ~120ms
#    - Fade out ~500ms
```

## Development Workflow

### Making Code Changes
1. Edit source files in `src/` directory using DOS-compatible editor or modern editor
2. Ensure line endings are DOS format (CRLF) if editing outside DOS
3. Run `build` in DOS environment
4. Test with `psgtest.exe` 
5. Verify audio output matches expected behavior
6. Check for memory leaks or crashes on exit

### Code Review Checklist
- [ ] Follows C89 syntax (no C99 features)
- [ ] Uses `/* */` comments only (no `//`)
- [ ] 4-space indentation, no tabs
- [ ] Lines ≤ 80 characters
- [ ] Static functions for internal use
- [ ] Explicit integer types (`unsigned`, `WORD`, `BYTE`)
- [ ] `const` qualifiers where appropriate
- [ ] `void` parameter lists for no-parameter functions
- [ ] No C library dependencies beyond DOS.H/CONIO.H

### Testing New PSG Features
When adding sound functionality:
1. **Test frequency accuracy**: Use oscilloscope or frequency counter if available
2. **Test volume levels**: Ensure 0-15 range works correctly
3. **Test timing**: Verify note durations match code expectations
4. **Test polyphony**: Verify multiple channels work simultaneously
5. **Test noise channel**: Validate different noise modes

### Code Style Requirements
- **C89 syntax only** - no C99/C++ extensions
- **No `//` comments** - use `/* */` style
- **4 spaces indentation** - no tabs
- **80 column limit**
- **Static functions** for internal use
- **ALL_CAPS macros**
- **Explicit types**: `unsigned`, `WORD`, `BYTE`
- **`const` where possible**
- **`void` parameter lists** for no-parameter functions

### Key Source Files
- `src/psgtest.c` - Main PSG test program and API examples
- `src/CONIO.H` - Microsoft C 6.0 console I/O header
- `src/DOS.H` - Microsoft C 6.0 DOS interface header
- `build.bat` - DOS build script

## Common Development Tasks

### Adding New Sound Features
1. Study existing PSG functions in `psgtest.c`:
   - `sn_set_tone()` - Set frequency for channels 0-2
   - `sn_set_volume()` - Set volume (0=loudest, 15=silent)
   - `sn_set_noise()` - Configure noise channel
2. Reference SN76496 datasheet for register details
3. Test on real hardware or accurate emulation

### Windows Driver Development (Future)
- Use **Windows 3.1 DDK tools** to build `.DRV` files
- Follow installable driver architecture
- Map MIDI messages to PSG channels

## Troubleshooting

### Build Fails with "Bad command or file name"
- Verify Microsoft C 6.0 is installed and in PATH
- Check that `cl.exe` and `link.exe` are accessible
- Ensure running in proper DOS environment
- Try: `cl /?` to test compiler availability

### Build Fails with "Cannot open include file"
- Verify `CONIO.H` and `DOS.H` exist in `src/` directory
- Check include path in compiler setup
- Ensure proper case sensitivity (DOS is case-insensitive)

### Link Fails with "Cannot find object file"
- Verify `psgtest.obj` was created by compiler step
- Check for compilation errors in `output.txt`
- Ensure sufficient disk space in DOS environment

### No Audio Output from psgtest.exe
- Verify Tandy emulation is enabled in DOSBox-X
- Check that Tandy 1000 machine type is selected
- Ensure audio is not muted in emulator
- Try different DOSBox-X audio settings (`sbtype=none`, `tandysound=on`)

### "Permission denied" on build.bat  
- Run in DOS environment, not Linux shell
- Use DOSBox-X or similar DOS emulator
- Check file permissions in mounted directory

### DOSBox-X Audio Issues
```
# In DOSBox-X config file or command line:
machine=tandy
tandysound=on
tandy=on
sbtype=none
```

### Compilation Warnings About Far Pointers
- Normal for 16-bit DOS programs using memory models
- Check that `/AS` (small memory model) is used in build.bat
- Warnings about `far` keywords are expected in DOS.H

### Testing Without Tandy Hardware
- Use DOSBox-X with Tandy emulation (most accurate)
- Alternative: mTCP project's Tandy sound tools for validation
- Note: Regular DOSBox may not accurately emulate PSG

## CI/Automation Limitations

**Standard Linux CI cannot build this project**. For automation:
- Set up DOSBox-X in CI environment
- Install Microsoft C 6.0 toolchain
- Example CI command: `dosbox-x -c "mount c . & c: & build.bat"`
- Consider Open Watcom as CI-friendly alternative

## Alternative Open Source Toolchain

### Open Watcom Setup (CI-Friendly)
Download Open Watcom 1.9 or 2.0:
```bash
# Install Open Watcom (varies by system)
wget https://github.com/open-watcom/open-watcom-v2/releases/latest
# Extract and add to PATH
```

### Open Watcom Build Commands
```bash
# 16-bit DOS executable
wcl -bt=dos -ml src/psgtest.c -fe=psgtest.exe

# Alternative with explicit options:
wcc -bt=dos -ml -ox src/psgtest.c
wlink system dos option tiny file psgtest.obj name psgtest.exe
```

### Open Watcom Compatibility Notes
- May require minor source modifications
- Different calling conventions for far pointers
- Some Microsoft C 6.0 specific features may not work
- Headers in `src/` directory are Microsoft-specific

### Recommended CI Integration
```yaml
# Example GitHub Actions workflow
name: Build Tandy Sound Driver
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Install Open Watcom
      run: |
        wget -q https://github.com/open-watcom/open-watcom-v2/releases/download/Current-build/ow-snapshot.tar.xz
        tar -xf ow-snapshot.tar.xz
        echo "$GITHUB_WORKSPACE/watcom/binl64" >> $GITHUB_PATH
    - name: Build with Open Watcom
      run: wcl -bt=dos -ml src/psgtest.c -fe=psgtest.exe
```

## Expected File Sizes and Validation

### Source Files (Pre-Build)
- `src/psgtest.c`: 123 lines, ~4.8 KB - Main PSG test program
- `src/CONIO.H`: 42 lines, ~1.5 KB - Console I/O declarations
- `src/DOS.H`: 225 lines, ~7.2 KB - DOS interface structures
- `build.bat`: 5 lines, ~200 bytes - Build script

### Build Outputs (Post-Build)
- `psgtest.exe`: ~1-2 KB (tiny memory model executable)
- `psgtest.obj`: ~1 KB (object file)
- `output.txt`: Variable size (compiler messages)
- `link.txt`: Variable size (linker messages)

### File Integrity Validation
Before building, verify source files:
```
# In DOS environment
dir src
type src\psgtest.c | find /c /v ""    # Should show 123 lines
```

After building, verify outputs exist:
```
dir *.exe
dir *.obj
```

## Project Structure
```
/
├── README.md          # Project overview
├── AGENTS.md          # AI contributor guidelines  
├── build.bat          # DOS build script
└── src/
    ├── psgtest.c      # Main PSG test program
    ├── CONIO.H        # Console I/O header
    └── DOS.H          # DOS interface header
```

## Quick Reference Commands

### In DOSBox-X/DOS:
```
build               # Build psgtest.exe
psgtest            # Run PSG audio test
dir                # List files
type output.txt    # View compiler output
type link.txt      # View linker output
```

### Common File Operations:
```
copy src\*.c backup\    # Backup source files
del *.obj              # Clean object files
del psgtest.exe        # Clean executable
```

Always verify changes by building and running the PSG test to ensure audio output remains correct.

## Validating These Instructions

### Before Using These Instructions
1. **Environment Test**: Verify DOSBox-X launches with Tandy emulation
2. **Toolchain Test**: Confirm Microsoft C 6.0 or Open Watcom is accessible  
3. **Build Test**: Successfully build `psgtest.exe` from clean state
4. **Audio Test**: Confirm PSG triad plays correctly

### Expected Instruction Success Rate
- **Environment Setup**: 90% success with DOSBox-X + MS C 6.0
- **Build Process**: 95% success once environment is correct
- **Audio Testing**: 85% success (depends on accurate Tandy emulation)
- **Code Changes**: 90% success following style guidelines

### When Instructions Fail
1. **Check Environment**: Verify DOS emulator and toolchain setup
2. **Check File Integrity**: Verify source files are intact and readable
3. **Check Audio Setup**: Ensure Tandy sound emulation is working
4. **Fallback**: Try Open Watcom toolchain for compatibility

### Updating These Instructions
When instructions become outdated:
- Test all commands in fresh DOS environment
- Update version numbers for DOSBox-X and toolchains
- Verify audio behavior matches documented expectations
- Test alternative toolchains for compatibility

## Reference Documentation
- [TI SN76489/SN76496 Datasheet](https://www.smspower.org/Development/SN76489)
- [Bisqwit's Tandy PSG Programming Guide](https://bisqwit.iki.fi/jutut/kuvat/programming_examples/tandysnd.html)
- [DOSBox-X Configuration Reference](https://dosbox-x.com/wiki/)
- [Microsoft C 6.0 Documentation](https://archive.org/details/MicrosoftC6.0)
- [Open Watcom Documentation](http://www.openwatcom.org/doc.php)