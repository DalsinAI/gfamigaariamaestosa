/* A Standard MIDI File as one list of timed channel messages, for the CAMD
 * player (camd_player.c): every track merged in tick order, each event's time
 * in microseconds from the tempo map, and its message packed the way CAMD's
 * PutMidi() takes it (status << 24 | data1 << 16 | data2 << 8).
 * Plain C99, no Amiga headers: tests/test_camd_smf.c runs it on the host.
 * GPL-2.0-or-later, like Aria Maestosa. Copyright (c) 2026 AmiCradle Team. */
#ifndef ARIA_CAMD_SMF_H
#define ARIA_CAMD_SMF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct amp_event {
    uint32_t us;    /* from the start of the song */
    uint32_t tick;  /* in the file's own ticks */
    uint32_t msg;   /* status << 24 | data1 << 16 | data2 << 8 */
};

struct amp_song {
    struct amp_event *ev;
    uint32_t n;
    uint32_t division;  /* ticks per quarter note */
    uint32_t end_us;    /* the last event, or the end-of-track metas if later */
    uint32_t end_tick;
};

enum {
    AMP_OK = 0,
    AMP_E_HEADER = -1,  /* no MThd, or SMPTE time (not written by Aria) */
    AMP_E_TRACK = -2,   /* a track runs past the data or a message is cut */
    AMP_E_NOMEM = -3
};

/* Parses len bytes of a format 0 or 1 file into song (ev malloc'd; free
 * with amp_free). Meta and SysEx events are left out; tempo metas shape the
 * times. Returns AMP_OK or an AMP_E_ code, with song empty. */
int amp_parse(const uint8_t *data, uint32_t len, struct amp_song *song);
void amp_free(struct amp_song *song);

/* The song's tick at us microseconds (between events, by the tempo then in
 * force), for the player's progress line. */
uint32_t amp_tick_at(const struct amp_song *song, uint32_t us);

/* How many bytes a packed message has on the wire (1 to 3). */
int amp_msg_len(uint32_t msg);

#ifdef __cplusplus
}
#endif
#endif
