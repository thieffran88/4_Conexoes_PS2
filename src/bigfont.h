/*
 * Fonte grande (amarela com contorno), recortada da arte do jogo.
 * Letras acentuadas são montadas com a letra base + acento.
 */
#ifndef BIGFONT_H
#define BIGFONT_H

#include "platform.h"

/* 'height' é a altura das maiúsculas em pixels. */
float bigfont_width(const char *text, float height);
void bigfont_draw(const char *text, float x, float y, float height, Color tint);
void bigfont_draw_center(const char *text, float cx, float y, float height,
                         Color tint);

#endif
