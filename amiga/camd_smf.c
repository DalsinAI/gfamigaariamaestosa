/* See camd_smf.h. GPL-2.0-or-later. Copyright (c) 2026 AmiCradle Team. */
#include "camd_smf.h"
#include <stdlib.h>
#include <string.h>

/* An event as read, before the tempo map gives it a time. */
struct raw {
    uint32_t tick;
    uint32_t order;   /* file order, so equal ticks keep it (stable) */
    uint32_t msg;     /* 0xFF510000 marks a tempo meta: value in tempo */
    uint32_t tempo;
};

#define TEMPO_MARK 0xFF510000u

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static uint32_t be16(const uint8_t *p) { return (uint32_t)p[0] << 8 | p[1]; }

/* A variable-length quantity at *pos (at most 4 bytes), or -1 past end. */
static int vlq(const uint8_t *d, uint32_t end, uint32_t *pos, uint32_t *out)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 4; i++) {
        uint8_t b;
        if (*pos >= end) return -1;
        b = d[(*pos)++];
        v = (v << 7) | (b & 0x7f);
        if (!(b & 0x80)) { *out = v; return 0; }
    }
    return -1;
}

static int data_bytes(uint8_t status)
{
    switch (status & 0xf0) {
    case 0xc0: case 0xd0: return 1;
    default: return 2;
    }
}

int amp_msg_len(uint32_t msg)
{
    return 1 + data_bytes((uint8_t)(msg >> 24));
}

static int cmp_raw(const void *a, const void *b)
{
    const struct raw *x = a, *y = b;
    if (x->tick != y->tick) return x->tick < y->tick ? -1 : 1;
    return x->order < y->order ? -1 : x->order > y->order;
}

static int push(struct raw **v, uint32_t *n, uint32_t *cap, struct raw r)
{
    if (*n == *cap) {
        uint32_t c = *cap ? *cap * 2 : 256;
        struct raw *nv = realloc(*v, c * sizeof **v);
        if (!nv) return -1;
        *v = nv; *cap = c;
    }
    r.order = *n;
    (*v)[(*n)++] = r;
    return 0;
}

/* ticks at tempo (microseconds per quarter) and division, in microseconds;
 * 64-bit so a long song at a slow tempo cannot overflow. */
static uint32_t ticks_us(uint32_t ticks, uint32_t tempo, uint32_t division)
{
    return (uint32_t)(((uint64_t)ticks * tempo) / division);
}

int amp_parse(const uint8_t *d, uint32_t len, struct amp_song *song)
{
    struct raw *raw = 0;
    uint32_t nraw = 0, cap = 0, ntracks, division, pos, t, i, last_tick = 0;
    memset(song, 0, sizeof *song);
    if (len < 14 || memcmp(d, "MThd", 4) || be32(d + 4) < 6) return AMP_E_HEADER;
    ntracks = be16(d + 10);
    division = be16(d + 12);
    if (division == 0 || (division & 0x8000)) return AMP_E_HEADER;
    pos = 8 + be32(d + 4);
    for (t = 0; t < ntracks; t++) {
        uint32_t end, tick = 0;
        uint8_t running = 0;
        if (pos + 8 > len || memcmp(d + pos, "MTrk", 4)) { free(raw); return AMP_E_TRACK; }
        end = pos + 8 + be32(d + pos + 4);
        if (end > len || end < pos) { free(raw); return AMP_E_TRACK; }
        pos += 8;
        while (pos < end) {
            uint32_t delta, l;
            uint8_t st;
            if (vlq(d, end, &pos, &delta)) { free(raw); return AMP_E_TRACK; }
            tick += delta;
            if (pos >= end) { free(raw); return AMP_E_TRACK; }
            st = d[pos];
            if (st == 0xff) {            /* meta: type, length, data */
                uint8_t type;
                if (pos + 2 > end) { free(raw); return AMP_E_TRACK; }
                type = d[pos + 1];
                pos += 2;
                if (vlq(d, end, &pos, &l) || pos + l > end) { free(raw); return AMP_E_TRACK; }
                if (type == 0x51 && l == 3) {
                    struct raw r = { tick, 0, TEMPO_MARK, (uint32_t)d[pos] << 16 | (uint32_t)d[pos + 1] << 8 | d[pos + 2] };
                    if (push(&raw, &nraw, &cap, r)) { free(raw); return AMP_E_NOMEM; }
                }
                pos += l;
                if (type == 0x2f) break;     /* end of track */
                continue;
            }
            if (st == 0xf0 || st == 0xf7) {  /* SysEx: left out */
                pos++;
                if (vlq(d, end, &pos, &l) || pos + l > end) { free(raw); return AMP_E_TRACK; }
                pos += l;
                running = 0;
                continue;
            }
            if (st & 0x80) { running = st; pos++; }
            else if (!running) { free(raw); return AMP_E_TRACK; }
            {
                int nb = data_bytes(running);
                uint32_t msg = (uint32_t)running << 24;
                if (pos + nb > end) { free(raw); return AMP_E_TRACK; }
                msg |= (uint32_t)(d[pos] & 0x7f) << 16;
                if (nb == 2) msg |= (uint32_t)(d[pos + 1] & 0x7f) << 8;
                pos += nb;
                {
                    struct raw r = { tick, 0, msg, 0 };
                    if (push(&raw, &nraw, &cap, r)) { free(raw); return AMP_E_NOMEM; }
                }
            }
        }
        if (tick > last_tick) last_tick = tick;
        pos = end;
    }
    if (nraw) qsort(raw, nraw, sizeof *raw, cmp_raw);
    song->division = division;
    song->ev = malloc((nraw ? nraw : 1) * sizeof *song->ev);
    if (!song->ev) { free(raw); return AMP_E_NOMEM; }
    {
        uint32_t tempo = 500000, at_tick = 0, at_us = 0, n = 0;
        for (i = 0; i < nraw; i++) {
            uint32_t us = at_us + ticks_us(raw[i].tick - at_tick, tempo, division);
            if (raw[i].msg == TEMPO_MARK) {
                at_us = us; at_tick = raw[i].tick;
                if (raw[i].tempo) tempo = raw[i].tempo;
                continue;
            }
            song->ev[n].us = us;
            song->ev[n].tick = raw[i].tick;
            song->ev[n].msg = raw[i].msg;
            n++;
        }
        song->n = n;
        song->end_tick = last_tick;
        song->end_us = at_us + ticks_us(last_tick > at_tick ? last_tick - at_tick : 0, tempo, division);
    }
    free(raw);
    return AMP_OK;
}

void amp_free(struct amp_song *song)
{
    free(song->ev);
    memset(song, 0, sizeof *song);
}

uint32_t amp_tick_at(const struct amp_song *s, uint32_t us)
{
    uint32_t lo = 0, hi = s->n;
    if (!s->n || us >= s->end_us) return s->end_tick;
    /* the last event at or before us */
    while (lo < hi) {
        uint32_t mid = (lo + hi) / 2;
        if (s->ev[mid].us <= us) lo = mid + 1; else hi = mid;
    }
    if (lo == 0) {
        /* before the first event: scale over its span */
        return s->ev[0].us ? (uint32_t)((uint64_t)s->ev[0].tick * us / s->ev[0].us) : 0;
    }
    {
        const struct amp_event *a = &s->ev[lo - 1];
        uint32_t nt = lo < s->n ? s->ev[lo].tick : s->end_tick;
        uint32_t nu = lo < s->n ? s->ev[lo].us : s->end_us;
        if (nu <= a->us) return a->tick;
        return a->tick + (uint32_t)((uint64_t)(nt - a->tick) * (us - a->us) / (nu - a->us));
    }
}
