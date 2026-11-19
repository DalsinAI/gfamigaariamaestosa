/* Host tests for amiga/camd_smf.c: cc -std=c99 -I amiga tests/test_camd_smf.c amiga/camd_smf.c
 * GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
#include "camd_smf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static uint8_t buf[4096];
static uint32_t blen;
static void put(const void *p, uint32_t n) { memcpy(buf + blen, p, n); blen += n; }
static void b(uint8_t x) { buf[blen++] = x; }
static void header(uint16_t fmt, uint16_t ntr, uint16_t div)
{
    blen = 0;
    put("MThd", 4); b(0); b(0); b(0); b(6);
    b(fmt >> 8); b(fmt & 255); b(ntr >> 8); b(ntr & 255); b(div >> 8); b(div & 255);
}
static uint32_t trk_start;
static void track_begin(void) { put("MTrk", 4); trk_start = blen; b(0); b(0); b(0); b(0); }
static void track_end(void)
{
    uint32_t l = blen - trk_start - 4;
    buf[trk_start] = l >> 24; buf[trk_start + 1] = l >> 16; buf[trk_start + 2] = l >> 8; buf[trk_start + 3] = l;
}
static void delta(uint32_t v)
{
    uint8_t tmp[4]; int n = 0;
    tmp[n++] = v & 0x7f;
    while (v >>= 7) tmp[n++] = 0x80 | (v & 0x7f);
    while (n) b(tmp[--n]);
}
#define MSG(s, a, c) ((uint32_t)(s) << 24 | (uint32_t)(a) << 16 | (uint32_t)(c) << 8)

static void test_format1_tempo_running_status(void)
{
    struct amp_song s;
    header(1, 2, 96);
    track_begin();                                       /* tempo track */
    delta(0); b(0xff); b(0x51); b(3); b(0x07); b(0xa1); b(0x20);   /* 500000 us/qn */
    delta(192); b(0xff); b(0x51); b(3); b(0x03); b(0xd0); b(0x90); /* 250000 at tick 192 */
    delta(0); b(0xff); b(0x2f); b(0);
    track_end();
    track_begin();
    delta(0); b(0xc0); b(5);                             /* program change: 1 data byte */
    delta(0); b(0x90); b(60); b(100);
    delta(96); b(64); b(90);                             /* running status */
    delta(0); b(0xf0); b(3); b(1); b(2); b(0xf7);         /* SysEx: skipped */
    delta(96); b(0x80); b(60); b(0);
    delta(96); b(0x80); b(64); b(0);                     /* tick 288, after the tempo change */
    delta(96); b(0xff); b(0x2f); b(0);                   /* end at tick 384 */
    track_end();
    CHECK(amp_parse(buf, blen, &s) == AMP_OK);
    CHECK(s.n == 5);
    CHECK(s.division == 96);
    CHECK(s.ev[0].msg == MSG(0xc0, 5, 0) && s.ev[0].us == 0);
    CHECK(s.ev[1].msg == MSG(0x90, 60, 100));
    CHECK(s.ev[2].msg == MSG(0x90, 64, 90) && s.ev[2].tick == 96 && s.ev[2].us == 500000);
    CHECK(s.ev[3].msg == MSG(0x80, 60, 0) && s.ev[3].tick == 192 && s.ev[3].us == 1000000);
    CHECK(s.ev[4].tick == 288 && s.ev[4].us == 1250000);  /* 96 ticks at 250000 */
    CHECK(s.end_tick == 384 && s.end_us == 1500000);
    CHECK(amp_tick_at(&s, 0) == 0);
    CHECK(amp_tick_at(&s, 250000) == 48);
    CHECK(amp_tick_at(&s, 1125000) == 240);
    CHECK(amp_tick_at(&s, 9000000) == 384);
    CHECK(amp_msg_len(s.ev[0].msg) == 2 && amp_msg_len(s.ev[1].msg) == 3);
    amp_free(&s);
    CHECK(s.ev == 0 && s.n == 0);
}

static void test_tracks_merge_in_order(void)
{
    struct amp_song s;
    header(1, 2, 480);
    track_begin(); delta(480); b(0x91); b(40); b(1); delta(0); b(0xff); b(0x2f); b(0); track_end();
    track_begin(); delta(0); b(0x92); b(41); b(1); delta(480); b(0x93); b(42); b(1); delta(0); b(0xff); b(0x2f); b(0); track_end();
    CHECK(amp_parse(buf, blen, &s) == AMP_OK);
    CHECK(s.n == 3);
    CHECK(s.ev[0].msg >> 24 == 0x92);
    CHECK(s.ev[1].msg >> 24 == 0x91 && s.ev[2].msg >> 24 == 0x93);  /* equal ticks: track order */
    CHECK(s.ev[1].us == 500000);                                     /* default 120 bpm */
    amp_free(&s);
}

static void test_bad_files(void)
{
    struct amp_song s;
    CHECK(amp_parse((const uint8_t *)"RIFF0000000000", 14, &s) == AMP_E_HEADER);
    header(0, 1, 0x8000 | 25);                       /* SMPTE */
    CHECK(amp_parse(buf, blen, &s) == AMP_E_HEADER);
    header(0, 1, 96);
    track_begin(); delta(0); b(0x90); b(60); track_end();  /* cut message */
    CHECK(amp_parse(buf, blen, &s) == AMP_E_TRACK);
    header(0, 1, 96);
    track_begin(); delta(0); b(60); b(1); track_end();     /* data with no status */
    CHECK(amp_parse(buf, blen, &s) == AMP_E_TRACK);
    header(0, 2, 96);
    track_begin(); delta(0); b(0xff); b(0x2f); b(0); track_end();  /* second track missing */
    CHECK(amp_parse(buf, blen, &s) == AMP_E_TRACK);
    CHECK(s.ev == 0 && s.n == 0);
    header(0, 1, 96);
    track_begin(); delta(0); b(0xff); b(0x2f); b(0); track_end();
    CHECK(amp_parse(buf, blen, &s) == AMP_OK && s.n == 0 && amp_tick_at(&s, 5) == 0);
    amp_free(&s);
}

static void test_long_slow_song_no_overflow(void)
{
    struct amp_song s;
    header(0, 1, 24);
    track_begin();
    delta(0); b(0xff); b(0x51); b(3); b(0xff); b(0xff); b(0xff);  /* slowest tempo */
    delta(24 * 60); b(0x90); b(60); b(1);                          /* 60 quarters */
    delta(0); b(0xff); b(0x2f); b(0);
    track_end();
    CHECK(amp_parse(buf, blen, &s) == AMP_OK);
    CHECK(s.n == 1 && s.ev[0].us == 60u * 16777215u);
    amp_free(&s);
}

int main(void)
{
    test_format1_tempo_running_status();
    test_tracks_merge_in_order();
    test_bad_files();
    test_long_slow_song_no_overflow();
    printf(fails ? "camd_smf: %d FAILED\n" : "camd_smf: all passed\n", fails);
    return fails != 0;
}
