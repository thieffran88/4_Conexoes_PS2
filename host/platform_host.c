/*
 * Plataforma "host" (PC): renderização por software em 640x448 e
 * controles roteirizados. Usada para testar o jogo e gerar capturas
 * de tela sem um PS2/emulador.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

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

void plat_end_frame(void) {}

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
