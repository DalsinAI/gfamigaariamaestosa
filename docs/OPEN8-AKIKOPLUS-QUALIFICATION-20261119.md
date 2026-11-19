# Aria Maestosa open8 / AkikoPlus qualification — 19 November 2026

This records the current build and native lab evidence for the Amiga port.  It
does not replace a real-card audio test: the Cradle lab is intentionally muted.

## Sources checked

- Repository: `DalsinAI/gfamigaariamaestosa`
- Continuation PR: #2, `team1/aria-open8-rc-20261119`, stacked on Team 3 PR #1
- Aria port binary commit: `cf2a61798b6117dffa9145b2ff167d447f82126e`
- AkikoPlus guest/CAMD commit: `DalsinAI/gfosautoconfigboards@827677a439ff821561c11143afc4aefb80973633`
- Open CAMD runtime: `kmatheussen/camd@55a2f589bba0277bb7e779547a0e6d9a15aff2c0`
- GFOpenWX runtime: release `v1.0-rc4`, commit pinned in `aria.lock`

## Clean builds

- Aria job `j20261119T185048-366be2`: open8, 167/167 objects, 78-file
  GFOpenUp Part, 27 fpuauto wrappers, exit 0.
- `AriaMaestosa`: 9,747,640 bytes; SHA-256
  `024d067668d3d4fbfa03206648abe401c4ce889532f4212de7d6e4b2342adb69`.
- AkikoPlus guest job `j20261119T183625-086efa`: open8, exit 0;
  `DEVS:Midi/akikoplus` 2,292 bytes, SHA-256
  `66abca966f25b7d465ee6a964e5ea2a6e2ae1b052b4e6f29b9b23be0dc632dec`.
- Host SMF parser, lock/patch, and Part tests passed before the build.

The earlier clean job `j20261119T184347-5e7f7c` correctly failed on a malformed
patch header.  The header was fixed and dry-run against pristine pinned Aria
source before the successful jobs above.

## Native Cradle lab

Lab: `lab:mitchy-20261108`, Workbench 47.100 / Kickstart 47.2, AkikoPlus board.

Installed and verified:

- `camd.library 40.4`
- `openwxtoolkit.library 1.1`
- `akikoplus 1.0` CAMD driver
- `akikomidi.device 1.0`
- the exact Aria binary from `j20261119T185048-366be2`

Observed in this session:

1. Independent `AKMTest` reported TX ready, General MIDI SoundFont loaded and
   MT-32 ROMs found, then delivered exactly 194/194 test bytes.
2. Aria launched into its complete editor.  The GFOpenWX first-show fix made
   its command-line loader run, and `akikoplus-smoke.mid` appeared as a piano
   roll rather than the former blank window.
3. Aria selected its CAMD backend even though this reused lab contained the
   stale legacy `No Sound` preference.
4. Pressing Play completed without `Couldn't play`.
5. After Aria closed, `AKMTest` reported **546 bytes already counted by the
   AkikoPlus board**.  The counter was zero after the lab restart, so these
   bytes came from Aria through CAMD and the `akikoplus.out.0` driver path.
6. A second `AKMTest` again delivered exactly 194/194 bytes, showing the port
   remained healthy after Aria playback.

## Result and remaining gate

The editor startup, MIDI import, Play action, CAMD routing, AkikoPlus driver and
OpenSynth-facing board byte path are qualified in the native lab.  Audible
output is not claimed because the lab runtime was muted.  Before calling this a
final release, repeat the same smoke MIDI on an audible card image and confirm
the expected C4 note from the loaded General MIDI SoundFont.
