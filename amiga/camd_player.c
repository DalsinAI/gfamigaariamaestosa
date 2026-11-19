/* See camd_player.h. GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
#include "camd_player.h"
#include "camd_calls.h"

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <string.h>

struct Library *CamdBase;
static APTR node, link;
static char cluster[64] = "akikoplus.out.0";

static struct amp_song song;
static volatile LONG playing, stop_asked;
static volatile ULONG position_us;
static struct SignalSemaphore send_lock;

const char *acp_cluster(void) { return cluster; }

int acp_open(void)
{
    struct TagItem ntags[] = {
        { ACAMD_MIDI_Name, (ULONG)"Aria Maestosa" },
        { ACAMD_MIDI_MsgQueue, 0 },
        { ACAMD_MIDI_SysExSize, 0 },
        { TAG_DONE, 0 }
    };
    struct TagItem ltags[] = {
        { ACAMD_MLINK_Location, (ULONG)cluster },
        { ACAMD_MLINK_Comment, (ULONG)"Aria Maestosa" },
        { TAG_DONE, 0 }
    };
    if (link) return 1;
    InitSemaphore(&send_lock);
    if (GetVar((STRPTR)"AriaMaestosa/MidiOut", (STRPTR)cluster, sizeof cluster, GVF_GLOBAL_ONLY) <= 0)
        strcpy(cluster, "akikoplus.out.0");
    if (!CamdBase) CamdBase = OpenLibrary((CONST_STRPTR)"camd.library", 37);
    if (!CamdBase) return 0;
    if (!node) node = camd_CreateMidiA(ntags);
    if (node) link = camd_AddMidiLinkA(node, ACAMD_MLTYPE_Sender, ltags);
    if (!link) { acp_close(); return 0; }
    return 1;
}

static void all_off(void)
{
    int ch;
    for (ch = 0; ch < 16; ch++) {
        camd_PutMidi(link, (ULONG)(0xb0 | ch) << 24 | 123u << 16);  /* all notes off */
        camd_PutMidi(link, (ULONG)(0xb0 | ch) << 24 | 121u << 16);  /* reset controllers */
    }
}

void acp_close(void)
{
    acp_stop();
    if (link) { all_off(); camd_RemoveMidiLink(link); link = 0; }
    if (node) { camd_DeleteMidi(node); node = 0; }
    if (CamdBase) { CloseLibrary(CamdBase); CamdBase = 0; }
}

void acp_send(uint32_t msg)
{
    if (!link) return;
    ObtainSemaphore(&send_lock);
    camd_PutMidi(link, msg);
    ReleaseSemaphore(&send_lock);
}

static ULONG us_between(const struct timeval *a, const struct timeval *b)
{
    return (b->tv_secs - a->tv_secs) * 1000000u + b->tv_micro - a->tv_micro;
}

/* The player process: waits for each event's time in steps of at most 20 ms
 * (so a stop is quick and the position stays fresh), then sends it. */
static void player(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct timerequest *tr = port ? (struct timerequest *)CreateIORequest(port, sizeof *tr) : 0;
    if (tr && !OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ, (struct IORequest *)tr, 0)) {
        struct Library *TimerBase = (struct Library *)tr->tr_node.io_Device;
        struct timeval start, now;
        ULONG i = 0;
        GetSysTime(&start);
        while (!stop_asked) {
            ULONG at;
            GetSysTime(&now);
            at = us_between(&start, &now);
            position_us = at;
            if (i >= song.n) {
                if (at >= song.end_us) break;
            } else if (song.ev[i].us <= at) {
                ObtainSemaphore(&send_lock);
                while (i < song.n && song.ev[i].us <= at) camd_PutMidi(link, song.ev[i++].msg);
                ReleaseSemaphore(&send_lock);
                continue;
            }
            {
                ULONG next = i < song.n ? song.ev[i].us : song.end_us;
                ULONG wait = next - at;
                if (wait > 20000) wait = 20000;
                tr->tr_node.io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 0;
                tr->tr_time.tv_micro = wait;
                DoIO((struct IORequest *)tr);
            }
        }
        ObtainSemaphore(&send_lock);
        all_off();
        ReleaseSemaphore(&send_lock);
        CloseDevice((struct IORequest *)tr);
    }
    if (tr) DeleteIORequest((struct IORequest *)tr);
    if (port) DeleteMsgPort(port);
    amp_free(&song);
    Forbid();       /* playing = 0 last: acp_stop's wait ends only once we are gone */
    playing = 0;
}

int acp_play(struct amp_song *s)
{
    struct Process *p;
    if (!link || playing) { amp_free(s); return 0; }
    song = *s;
    memset(s, 0, sizeof *s);
    stop_asked = 0;
    position_us = 0;
    playing = 1;
    p = CreateNewProcTags(NP_Entry, (ULONG)player, NP_Name, (ULONG)"Aria Maestosa MIDI",
                          NP_Priority, 21, NP_StackSize, 16384, TAG_DONE);
    if (!p) { playing = 0; amp_free(&song); return 0; }
    return 1;
}

void acp_stop(void)
{
    int n;
    if (!playing) return;
    stop_asked = 1;
    for (n = 0; playing && n < 250; n++) Delay(1);   /* the player checks every 20 ms */
}

int acp_playing(void) { return playing; }
uint32_t acp_position_us(void) { return position_us; }
