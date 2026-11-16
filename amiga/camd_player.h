/* Aria Maestosa's playback on the Amiga: a song (camd_smf.h) sent to a CAMD
 * cluster (out.0 unless ENV:AriaMaestosa/MidiOut names another) by a
 * process of its own, timed by timer.device. Without camd.library nothing
 * opens and Aria says so.
 * GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
#ifndef ARIA_CAMD_PLAYER_H
#define ARIA_CAMD_PLAYER_H

#include "camd_smf.h"

#ifdef __cplusplus
extern "C" {
#endif

int acp_open(void);                 /* 1 when camd.library and the link are there */
void acp_close(void);
const char *acp_cluster(void);      /* the cluster the link sends to */
int acp_play(struct amp_song *song); /* takes the song (freed when done); 0 when it can't start */
void acp_stop(void);                /* asks the player to stop and waits for it */
int acp_playing(void);
uint32_t acp_position_us(void);
void acp_send(uint32_t msg);        /* one message now (preview notes) */

#ifdef __cplusplus
}
#endif
#endif
