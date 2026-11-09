/*
 * Testes do sintetizador: tamanho das músicas e sanidade do sinal.
 * Uso: ./tests/test_audio [pasta]  -> grava WAVs de cada música/efeito
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../src/audio.h"

static int failures = 0, checks = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        printf("FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

static void write_wav(const char *path, const short *s, int n)
{
    FILE *f = fopen(path, "wb");
    int rate = AUDIO_RATE, bytes = n * 2, v;
    if (!f)
        return;
    fwrite("RIFF", 1, 4, f);
    v = 36 + bytes; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f);
    { short fmt = 1, ch = 1, align = 2, bits = 16;
      fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
      fwrite(&rate, 4, 1, f); v = rate * 2; fwrite(&v, 4, 1, f);
      fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f); }
    fwrite("data", 1, 4, f);
    fwrite(&bytes, 4, 1, f);
    fwrite(s, 2, n, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : NULL;
    static short buf[AUDIO_RATE * 40];
    int song, c, id;

    audio_init(AUDIO_RATE);

    for (song = 0; song < MUS_COUNT; song++) {
        int len = audio_song_steps(song, 0);
        CHECK(len > 0 && len % 16 == 0);
        for (c = 1; c < 4; c++) {
            if (audio_song_steps(song, c) != len)
                printf("música %d canal %d: %d passos (melodia: %d)\n", song, c,
                       audio_song_steps(song, c), len);
            CHECK(audio_song_steps(song, c) == len);
        }
    }

    for (song = 0; song < MUS_COUNT; song++) {
        int n = song <= MUS_GAME ? AUDIO_RATE * 30 : AUDIO_RATE * 6;
        int i, peak = 0;
        double energy = 0;
        audio_init(AUDIO_RATE);
        audio_music(song);
        audio_mix(buf, n);
        for (i = 0; i < n; i++) {
            int a = abs(buf[i]);
            if (a > peak) peak = a;
            energy += (double)buf[i] * buf[i];
        }
        printf("música %d: pico %d, rms %.0f\n", song, peak,
               n ? sqrt(energy / n) : 0);
        CHECK(peak > 3000);
        CHECK(peak < 32000);
        if (song > MUS_GAME)
            CHECK(!audio_music_playing());
        if (dir) {
            char path[512];
            snprintf(path, sizeof(path), "%s/musica_%d.wav", dir, song);
            write_wav(path, buf, n);
        }
    }

    audio_init(AUDIO_RATE);
    for (id = 0; id < SFX_COUNT; id++) {
        int i, peak = 0, n = AUDIO_RATE / 2;
        audio_sfx(id);
        audio_mix(buf, n);
        for (i = 0; i < n; i++)
            if (abs(buf[i]) > peak) peak = abs(buf[i]);
        CHECK(peak > 1000);
        if (dir) {
            char path[512];
            snprintf(path, sizeof(path), "%s/efeito_%d.wav", dir, id);
            write_wav(path, buf, n);
        }
    }

    printf("%d verificações, %d falhas\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
