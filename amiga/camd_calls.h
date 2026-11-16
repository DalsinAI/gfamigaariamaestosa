/* The camd.library calls the player uses, called through its vectors (the
 * NDK 3.2 has no CAMD headers). Vector offsets and tag values are CAMD 40's,
 * as AROS's workbench/libs/camd/camd.conf and compiler/include/midi/camd.h
 * also give them: CreateMidiA -42, DeleteMidi -48, FlushMidi -78,
 * AddMidiLinkA -84, RemoveMidiLink -90, PutMidi -138.
 * GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
#ifndef ARIA_CAMD_CALLS_H
#define ARIA_CAMD_CALLS_H

#include <exec/types.h>
#include <utility/tagitem.h>

#define ACAMD_MIDI_Base     (TAG_USER + 65)
#define ACAMD_MIDI_Name     (ACAMD_MIDI_Base + 0)
#define ACAMD_MIDI_MsgQueue (ACAMD_MIDI_Base + 6)
#define ACAMD_MIDI_SysExSize (ACAMD_MIDI_Base + 7)
#define ACAMD_MLINK_Base     (TAG_USER + 65)
#define ACAMD_MLINK_Location (ACAMD_MLINK_Base + 0)
#define ACAMD_MLINK_Comment  (ACAMD_MLINK_Base + 4)
#define ACAMD_MLTYPE_Sender  1

#ifdef __HAVE_68881__
#define ACAMD_FPCLOBBER , "fp0", "fp1"
#else
#define ACAMD_FPCLOBBER
#endif

extern struct Library *CamdBase;

static inline APTR camd_CreateMidiA(struct TagItem *tags)
{
    register APTR r __asm("d0");
    register struct Library *b __asm("a6") = CamdBase;
    register struct TagItem *t __asm("a0") = tags;
    __asm volatile ("jsr a6@(-42:W)" : "=r"(r), "+r"(t) : "r"(b) : "d1", "a1", "cc", "memory" ACAMD_FPCLOBBER);
    return r;
}

static inline void camd_DeleteMidi(APTR node)
{
    register struct Library *b __asm("a6") = CamdBase;
    register APTR n __asm("a0") = node;
    __asm volatile ("jsr a6@(-48:W)" : "+r"(n) : "r"(b) : "d0", "d1", "a1", "cc", "memory" ACAMD_FPCLOBBER);
}

static inline void camd_FlushMidi(APTR node)
{
    register struct Library *b __asm("a6") = CamdBase;
    register APTR n __asm("a0") = node;
    __asm volatile ("jsr a6@(-78:W)" : "+r"(n) : "r"(b) : "d0", "d1", "a1", "cc", "memory" ACAMD_FPCLOBBER);
}

static inline APTR camd_AddMidiLinkA(APTR node, LONG type, struct TagItem *tags)
{
    register APTR r __asm("d0") = (APTR)type;
    register struct Library *b __asm("a6") = CamdBase;
    register APTR n __asm("a0") = node;
    register struct TagItem *t __asm("a1") = tags;
    __asm volatile ("jsr a6@(-84:W)" : "+r"(r), "+r"(n), "+r"(t) : "r"(b) : "d1", "cc", "memory" ACAMD_FPCLOBBER);
    return r;
}

static inline void camd_RemoveMidiLink(APTR link)
{
    register struct Library *b __asm("a6") = CamdBase;
    register APTR l __asm("a0") = link;
    __asm volatile ("jsr a6@(-90:W)" : "+r"(l) : "r"(b) : "d0", "d1", "a1", "cc", "memory" ACAMD_FPCLOBBER);
}

static inline void camd_PutMidi(APTR link, ULONG msg)
{
    register struct Library *b __asm("a6") = CamdBase;
    register APTR l __asm("a0") = link;
    register ULONG m __asm("d0") = msg;
    __asm volatile ("jsr a6@(-138:W)" : "+r"(l), "+r"(m) : "r"(b) : "d1", "a1", "cc", "memory" ACAMD_FPCLOBBER);
}

#endif
