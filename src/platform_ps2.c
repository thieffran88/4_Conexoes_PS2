/*
 * Implementação da plataforma para o PlayStation 2 (PS2SDK + gsKit).
 */
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>

#include "assets.h"
#include "platform.h"

static GSGLOBAL *gs;
static GSTEXTURE textures[SPR_COUNT];
static char pad_buf[2][256] __attribute__((aligned(64)));

static u64 to_gs(Color c)
{
    unsigned int a = (c >> 24) & 0xFF;
    /* No GS, 0x80 é totalmente opaco. */
    a = (a * 128 + 127) / 255;
    return GS_SETREG_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, a,
                           0x00);
}

/* Cor de modulação de textura: no GS, 0x80 = 1.0. */
static u64 to_gs_tint(Color c)
{
    unsigned int r = ((c >> 16) & 0xFF) * 128 / 255;
    unsigned int g = ((c >> 8) & 0xFF) * 128 / 255;
    unsigned int b = (c & 0xFF) * 128 / 255;
    unsigned int a = ((c >> 24) & 0xFF) * 128 / 255;
    return GS_SETREG_RGBAQ(r, g, b, a, 0x00);
}

static void init_textures(void)
{
    int i;
    for (i = 0; i < SPR_COUNT; i++) {
        const AssetSprite *sp = &g_sprites[i];
        GSTEXTURE *t = &textures[i];
        memset(t, 0, sizeof(*t));
        t->Width = sp->w;
        t->Height = sp->h;
        t->PSM = sp->psm == ASSET_PSM_CT32 ? GS_PSM_CT32 : GS_PSM_CT16;
        t->Mem = (u32 *)(g_asset_pak + sp->offset);
        t->Filter = GS_FILTER_LINEAR;
    }
}

static void init_pads(void)
{
    int ret;

    SifInitRpc(0);
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0)
        printf("4 Conexoes: falha ao carregar SIO2MAN (%d)\n", ret);
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0)
        printf("4 Conexoes: falha ao carregar PADMAN (%d)\n", ret);

    padInit(0);
    padPortOpen(0, 0, pad_buf[0]);
    padPortOpen(1, 0, pad_buf[1]);
}

int plat_init(void)
{
    init_pads();

    gs = gsKit_init_global();
    gs->PSM = GS_PSM_CT24;
    gs->PSMZ = GS_PSMZ_16S;
    gs->ZBuffering = GS_SETTING_OFF;
    gs->DoubleBuffering = GS_SETTING_ON;
    gs->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, GS_ONESHOT);
    gsKit_set_test(gs, GS_ZTEST_OFF);
    /* Mistura alfa padrão: Cs*As + Cd*(1-As). */
    gsKit_set_primalpha(gs, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    /* O gerenciador envia as texturas para a VRAM conforme o uso. */
    gsKit_TexManager_init(gs);
    init_textures();
    return 0;
}

int plat_width(void) { return gs->Width; }
int plat_height(void) { return gs->Height; }

void plat_begin_frame(Color clear)
{
    gsKit_clear(gs, to_gs(clear));
}

void plat_rect(float x, float y, float w, float h, Color c)
{
    gsKit_prim_sprite(gs, x, y, x + w, y + h, 1, to_gs(c));
}

void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3,
              Color c)
{
    gsKit_prim_triangle(gs, x1, y1, x2, y2, x3, y3, 1, to_gs(c));
}

void plat_gradient(float x, float y, float w, float h, Color top,
                   Color bottom)
{
    u64 t = to_gs(top), b = to_gs(bottom);
    gsKit_prim_quad_gouraud(gs, x, y, x + w, y, x, y + h, x + w, y + h, 1,
                            t, t, b, b);
}

void plat_sprite(int id, float x, float y, float w, float h, float u0,
                 float v0, float u1, float v1, Color tint)
{
    GSTEXTURE *t = &textures[id];
    gsKit_TexManager_bind(gs, t);
    gsKit_prim_sprite_texture(gs, t, x, y, u0, v0, x + w, y + h, u1, v1, 1,
                              to_gs_tint(tint));
}

void plat_end_frame(void)
{
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
    gsKit_TexManager_nextFrame(gs);
}

int plat_pad_connected(int pad)
{
    int st = padGetState(pad, 0);
    return st == PAD_STATE_STABLE || st == PAD_STATE_FINDCTP1;
}

unsigned int plat_buttons(int pad)
{
    struct padButtonStatus status;
    unsigned int raw, out = 0;

    if (!plat_pad_connected(pad))
        return 0;
    if (padRead(pad, 0, &status) == 0)
        return 0;

    raw = 0xFFFF ^ status.btns; /* os bits vêm invertidos */
    if (raw & PAD_UP)       out |= BTN_UP;
    if (raw & PAD_DOWN)     out |= BTN_DOWN;
    if (raw & PAD_LEFT)     out |= BTN_LEFT;
    if (raw & PAD_RIGHT)    out |= BTN_RIGHT;
    if (raw & PAD_CROSS)    out |= BTN_CROSS;
    if (raw & PAD_CIRCLE)   out |= BTN_CIRCLE;
    if (raw & PAD_SQUARE)   out |= BTN_SQUARE;
    if (raw & PAD_TRIANGLE) out |= BTN_TRIANGLE;
    if (raw & PAD_START)    out |= BTN_START;
    if (raw & PAD_SELECT)   out |= BTN_SELECT;
    return out;
}
