/*
 * Sintetizador chiptune do 4 Conexões.
 *
 * Quatro canais de música (melodia em onda quadrada, harmonia em
 * arpejo, baixo em onda triangular e bateria com ruído) e até seis
 * vozes de efeitos sonoros. As músicas são composições originais
 * escritas na notação abaixo.
 *
 * Notação: cada token é uma semicolcheia.
 *   C4, F#3, Bb5   nota (nome + oitava)
 *   .              sustenta a nota anterior
 *   -              pausa
 *   [ ... ]N       repete o trecho N vezes
 * Bateria: K = bumbo, S = caixa, h = chimbal, . ou - = nada.
 */
#include <math.h>
#include <string.h>

#include "audio.h"

#define MAX_STEPS 512
#define NOTE_REST 0
#define NOTE_HOLD 1

enum { CH_LEAD, CH_HARM, CH_BASS, CH_DRUM, CH_COUNT };

typedef struct {
    const char *ch[CH_COUNT];
    int bpm;
    int loop;
} SongDef;

/* ---------------------------------------------------------------- */
/* Músicas                                                           */
/* ---------------------------------------------------------------- */
static const SongDef SONGS[MUS_COUNT] = {
    /* MUS_TITLE: tema do menu, alegre (C - Am - F - G) */
    {
        {
            /* melodia */
            "E5 . G5 . C6 . . . B5 . G5 . E5 . . . "
            "A5 . . . G5 . E5 . C5 . . . E5 . . . "
            "F5 . A5 . C6 . A5 . G5 . F5 . A5 . . . "
            "G5 . . . . . D5 . G5 . B5 . D6 . . . "
            "E6 . D6 . C6 . G5 . E5 . G5 . C6 . . . "
            "C6 . B5 . A5 . E5 . A5 . . . C6 . . . "
            "A5 . G5 . F5 . A5 . G5 . . . D5 . . . "
            "C5 . E5 . G5 . C6 . . . . . - - - - ",
            /* harmonia */
            "[C4 E4 G4 C5 ]4 [A3 C4 E4 A4 ]4 [F3 A3 C4 F4 ]4 [G3 B3 D4 G4 ]4 "
            "[C4 E4 G4 C5 ]4 [A3 C4 E4 A4 ]4 [F3 A3 C4 F4 ]2 [G3 B3 D4 G4 ]2 "
            "[C4 E4 G4 C5 ]2 C5 . . . - - - - ",
            /* baixo */
            "[C2 . C3 . ]4 [A1 . A2 . ]4 [F1 . F2 . ]4 [G1 . G2 . ]4 "
            "[C2 . C3 . ]4 [A1 . A2 . ]4 [F1 . F2 . ]2 [G1 . G2 . ]2 "
            "[C2 . C3 . ]2 C2 . . . - - - - ",
            /* bateria */
            "[K . h . S . h . K . h . S . h . ]7 "
            "K . h . S . S S K . S . S S S S ",
        },
        132, 1
    },
    /* MUS_GAME: tema da partida, mais tranquilo (F - G - Em - Am) */
    {
        {
            "A5 . . . C6 . . . A5 . G5 . F5 . . . "
            "G5 . . . B5 . . . D6 . . . B5 . . . "
            "E5 . G5 . B5 . . . G5 . E5 . . . . . "
            "A5 . . . . . . . C6 . B5 . A5 . E5 . "
            "F5 . A5 . C6 . F6 . E6 . C6 . A5 . . . "
            "D6 . . . B5 . . . G5 . A5 . B5 . . . "
            "C6 . . . . . . . G5 . E5 . G5 . . . "
            "C6 . . . . . . . - - - - - - - - ",
            "[F4 . A4 . C5 . A4 . ]2 [G4 . B4 . D5 . B4 . ]2 "
            "[E4 . G4 . B4 . G4 . ]2 [A4 . C5 . E5 . C5 . ]2 "
            "[F4 . A4 . C5 . A4 . ]2 [G4 . B4 . D5 . B4 . ]2 "
            "[C5 . E5 . G5 . E5 . ]4 ",
            "F2 . . . F3 . . . F2 . . . C3 . . . "
            "G2 . . . G3 . . . G2 . . . D3 . . . "
            "E2 . . . E3 . . . E2 . . . B2 . . . "
            "A2 . . . A3 . . . A2 . . . E3 . . . "
            "F2 . . . F3 . . . F2 . . . C3 . . . "
            "G2 . . . G3 . . . G2 . . . D3 . . . "
            "[C3 . . . C4 . . . C3 . . . G2 . . . ]2 ",
            "[K . . h S . . h K . K h S . . h ]7 "
            "K . . h S . . h K . S S S . S S ",
        },
        112, 1
    },
    /* MUS_WIN: vinheta de vitória */
    {
        {
            "C5 . E5 . G5 . C6 . . . G5 . C6 . . . "
            "E6 . . . . . . . . . . . - - - - ",
            "[C4 E4 G4 C5 ]4 [E4 G4 C5 E5 ]2 C5 . . . - - - - ",
            "C3 . G2 . C3 . G2 . C3 . . . C3 . . . "
            "C2 . . . . . . . . . . . - - - - ",
            "K . . . K . . . K . S . K . S S "
            "K . . . . . . . . . . . - - - - ",
        },
        150, 0
    },
    /* MUS_LOSE: vinheta de derrota */
    {
        {
            "E5 . . . D#5 . . . D5 . . . C#5 . . . "
            "C5 . . . . . . . . . . . - - - - ",
            "A3 . . . G#3 . . . G3 . . . F#3 . . . "
            "F3 . . . . . . . - - - - - - - - ",
            "A2 . . . . . . . . . . . . . . . "
            "F2 . . . . . . . - - - - - - - - ",
            "[- ]16 K . . . - - - - - - - - - - - - ",
        },
        96, 0
    },
    /* MUS_DRAW: vinheta de empate */
    {
        {
            "G5 . E5 . C5 . E5 . G5 . . . . . . . "
            "D5 . . . . . . . - - - - - - - - ",
            "[C4 E4 G4 E4 ]4 [B3 D4 G4 D4 ]2 - - - - - - - - ",
            "C3 . . . . . . . C3 . . . . . . . "
            "G2 . . . . . . . - - - - - - - - ",
            "K . . . S . . . K . . . S . . . "
            "K . . . - - - - - - - - - - - - ",
        },
        120, 0
    },
};

/* ---------------------------------------------------------------- */
/* Efeitos sonoros (até 3 segmentos em sequência)                   */
/* ---------------------------------------------------------------- */
enum { W_SQUARE, W_TRI, W_NOISE };

typedef struct {
    int wave;
    float f0, f1;   /* frequência inicial/final (Hz) */
    float dur;      /* segundos */
    float vol;
    float duty;
} SfxSeg;

typedef struct {
    SfxSeg seg[3];
    int nseg;
} SfxDef;

static const SfxDef SFX[SFX_COUNT] = {
    [SFX_MOVE]    = { { { W_SQUARE, 880, 880, 0.035f, 0.16f, 0.5f } }, 1 },
    [SFX_TICK]    = { { { W_SQUARE, 1320, 1320, 0.02f, 0.07f, 0.25f } }, 1 },
    [SFX_SELECT]  = { { { W_SQUARE, 660, 660, 0.05f, 0.18f, 0.5f },
                        { W_SQUARE, 990, 990, 0.08f, 0.18f, 0.5f } }, 2 },
    [SFX_BACK]    = { { { W_SQUARE, 700, 380, 0.10f, 0.16f, 0.5f } }, 1 },
    [SFX_START]   = { { { W_SQUARE, 523, 523, 0.07f, 0.2f, 0.25f },
                        { W_SQUARE, 784, 784, 0.07f, 0.2f, 0.25f },
                        { W_SQUARE, 1047, 1047, 0.18f, 0.2f, 0.25f } }, 3 },
    [SFX_DROP]    = { { { W_TRI, 900, 300, 0.12f, 0.20f, 0.5f } }, 1 },
    [SFX_LAND]    = { { { W_NOISE, 2200, 900, 0.05f, 0.30f, 0.5f },
                        { W_TRI, 160, 60, 0.10f, 0.45f, 0.5f } }, 2 },
    [SFX_POP]     = { { { W_SQUARE, 250, 900, 0.10f, 0.20f, 0.25f },
                        { W_NOISE, 3000, 3000, 0.04f, 0.15f, 0.5f } }, 2 },
    [SFX_INVALID] = { { { W_SQUARE, 140, 140, 0.09f, 0.22f, 0.5f },
                        { W_SQUARE, 110, 110, 0.14f, 0.22f, 0.5f } }, 2 },
    [SFX_PAUSE]   = { { { W_SQUARE, 988, 988, 0.05f, 0.16f, 0.5f },
                        { W_SQUARE, 659, 659, 0.05f, 0.16f, 0.5f },
                        { W_SQUARE, 988, 988, 0.09f, 0.16f, 0.5f } }, 3 },
};

/* ---------------------------------------------------------------- */
/* Estado                                                            */
/* ---------------------------------------------------------------- */
typedef struct {
    unsigned char steps[CH_COUNT][MAX_STEPS];
    int len[CH_COUNT];
} SongData;

typedef struct {
    float phase, inc;
    float env;
    int on;
} Osc;

#define MAX_VOICES 6

typedef struct {
    const SfxDef *def;
    int seg;
    float t;
    float phase;
} Voice;

static struct {
    int rate;
    SongData songs[MUS_COUNT];
    float note_hz[128];

    int song;
    int playing;
    int music_on;
    int step;
    float step_left;
    float step_len;

    Osc lead, harm, bass;
    /* bateria */
    float kick_t, snare_t, hat_t;
    float kick_phase;

    unsigned int lfsr;
    float noise_hold;
    float noise_phase;

    Voice voices[MAX_VOICES];
    float vibrato;
} A;

/* ---------------------------------------------------------------- */
/* Leitura da notação                                                */
/* ---------------------------------------------------------------- */
static int expand(const char *src, char *dst, int cap)
{
    int n = 0;
    while (*src && n < cap - 1) {
        if (*src == '[') {
            const char *start = ++src, *end = start;
            int reps = 0, i;
            while (*end && *end != ']')
                end++;
            src = *end ? end + 1 : end;
            while (*src >= '0' && *src <= '9')
                reps = reps * 10 + (*src++ - '0');
            for (i = 0; i < reps; i++) {
                const char *p;
                for (p = start; p < end && n < cap - 2; p++)
                    dst[n++] = *p;
                dst[n++] = ' ';
            }
        } else {
            dst[n++] = *src++;
        }
    }
    dst[n] = 0;
    return n;
}

static int parse_note(const char *t, int len)
{
    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 }; /* A..G */
    int semi, oct, i = 1;
    if (t[0] < 'A' || t[0] > 'G')
        return NOTE_REST;
    semi = base[t[0] - 'A'];
    if (i < len && t[i] == '#') { semi++; i++; }
    else if (i < len && t[i] == 'b') { semi--; i++; }
    if (i >= len)
        return NOTE_REST;
    oct = t[i] - '0';
    return 12 * (oct + 1) + semi;
}

static int parse_channel(const char *src, unsigned char *out, int drum)
{
    static char buf[4096];
    const char *p;
    int n = 0;
    expand(src, buf, sizeof(buf));
    p = buf;
    while (*p && n < MAX_STEPS) {
        const char *t;
        int len;
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        t = p;
        while (*p && *p != ' ')
            p++;
        len = (int)(p - t);
        if (drum) {
            out[n++] = t[0] == 'K' ? 1 : t[0] == 'S' ? 2 : t[0] == 'h' ? 3 : 0;
        } else if (t[0] == '.') {
            out[n++] = NOTE_HOLD;
        } else if (t[0] == '-') {
            out[n++] = NOTE_REST;
        } else {
            out[n++] = (unsigned char)parse_note(t, len);
        }
    }
    return n;
}

/* ---------------------------------------------------------------- */
/* Música                                                            */
/* ---------------------------------------------------------------- */
static void note_on(Osc *o, int code)
{
    if (code == NOTE_HOLD)
        return;
    if (code == NOTE_REST) {
        o->on = 0;
        return;
    }
    o->inc = A.note_hz[code] / A.rate;
    o->env = 1.0f;
    o->on = 1;
}

static void do_step(void)
{
    const SongData *s = &A.songs[A.song];
    int len = s->len[CH_LEAD];

    if (A.step >= len) {
        if (SONGS[A.song].loop) {
            A.step = 0;
        } else {
            A.playing = 0;
            A.lead.on = A.harm.on = A.bass.on = 0;
            return;
        }
    }
    if (A.step < s->len[CH_LEAD]) note_on(&A.lead, s->steps[CH_LEAD][A.step]);
    if (A.step < s->len[CH_HARM]) note_on(&A.harm, s->steps[CH_HARM][A.step]);
    if (A.step < s->len[CH_BASS]) note_on(&A.bass, s->steps[CH_BASS][A.step]);
    if (A.step < s->len[CH_DRUM]) {
        int d = s->steps[CH_DRUM][A.step];
        if (d == 1) { A.kick_t = 0.0001f; A.kick_phase = 0; }
        else if (d == 2) A.snare_t = 0.0001f;
        else if (d == 3) A.hat_t = 0.0001f;
    }
    A.step++;
}

static float noise(void)
{
    unsigned int bit = ((A.lfsr >> 0) ^ (A.lfsr >> 1)) & 1;
    A.lfsr = (A.lfsr >> 1) | (bit << 14);
    return (A.lfsr & 1) ? 1.0f : -1.0f;
}

static float music_sample(void)
{
    float out = 0, dt = 1.0f / A.rate;

    if (!A.playing)
        return 0;

    A.step_left -= 1.0f;
    if (A.step_left <= 0) {
        A.step_left += A.step_len;
        do_step();
        if (!A.playing)
            return 0;
    }

    /* melodia: quadrada 25% com vibrato leve */
    A.vibrato += 5.5f * dt;
    if (A.vibrato > 1) A.vibrato -= 1;
    if (A.lead.on || A.lead.env > 0.001f) {
        float vib = 1.0f + 0.004f * sinf(A.vibrato * 6.2831853f);
        float target = A.lead.on ? 0.55f : 0.0f;
        float k = A.lead.on ? 0.9995f : 0.997f;
        A.lead.phase += A.lead.inc * vib;
        if (A.lead.phase >= 1) A.lead.phase -= 1;
        A.lead.env = target + (A.lead.env - target) * k;
        out += (A.lead.phase < 0.25f ? 1.0f : -1.0f) * A.lead.env * 0.20f;
    }

    /* harmonia: quadrada 12,5% em arpejo, curta */
    if (A.harm.on || A.harm.env > 0.001f) {
        A.harm.phase += A.harm.inc;
        if (A.harm.phase >= 1) A.harm.phase -= 1;
        A.harm.env *= A.harm.on ? 0.99975f : 0.996f;
        out += (A.harm.phase < 0.125f ? 1.0f : -1.0f) * A.harm.env * 0.08f;
    }

    /* baixo: triangular */
    if (A.bass.on || A.bass.env > 0.001f) {
        float tri;
        A.bass.phase += A.bass.inc;
        if (A.bass.phase >= 1) A.bass.phase -= 1;
        if (!A.bass.on)
            A.bass.env *= 0.997f;
        else
            A.bass.env = 0.85f + (A.bass.env - 0.85f) * 0.9998f;
        tri = A.bass.phase < 0.5f ? 4 * A.bass.phase - 1 : 3 - 4 * A.bass.phase;
        out += tri * A.bass.env * 0.30f;
    }

    /* bateria */
    if (A.kick_t > 0) {
        float f = 150.0f * expf(-A.kick_t * 30.0f) + 45.0f;
        A.kick_phase += f * dt;
        if (A.kick_phase >= 1) A.kick_phase -= 1;
        out += sinf(A.kick_phase * 6.2831853f) * expf(-A.kick_t * 18.0f) * 0.42f;
        A.kick_t += dt;
        if (A.kick_t > 0.25f) A.kick_t = 0;
    }
    if (A.snare_t > 0) {
        out += noise() * expf(-A.snare_t * 22.0f) * 0.16f;
        A.snare_t += dt;
        if (A.snare_t > 0.2f) A.snare_t = 0;
    }
    if (A.hat_t > 0) {
        float n = noise();
        out += (n - A.noise_hold) * 0.5f * expf(-A.hat_t * 90.0f) * 0.10f;
        A.noise_hold = n;
        A.hat_t += dt;
        if (A.hat_t > 0.05f) A.hat_t = 0;
    }
    return out;
}

/* ---------------------------------------------------------------- */
/* Efeitos                                                           */
/* ---------------------------------------------------------------- */
static float sfx_sample(void)
{
    float out = 0, dt = 1.0f / A.rate;
    int i;
    for (i = 0; i < MAX_VOICES; i++) {
        Voice *v = &A.voices[i];
        const SfxSeg *s;
        float u, f, env, x;
        if (!v->def)
            continue;
        s = &v->def->seg[v->seg];
        u = v->t / s->dur;
        f = s->f0 + (s->f1 - s->f0) * u;
        env = (1.0f - u);
        env = env * env * 0.7f + 0.3f * (1.0f - u);
        v->phase += f * dt;
        if (v->phase >= 1) {
            v->phase -= (float)(int)v->phase;
            if (s->wave == W_NOISE)
                A.noise_phase = noise();
        }
        if (s->wave == W_SQUARE)
            x = v->phase < s->duty ? 1.0f : -1.0f;
        else if (s->wave == W_TRI)
            x = v->phase < 0.5f ? 4 * v->phase - 1 : 3 - 4 * v->phase;
        else
            x = A.noise_phase;
        out += x * env * s->vol;

        v->t += dt;
        if (v->t >= s->dur) {
            v->t = 0;
            v->seg++;
            if (v->seg >= v->def->nseg)
                v->def = 0;
        }
    }
    return out;
}

/* ---------------------------------------------------------------- */
/* API                                                               */
/* ---------------------------------------------------------------- */
void audio_init(int rate)
{
    int i, s, c;
    memset(&A, 0, sizeof(A));
    A.rate = rate > 0 ? rate : AUDIO_RATE;
    A.lfsr = 0x4A3Bu;
    A.music_on = 1;
    A.song = MUS_NONE;
    for (i = 0; i < 128; i++)
        A.note_hz[i] = 440.0f * powf(2.0f, (i - 69) / 12.0f);
    for (s = 0; s < MUS_COUNT; s++)
        for (c = 0; c < CH_COUNT; c++)
            A.songs[s].len[c] = parse_channel(SONGS[s].ch[c],
                                               A.songs[s].steps[c],
                                               c == CH_DRUM);
}

void audio_music(int song)
{
    if (song == A.song && (A.playing || !SONGS[song].loop))
        return;
    A.song = song;
    A.lead.on = A.harm.on = A.bass.on = 0;
    A.lead.env = A.harm.env = A.bass.env = 0;
    A.kick_t = A.snare_t = A.hat_t = 0;
    if (song < 0 || song >= MUS_COUNT) {
        A.song = MUS_NONE;
        A.playing = 0;
        return;
    }
    A.step = 0;
    A.step_len = A.rate * 60.0f / SONGS[song].bpm / 4.0f;
    A.step_left = 0;
    A.playing = 1;
}

int audio_music_playing(void)
{
    return A.playing;
}

void audio_sfx(int id)
{
    int i, best = 0;
    float oldest = -1;
    if (id < 0 || id >= SFX_COUNT)
        return;
    for (i = 0; i < MAX_VOICES; i++) {
        if (!A.voices[i].def) {
            best = i;
            break;
        }
        if (A.voices[i].t > oldest) {
            oldest = A.voices[i].t;
            best = i;
        }
    }
    A.voices[best].def = &SFX[id];
    A.voices[best].seg = 0;
    A.voices[best].t = 0;
    A.voices[best].phase = 0;
}

void audio_set_music_enabled(int on)
{
    A.music_on = on;
}

int audio_music_enabled(void)
{
    return A.music_on;
}

void audio_mix(short *out, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        float m = music_sample();
        float v = (A.music_on ? m * 0.75f : 0.0f) + sfx_sample() * 0.8f;
        int s;
        if (v > 1) v = 1;
        if (v < -1) v = -1;
        s = (int)(v * 30000.0f);
        out[i] = (short)s;
    }
}

int audio_song_steps(int song, int channel)
{
    if (song < 0 || song >= MUS_COUNT || channel < 0 || channel >= CH_COUNT)
        return -1;
    return A.songs[song].len[channel];
}
