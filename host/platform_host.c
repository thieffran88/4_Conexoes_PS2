/*
 * Plataforma "host" (PC): renderização por software em 640x448 e
 * controles roteirizados. Usada para testar o jogo e gerar capturas
 * de tela sem um PS2/emulador.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "../src/assets.h"
#include "../src/audio.h"
#include "../src/platform.h"

#define W 640
#define H 448

static float fb[H][W][3];
static unsigned int buttons[2];
static int connected[2] = { 1, 0 };

void host_set_buttons(int pad, unsigned int b) { buttons[pad] = b; }
void host_set_connected(int pad, int c) { connected[pad] = c; }

int plat_init(void) { return 0; }
int plat_width(void) { return W; }
int plat_height(void) { return H; }

static void blend(int x, int y, Color c)
{
    float a = ((c >> 24) & 0xFF) / 255.0f;
    float r = ((c >> 16) & 0xFF) / 255.0f;
    float g = ((c >> 8) & 0xFF) / 255.0f;
    float b = (c & 0xFF) / 255.0f;
    if (x < 0 || x >= W || y < 0 || y >= H)
        return;
    fb[y][x][0] = r * a + fb[y][x][0] * (1 - a);
    fb[y][x][1] = g * a + fb[y][x][1] * (1 - a);
    fb[y][x][2] = b * a + fb[y][x][2] * (1 - a);
}

void plat_begin_frame(Color clear)
{
    int x, y;
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            fb[y][x][0] = ((clear >> 16) & 0xFF) / 255.0f;
            fb[y][x][1] = ((clear >> 8) & 0xFF) / 255.0f;
            fb[y][x][2] = (clear & 0xFF) / 255.0f;
        }
}

void plat_rect(float x, float y, float w, float h, Color c)
{
    int x0 = (int)floorf(x + 0.5f), y0 = (int)floorf(y + 0.5f);
    int x1 = (int)floorf(x + w + 0.5f), y1 = (int)floorf(y + h + 0.5f);
    int i, j;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++)
            blend(i, j, c);
}

static float edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3,
              Color c)
{
    float minx = fminf(x1, fminf(x2, x3)), maxx = fmaxf(x1, fmaxf(x2, x3));
    float miny = fminf(y1, fminf(y2, y3)), maxy = fmaxf(y1, fmaxf(y2, y3));
    float area = edge(x1, y1, x2, y2, x3, y3);
    int x, y;
    if (fabsf(area) < 1e-6f)
        return;
    for (y = (int)floorf(miny); y <= (int)ceilf(maxy); y++)
        for (x = (int)floorf(minx); x <= (int)ceilf(maxx); x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = edge(x2, y2, x3, y3, px, py);
            float w1 = edge(x3, y3, x1, y1, px, py);
            float w2 = edge(x1, y1, x2, y2, px, py);
            if (area < 0) { w0 = -w0; w1 = -w1; w2 = -w2; }
            if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                blend(x, y, c);
        }
}

void plat_gradient(float x, float y, float w, float h, Color top,
                   Color bottom)
{
    int j;
    for (j = 0; j < (int)h; j++) {
        float t = h > 1 ? j / (h - 1) : 0;
        unsigned int r = (unsigned int)(((top >> 16) & 0xFF) * (1 - t) + ((bottom >> 16) & 0xFF) * t);
        unsigned int g = (unsigned int)(((top >> 8) & 0xFF) * (1 - t) + ((bottom >> 8) & 0xFF) * t);
        unsigned int b = (unsigned int)((top & 0xFF) * (1 - t) + (bottom & 0xFF) * t);
        plat_rect(x, y + j, w, 1, RGB(r, g, b));
    }
}

/* Texturas decodificadas (RGBA em 0..1, alfa não pré-multiplicado). */
static float *tex_cache[SPR_COUNT];

static const float *texture(int id)
{
    const AssetSprite *sp = &g_sprites[id];
    const unsigned char *src = g_asset_pak + sp->offset;
    int i, n = sp->w * sp->h;
    float *t;

    if (tex_cache[id])
        return tex_cache[id];
    t = malloc(sizeof(float) * 4 * n);
    for (i = 0; i < n; i++) {
        if (sp->psm == ASSET_PSM_CT32) {
            t[i * 4 + 0] = src[i * 4 + 0] / 255.0f;
            t[i * 4 + 1] = src[i * 4 + 1] / 255.0f;
            t[i * 4 + 2] = src[i * 4 + 2] / 255.0f;
            t[i * 4 + 3] = src[i * 4 + 3] >= 128 ? 1.0f : src[i * 4 + 3] / 128.0f;
        } else {
            unsigned int v = src[i * 2] | (src[i * 2 + 1] << 8);
            t[i * 4 + 0] = ((v >> 0) & 31) / 31.0f;
            t[i * 4 + 1] = ((v >> 5) & 31) / 31.0f;
            t[i * 4 + 2] = ((v >> 10) & 31) / 31.0f;
            t[i * 4 + 3] = 1.0f;
        }
    }
    tex_cache[id] = t;
    return t;
}

static void texel(const float *t, int w, int h, int x, int y, float *out)
{
    const float *p;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= w) x = w - 1;
    if (y >= h) y = h - 1;
    p = t + (y * w + x) * 4;
    out[0] = p[0]; out[1] = p[1]; out[2] = p[2]; out[3] = p[3];
}

void plat_sprite(int id, float x, float y, float w, float h, float u0,
                 float v0, float u1, float v1, Color tint)
{
    const AssetSprite *sp = &g_sprites[id];
    const float *t = texture(id);
    float tr = ((tint >> 16) & 0xFF) / 255.0f, tg = ((tint >> 8) & 0xFF) / 255.0f;
    float tb = (tint & 0xFF) / 255.0f, ta = ((tint >> 24) & 0xFF) / 255.0f;
    int px0 = (int)floorf(x + 0.5f), py0 = (int)floorf(y + 0.5f);
    int px1 = (int)floorf(x + w + 0.5f), py1 = (int)floorf(y + h + 0.5f);
    int px, py;

    if (w <= 0 || h <= 0)
        return;
    for (py = py0; py < py1; py++) {
        float v = v0 + ((py + 0.5f) - y) / h * (v1 - v0) - 0.5f;
        int iy = (int)floorf(v);
        float fy = v - iy;
        if (py < 0 || py >= H)
            continue;
        for (px = px0; px < px1; px++) {
            float u = u0 + ((px + 0.5f) - x) / w * (u1 - u0) - 0.5f;
            int ix = (int)floorf(u);
            float fx = u - ix, a[4], b[4], c[4], d[4], col[4];
            int k;
            if (px < 0 || px >= W)
                continue;
            texel(t, sp->w, sp->h, ix, iy, a);
            texel(t, sp->w, sp->h, ix + 1, iy, b);
            texel(t, sp->w, sp->h, ix, iy + 1, c);
            texel(t, sp->w, sp->h, ix + 1, iy + 1, d);
            for (k = 0; k < 4; k++)
                col[k] = (a[k] * (1 - fx) + b[k] * fx) * (1 - fy) +
                         (c[k] * (1 - fx) + d[k] * fx) * fy;
            col[0] *= tr; col[1] *= tg; col[2] *= tb; col[3] *= ta;
            if (col[3] <= 0.002f)
                continue;
            fb[py][px][0] = col[0] * col[3] + fb[py][px][0] * (1 - col[3]);
            fb[py][px][1] = col[1] * col[3] + fb[py][px][1] * (1 - col[3]);
            fb[py][px][2] = col[2] * col[3] + fb[py][px][2] * (1 - col[3]);
        }
    }
}

/* Áudio: cada quadro gera 1/60 s de amostras e guarda para gravar em WAV. */
static short *audio_buf;
static int audio_len, audio_cap;
static double audio_acc;

static void audio_frame(int n)
{
    if (audio_len + n > audio_cap) {
        audio_cap = (audio_len + n) * 2 + 4096;
        audio_buf = realloc(audio_buf, sizeof(short) * audio_cap);
    }
    audio_mix(audio_buf + audio_len, n);
    audio_len += n;
}

void plat_audio_long_task(void) {}

void plat_end_frame(void)
{
    int n;
    audio_acc += AUDIO_RATE / 60.0;
    n = (int)audio_acc;
    audio_acc -= n;
    audio_frame(n);
}

int host_save_wav(const char *path)
{
    FILE *f = fopen(path, "wb");
    int rate = AUDIO_RATE, bytes = audio_len * 2, v;
    short fmt = 1, ch = 1, align = 2, bits = 16;
    if (!f)
        return -1;
    fwrite("RIFF", 1, 4, f);
    v = 36 + bytes; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f);
    fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f); v = rate * 2; fwrite(&v, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&bytes, 4, 1, f);
    fwrite(audio_buf, 2, audio_len, f);
    fclose(f);
    return 0;
}

unsigned int plat_buttons(int pad) { return connected[pad] ? buttons[pad] : 0; }
int plat_pad_connected(int pad) { return connected[pad]; }

int host_save_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int x, y;
    if (!f)
        return -1;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            unsigned char px[3];
            int k;
            for (k = 0; k < 3; k++) {
                float v = fb[y][x][k];
                px[k] = (unsigned char)(v < 0 ? 0 : v > 1 ? 255 : v * 255 + 0.5f);
            }
            fwrite(px, 1, 3, f);
        }
    fclose(f);
    return 0;
}
