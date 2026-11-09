#include "assets.h"
#include "bigfont.h"

#define SPACE_ADV 0.40f

static unsigned int next_cp(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    unsigned int cp;
    if (s[0] < 0x80) {
        cp = s[0];
        *p += 1;
    } else if ((s[0] & 0xE0) == 0xC0 && s[1]) {
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2;
    } else if ((s[0] & 0xF0) == 0xE0 && s[1] && s[2]) {
        cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3;
    } else {
        cp = '?';
        *p += 1;
    }
    return cp;
}

static const BigGlyph *find(unsigned int cp)
{
    int i;
    for (i = 0; i < g_glyph_count; i++)
        if (g_glyphs[i].cp == cp)
            return &g_glyphs[i];
    return 0;
}

/* Letra base + acento (0 = nenhum). */
static unsigned int decompose(unsigned int cp, unsigned int *accent)
{
    *accent = 0;
    if (cp >= 'a' && cp <= 'z')
        return cp - 32;
    if (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7)
        cp -= 0x20;
    switch (cp) {
    case 0xC1: *accent = BIGFONT_ACUTE; return 'A';
    case 0xC0: *accent = BIGFONT_GRAVE; return 'A';
    case 0xC2: *accent = BIGFONT_CIRC;  return 'A';
    case 0xC3: *accent = BIGFONT_TILDE; return 'A';
    case 0xC9: *accent = BIGFONT_ACUTE; return 'E';
    case 0xCA: *accent = BIGFONT_CIRC;  return 'E';
    case 0xCD: *accent = BIGFONT_ACUTE; return 'I';
    case 0xD3: *accent = BIGFONT_ACUTE; return 'O';
    case 0xD4: *accent = BIGFONT_CIRC;  return 'O';
    case 0xD5: *accent = BIGFONT_TILDE; return 'O';
    case 0xDA: *accent = BIGFONT_ACUTE; return 'U';
    case 0xD1: *accent = BIGFONT_TILDE; return 'N';
    case 0xC7: *accent = BIGFONT_CEDIL; return 'C';
    default: return cp;
    }
}

float bigfont_width(const char *text, float height)
{
    float s = height / BIGFONT_CAP, w = 0;
    while (*text) {
        unsigned int acc;
        unsigned int cp = decompose(next_cp(&text), &acc);
        const BigGlyph *g = cp == ' ' ? 0 : find(cp);
        w += g ? g->adv * s : BIGFONT_CAP * SPACE_ADV * s;
    }
    return w;
}

static void glyph(const BigGlyph *g, float x, float y, float s, Color tint)
{
    plat_sprite(SPR_FONT, x, y, g->w * s, g->h * s, (float)g->x, (float)g->y,
                (float)(g->x + g->w), (float)(g->y + g->h), tint);
}

void bigfont_draw(const char *text, float x, float y, float height, Color tint)
{
    float s = height / BIGFONT_CAP;
    while (*text) {
        unsigned int acc;
        unsigned int cp = decompose(next_cp(&text), &acc);
        const BigGlyph *g = cp == ' ' ? 0 : find(cp);
        if (!g) {
            x += BIGFONT_CAP * SPACE_ADV * s;
            continue;
        }
        glyph(g, x, y + g->yoff * s, s, tint);
        if (acc) {
            const BigGlyph *a = find(acc);
            if (a) {
                float ax = x + (g->w - a->w) * s / 2;
                float ay = acc == BIGFONT_CEDIL
                    ? y + (BIGFONT_CAP - 3) * s
                    : y + g->yoff * s - a->h * s + 4 * s;
                glyph(a, ax, ay, s, tint);
            }
        }
        x += g->adv * s;
    }
}

void bigfont_draw_center(const char *text, float cx, float y, float height,
                         Color tint)
{
    bigfont_draw(text, cx - bigfont_width(text, height) / 2, y, height, tint);
}
