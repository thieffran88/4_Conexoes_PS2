#ifndef AI_H
#define AI_H

#include "board.h"

enum { AI_EASY = 0, AI_MEDIUM = 1, AI_HARD = 2 };

typedef struct {
    int col;
    int type; /* MOVE_DROP ou MOVE_POP */
} AiMove;

/* Escolhe a jogada do jogador da vez em 'b'. 'seed' alimenta a
 * aleatoriedade usada para variar as partidas. */
AiMove ai_choose(const Board *b, int level, unsigned int *seed);

#endif
