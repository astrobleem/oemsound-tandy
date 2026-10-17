# Player format adoption plan

Status: implementation in progress in isolated source work. The current
private R4 MINIPLAY accepts SMF MIDI, raw TND and AGI SND through their
documented paths. MOD, VGM and VGZ are not integrated or qualified player
inputs yet. The public releases retain their existing format scope.

The earlier MOD/VGM work is a starting point, not completed generic player
support. TNDYTracker's `IE.PAS` / `Import_MOD` converts MK/4CHN patterns;
it does not reproduce their original samples. Game-specific VGM conversion
exists, but MINIPLAY has no generic Winamp-style VGM decoder today.

## Milestones

1. Implement bounded foreground streaming parsers and original synthetic
   fixtures. Accept only identified compatible inputs. Malformed or
   unsupported chips, signatures, commands and effects receive a concrete
   reason. Preserve offsets and timing state needed to diagnose failures.
2. Add a usable player input or a documented checked preparation workflow.
   The SN PSG VGM path preserves source clock, 44.1 kHz waits, fractional
   accumulation, loop position and register-write ordering. VGZ preparation
   has bounded decompression and produces the same checked VGM stream.
   The four-channel MOD path retains all four logical channels and maps
   instruments explicitly to PSG patches with documented voice allocation.
   Original sample timbre/fidelity is outside that conversion's scope.
3. Integrate through the existing foreground producer and sole RAWCLK
   owner. Keep MIDI, raw and new-format ownership, pause/reset/stop/EOF/close
   rules consistent. No additional timer or IRQ owner is introduced.
4. Qualify fresh native Windows 3.0 builds with synthetic and malformed
   inputs, exact event/register traces and permitted private music playback.
   Check loops, timing, noise behavior, queue pressure, silence/release and
   CPU/RAM cost. Add supported-input claims only for the tested subset.

The core library's initial publication need not wait for these milestones.
CF repair and core app acceptance retain priority. OutRun game work remains
paused; this plan requests no game edits or new public music assets.

## Backend details that must be resolved

R4's prepared-state minimum spacing is 8 ms. Faster source transitions
cannot be silently merged while claiming event equivalence. The adapter
must identify an unsupported timing case or use a separately qualified
backend capability that can represent it. Source-clock conversion and
wait remainder must survive refill boundaries and loops.

The existing 16-byte TCFRAME noise flag writes both noise control and
attenuation. SN noise-control writes retrigger the noise generator;
attenuation-only writes must preserve it. A new format cannot represent
both operations faithfully through that flag alone. Coordinate an additive
versioned capability/entrypoint with the backend owner; preserve existing
frame layouts, ordinals and old caller behavior. Also retain the selected
noise mode and tone-2 clock coupling.

MOD mapping must expose its supported effects and timing rules. Four source
channels do not imply four melodic PSG voices, and source channel four is
not automatically percussion. Instrument mapping, tone/noise selection and
any deterministic voice stealing must be visible in the contract.

Code and original fixtures may be published with applicable notices.
Copyrighted songs, samples and game media are excluded from public source
and test packages. A private playback result does not grant redistribution
rights or establish support for every file with the same extension.

See [versioned timing and current format paths](SNDTIME.MD).
