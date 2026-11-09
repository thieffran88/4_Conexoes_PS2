/* Gerado por tools/make_assets.py - não edite. */
#ifndef ASSETS_H
#define ASSETS_H

#define ASSET_PSM_CT32 0
#define ASSET_PSM_CT16 2

enum {
    SPR_LOGO,
    SPR_BOY_BIG,
    SPR_GIRL_BIG,
    SPR_BOY_IDLE,
    SPR_BOY_CHEER,
    SPR_BOY_SAD,
    SPR_BOY_THINK,
    SPR_BOY_SURPRISE,
    SPR_GIRL_IDLE,
    SPR_GIRL_CHEER,
    SPR_GIRL_SAD,
    SPR_GIRL_THINK,
    SPR_GIRL_SURPRISE,
    SPR_BOY_FACE0,
    SPR_BOY_FACE1,
    SPR_BOY_FACE2,
    SPR_BOY_FACE3,
    SPR_BOY_FACE4,
    SPR_BOY_FACE5,
    SPR_BOY_FACE6,
    SPR_BOY_FACE7,
    SPR_GIRL_FACE0,
    SPR_GIRL_FACE1,
    SPR_GIRL_FACE2,
    SPR_GIRL_FACE3,
    SPR_GIRL_FACE4,
    SPR_GIRL_FACE5,
    SPR_GIRL_FACE6,
    SPR_GIRL_FACE7,
    SPR_DISC_RED,
    SPR_DISC_YELLOW,
    SPR_ICON_1P,
    SPR_ICON_2P,
    SPR_ICON_PAD,
    SPR_ICON_TROPHY,
    SPR_ICON_CROWN,
    SPR_ICON_HELP,
    SPR_ICON_SOUND,
    SPR_ICON_STAR,
    SPR_BTN_CROSS,
    SPR_BTN_CIRCLE,
    SPR_BTN_TRIANGLE,
    SPR_BTN_SQUARE,
    SPR_COVER,
    SPR_FONT,
    SPR_COUNT
};

typedef struct {
    unsigned int offset;
    unsigned short w, h;
    unsigned char psm;
} AssetSprite;

typedef struct {
    unsigned short cp;
    short x, y, w, h, yoff, adv;
} BigGlyph;

#define BIGFONT_CAP 34
#define BIGFONT_ACUTE 0xE000
#define BIGFONT_GRAVE 0xE001
#define BIGFONT_CIRC 0xE002
#define BIGFONT_TILDE 0xE003
#define BIGFONT_CEDIL 0xE004

extern const AssetSprite g_sprites[SPR_COUNT];
extern const BigGlyph g_glyphs[];
extern const int g_glyph_count;
extern const unsigned char g_asset_pak[];

#endif
