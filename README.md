# GFAmigaAriaMaestosa

Aria Maestosa, the MIDI sequencer and editor (score, keyboard, guitar tablature, drum and controller views), for GFOS (AmigaOS 3.2.3 with GFOpenUp), installed as a GFOpenUp part.

Designed, tested and delivered by The AmiCradle Team.

## What you need

- GFOS: AmigaOS 3.2.3 with GFOpenUp, and the GFOpenWX Toolkit (openwxtoolkit.library) it ships.
- A 68040 or 68060 (soft float; the FPU is used when there is one), an RTG screen, about 8 MB of free memory.
- For sound: camd.library and a CAMD MIDI driver (a serial MIDI interface to a synthesizer, or a software one). Aria sends to the CAMD cluster `out.0`; set another with `SetEnv SAVE AriaMaestosa/MidiOut <cluster>`. Without CAMD, Aria still writes, edits, prints and saves songs (.aria and MIDI files).

## Install

    GFOpenUpTool INSTALL Parts/AriaMaestosa

puts it in `SYS:Utilities/AriaMaestosa`. Its settings and presets are kept in that drawer's `User` drawer.

## What is different on the Amiga

- Playback is our own CAMD player (`amiga/`): Aria's song as a MIDI file, parsed by `camd_smf.c` and sent by a process of its own timed by timer.device (`camd_player.c`). Preview notes go straight out.
- No MIDI input recording and no audio (WAV) export in this version.
- Drawing is wxWidgets' own (`RENDERER_WXWIDGETS`): no OpenGL, no GFOpenGPU.
- Resources sit beside the program (`PROGDIR:Resources/`), settings in `PROGDIR:User/` (patch 0001).
- RtMidi has an Amiga section reporting no ports (patch 0002).

## Building

    STOVE=.../stoves/os32-gcc16-open8 OUT=out ./build-amiga.sh

or in Cradle, `stove_build` with the os32-gcc16-open8 stove and target `.`. Aria Maestosa, the GFOpenWX Toolkit and fpuauto are fetched at the commits in `aria.lock`; nothing third-party is kept here. A run stops at 840 s and goes on next time (the toolkit takes about two runs the first time). The cache includes the resolved stove and compiler version, so changing compilers cannot reuse stale toolkit or Aria objects.

## Tests

    make -C tests test                 # SMF parser, Part file, lock
    make -C tests test ARIA_SRC=<aria checkout at the lock's commit>   # also applies the patches

## Licence

Aria Maestosa is GPL-2.0-or-later by Marianne Gagnon and its contributors (https://github.com/ariamaestosa/ariamaestosa); libjdkmidi, irrXML and rtmidi in its tree carry their own licences. The Amiga files here are GPL-2.0-or-later, Copyright (c) 2026 AmiCradle Team.
