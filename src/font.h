/*
 * Fonte bitmap 5x7 com suporte a acentos do português.
 * Independente de plataforma: desenha via callback de retângulos.
 */
#ifndef FONT_H
#define FONT_H

typedef void (*FontRectFn)(float x, float y, float w, float h, void *user);

/* Largura em pixels de um texto UTF-8 com a escala dada. */
float font_width(const char *text, float scale);

/* Desenha o texto (UTF-8). (x, y) é o canto superior esquerdo da
 * linha de base das maiúsculas (acentos ficam acima de y). */
void font_draw(const char *text, float x, float y, float scale,
               FontRectFn rect, void *user);

#endif
