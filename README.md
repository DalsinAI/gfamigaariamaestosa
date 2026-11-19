# GFAmigaAriaMaestosa

Aria Maestosa, the MIDI sequencer and editor (score, keyboard, guitar tablature, drum and controller views), for GFOS (AmigaOS 3.2.3 with GFOpenUp), installed as a GFOpenUp part.

Designed, tested and delivered by The AmiCradle Team.

## What you need

- GFOS: AmigaOS 3.2.3 with GFOpenUp, and the GFOpenWX Toolkit (openwxtoolkit.library) it ships.
- A 68040 or 68060 (soft float; the FPU is used when there is one), an RTG screen, about 8 MB of free memory.
- For sound: camd.library and the AkikoPlus CAMD driver. Aria sends to `akikoplus.out.0`, whose bytes are rendered by AkikoPlus/OpenSynth; set another CAMD cluster with `SetEnv SAVE AriaMaestosa/MidiOut <cluster>`. Without CAMD, Aria still writes, edits, prints and saves songs (.aria and MIDI files).

## Install

For a release candidate, unpack `gfamigaariamaestosa-*-part.tar.gz`, then:

    GFOpenUpTool INSTALL Parts/AriaMaestosa

puts it in `SYS:Utilities/AriaMaestosa`. Its settings and presets are kept in that drawer's `User` drawer.

The release archive also carries `PART.json` (the exact source commit and
binary digest) and `SHA256SUMS` is published beside it. This keeps the file
being tested tied to the source and clean Open8 build that produced it.

## What is different on the Amiga

- Playback is our own CAMD player (`amiga/`): Aria's song as a MIDI file, parsed by `camd_smf.c` and sent by a process of its own timed by timer.device (`camd_player.c`). Preview notes go straight out.
- No MIDI input recording and no audio (WAV) export in this version.
- Drawing is wxWidgets' own (`RENDERER_WXWIDGETS`): no OpenGL, no GFOpenGPU.
- Resources sit beside the program (`PROGDIR:Resources/`), settings in `PROGDIR:User/` (patch 0001).
- RtMidi has an Amiga section reporting no ports (patch 0002).
- GFOpenWX's first window show is initialised explicitly, so the editor and command-line song loader run (patch 0004).
- The Amiga build selects CAMD even when an old cross-platform `No Sound` preference remains (patch 0005).

## Building

    STOVE=.../stoves/os32-gcc16-open8 OUT=out ./build-amiga.sh

or in Cradle, `stove_build` with the os32-gcc16-open8 stove and target `.`. Aria Maestosa, the GFOpenWX Toolkit and fpuauto are fetched at the commits in `aria.lock`; nothing third-party is kept here. A run stops at 840 s and goes on next time (the toolkit takes about two runs the first time). The cache includes the resolved stove and compiler version, so changing compilers cannot reuse stale toolkit or Aria objects.

After a complete build, make an installable candidate with:

    ./package.sh out dist 1.4.13-rc1 <full-source-commit>

## Tests

    make -C tests test                 # SMF parser, Part file, lock
    make -C tests test ARIA_SRC=<aria checkout at the lock's commit>   # also applies the patches

The current clean-build and native AkikoPlus byte-path receipt is in
[`docs/OPEN8-AKIKOPLUS-QUALIFICATION-20261119.md`](docs/OPEN8-AKIKOPLUS-QUALIFICATION-20261119.md).

## Licence

Aria Maestosa is GPL-2.0-or-later by Marianne Gagnon and its contributors (https://github.com/ariamaestosa/ariamaestosa); libjdkmidi, irrXML and rtmidi in its tree carry their own licences. The Amiga files here are GPL-2.0-or-later, Copyright (c) 2026 AmiCradle Team.
