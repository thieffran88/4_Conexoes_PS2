/*
 * Camada de plataforma: o jogo (game.c) só conversa com estas funções.
 *  - platform_ps2.c : PlayStation 2 (gsKit + libpad)
 *  - platform_host.c: PC, renderização por software para testes/capturas
 */
#ifndef PLATFORM_H
#define PLATFORM_H

/* Botões do controle (máscara de bits). */
enum {
    BTN_UP       = 1 << 0,
    BTN_DOWN     = 1 << 1,
    BTN_LEFT     = 1 << 2,
    BTN_RIGHT    = 1 << 3,
    BTN_CROSS    = 1 << 4,
    BTN_CIRCLE   = 1 << 5,
    BTN_SQUARE   = 1 << 6,
    BTN_TRIANGLE = 1 << 7,
    BTN_START    = 1 << 8,
    BTN_SELECT   = 1 << 9
};

/* Cores no formato 0xAARRGGBB (AA = 0xFF opaco). */
typedef unsigned int Color;

#define RGB(r, g, b)     (0xFF000000u | ((r) << 16) | ((g) << 8) | (b))
#define RGBA(r, g, b, a) (((Color)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))

int  plat_init(void);
int  plat_width(void);
int  plat_height(void);

void plat_begin_frame(Color clear);
void plat_rect(float x, float y, float w, float h, Color c);
void plat_tri(float x1, float y1, float x2, float y2, float x3, float y3,
              Color c);
void plat_gradient(float x, float y, float w, float h, Color top,
                   Color bottom);
/* Desenha a sprite 'id' (ver assets.h) no retângulo (x, y, w, h),
 * usando a região (u0, v0)-(u1, v1) da textura, em texels. 'tint'
 * multiplica as cores (branco opaco = sem alteração). */
void plat_sprite(int id, float x, float y, float w, float h, float u0,
                 float v0, float u1, float v1, Color tint);
void plat_end_frame(void);

/* Botões pressionados no controle 'pad' (0 ou 1). */
unsigned int plat_buttons(int pad);
int plat_pad_connected(int pad);

#endif
