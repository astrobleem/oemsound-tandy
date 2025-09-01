# Tandy Sound Driver (tandy-snd-drv)

## Overview
This project explores the unused sound capabilities of the **Tandy 1000 EX** and related models under Windows 3.x.  
While the system includes a powerful-for-its-era **SN76496 programmable sound generator** (3 tone channels + 1 noise channel), Windows never shipped with a driver for it. Applications default to PC speaker only.

This repository aims to change that.

## Goals
- Provide a **Windows 3.x installable driver (.DRV)** that exposes the Tandy PSG as a MIDI device.  
- Map Windows MIDI messages to PSG voices (square wave channels + noise).  
- Optionally add **hybrid mode** (PC speaker bass, PSG treble) as documented by [Bisqwit](https://bisqwit.iki.fi/jutut/kuvat/programming_examples/tandysnd.html).  
- Keep CPU usage reasonable for 8088/8086-class machines.

## Features (Planned)
- **MIDI-style playback**: Map MIDI channels/notes → PSG tones.  
- **3-voice polyphony** with note-stealing for extra channels.  
- **Noise/percussion mapping** for simple drum effects.  
- **Optional PC speaker support** for low-pitch bass notes.  

## Why?
The Tandy PSG is historically significant and appears in many DOS games. Bringing it into Windows gives us:  
- Authentic retro sound for system events, Media Player, and MIDI files.  
- A chance to explore **low-level Windows driver development** on vintage hardware.  
- A neat counterpart to the `oemdisplay-tandy` graphics project.

## Status
🚧 Work in progress. Early stage planning & research.  
Driver code and test TSRs will be added step by step.

## References
- [Bisqwit’s Tandy PSG examples](https://bisqwit.iki.fi/jutut/kuvat/programming_examples/tandysnd.html)  
- [TI SN76489 / SN76496 datasheet](https://www.smspower.org/Development/SN76489)  
- Windows 3.1 DDK documentation on Installable Drivers  

---

## License
MIT License (subject to change if needed).

---

## Contributing
Contributions welcome — whether code, docs, or testing on real hardware/DOSBox. Please open an issue or PR.

