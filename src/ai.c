/*
 * Adversário controlado pelo console: negamax com poda alfa-beta e
 * uma heurística por "janelas" de 4 casas.
 */
#include <string.h>
#include "ai.h"

#define WIN_SCORE 1000000
#define MAX_MOVES (BOARD_COLS * 2)

typedef struct {
    unsigned char cells[BOARD_ROWS][BOARD_COLS];
    unsigned char heights[BOARD_COLS];
    int popout;
} State;

/* Ordem de exploração: colunas centrais primeiro. */
static const int ORDER[BOARD_COLS] = { 3, 2, 4, 1, 5, 0, 6 };

static unsigned int rnd(unsigned int *s)
{
    *s = *s * 1103515245u + 12345u;
    return (*s >> 16) & 0x7fff;
}

static int other(int p) { return p == PLAYER1 ? PLAYER2 : PLAYER1; }

static int line_at(const State *s, int r, int c, int p)
{
    static const int D[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
    int d;
    for (d = 0; d < 4; d++) {
        int n = 1, k;
        for (k = 1; k < CONNECT_N; k++) {
            int rr = r + D[d][0] * k, cc = c + D[d][1] * k;
            if (rr < 0 || rr >= BOARD_ROWS || cc < 0 || cc >= BOARD_COLS ||
                s->cells[rr][cc] != p)
                break;
            n++;
        }
        for (k = 1; k < CONNECT_N; k++) {
            int rr = r - D[d][0] * k, cc = c - D[d][1] * k;
            if (rr < 0 || rr >= BOARD_ROWS || cc < 0 || cc >= BOARD_COLS ||
                s->cells[rr][cc] != p)
                break;
            n++;
        }
        if (n >= CONNECT_N)
            return 1;
    }
    return 0;
}

static int any_line(const State *s, int p)
{
    int r, c;
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++)
            if (s->cells[r][c] == p && line_at(s, r, c, p))
                return 1;
    return 0;
}

static int score_window(int mine, int theirs, int empty)
{
    if (mine && theirs)
        return 0;
    if (mine == 3 && empty == 1) return 6;
    if (mine == 2 && empty == 2) return 2;
    if (theirs == 3 && empty == 1) return -7;
    if (theirs == 2 && empty == 2) return -2;
    return 0;
}

static int evaluate(const State *s, int p)
{
    int r, c, k, score = 0;
    int q = other(p);

    for (r = 0; r < BOARD_ROWS; r++) {
        if (s->cells[r][3] == p) score += 3;
        else if (s->cells[r][3] == q) score -= 3;
    }

    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++) {
            static const int D[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
            int d;
            for (d = 0; d < 4; d++) {
                int er = r + D[d][0] * 3, ec = c + D[d][1] * 3;
                int mine = 0, theirs = 0, empty = 0;
                if (er < 0 || er >= BOARD_ROWS || ec < 0 || ec >= BOARD_COLS)
                    continue;
                for (k = 0; k < CONNECT_N; k++) {
                    int v = s->cells[r + D[d][0] * k][c + D[d][1] * k];
                    if (v == p) mine++;
                    else if (v == q) theirs++;
                    else empty++;
                }
                score += score_window(mine, theirs, empty);
            }
        }
    return score;
}

static int gen_moves(const State *s, int p, AiMove *out)
{
    int n = 0, i;
    for (i = 0; i < BOARD_COLS; i++) {
        int c = ORDER[i];
        if (s->heights[c] < BOARD_ROWS) {
            out[n].col = c;
            out[n].type = MOVE_DROP;
            n++;
        }
    }
    if (s->popout)
        for (i = 0; i < BOARD_COLS; i++) {
            int c = ORDER[i];
            if (s->heights[c] > 0 && s->cells[0][c] == p) {
                out[n].col = c;
                out[n].type = MOVE_POP;
                n++;
            }
        }
    return n;
}

/* Aplica a jogada e devolve: 1 = 'p' venceu, -1 = adversário venceu,
 * 0 = segue o jogo. 'undo' guarda a coluna para desfazer. */
static int apply(State *s, AiMove m, int p, unsigned char *undo)
{
    int c = m.col, r;
    for (r = 0; r < BOARD_ROWS; r++)
        undo[r] = s->cells[r][c];
    undo[BOARD_ROWS] = s->heights[c];

    if (m.type == MOVE_DROP) {
        r = s->heights[c]++;
        s->cells[r][c] = (unsigned char)p;
        return line_at(s, r, c, p) ? 1 : 0;
    }

    for (r = 0; r < BOARD_ROWS - 1; r++)
        s->cells[r][c] = s->cells[r + 1][c];
    s->cells[BOARD_ROWS - 1][c] = EMPTY;
    s->heights[c]--;
    if (any_line(s, p))
        return 1;
    if (any_line(s, other(p)))
        return -1;
    return 0;
}

static void undo_move(State *s, int c, const unsigned char *undo)
{
    int r;
    for (r = 0; r < BOARD_ROWS; r++)
        s->cells[r][c] = undo[r];
    s->heights[c] = undo[BOARD_ROWS];
}

static int full(const State *s)
{
    int c;
    for (c = 0; c < BOARD_COLS; c++)
        if (s->heights[c] < BOARD_ROWS)
            return 0;
    return 1;
}

static int negamax(State *s, int depth, int alpha, int beta, int p)
{
    AiMove moves[MAX_MOVES];
    unsigned char undo[BOARD_ROWS + 1];
    int n, i, best = -WIN_SCORE * 2;

    if (!s->popout && full(s))
        return 0;
    if (depth == 0)
        return evaluate(s, p);

    n = gen_moves(s, p, moves);
    if (n == 0)
        return 0;

    for (i = 0; i < n; i++) {
        int res = apply(s, moves[i], p, undo), val;
        if (res == 1)
            val = WIN_SCORE + depth;
        else if (res == -1)
            val = -WIN_SCORE - depth;
        else
            val = -negamax(s, depth - 1, -beta, -alpha, other(p));
        undo_move(s, moves[i].col, undo);

        if (val > best)
            best = val;
        if (best > alpha)
            alpha = best;
        if (alpha >= beta)
            break;
    }
    return best;
}

AiMove ai_choose(const Board *b, int level, unsigned int *seed)
{
    static const int DEPTH[3] = { 2, 4, 6 };
    State s;
    AiMove moves[MAX_MOVES], best;
    unsigned char undo[BOARD_ROWS + 1];
    int scores[MAX_MOVES];
    int n, i, p = b->current, best_score = -WIN_SCORE * 3, ties = 0;
    int depth;

    if (level < AI_EASY) level = AI_EASY;
    if (level > AI_HARD) level = AI_HARD;
    depth = DEPTH[level];
    /* A variante Retirada tem mais jogadas por turno: reduz um nível. */
    if (b->popout && depth > 2)
        depth--;

    memcpy(s.cells, b->cells, sizeof(s.cells));
    memcpy(s.heights, b->heights, sizeof(s.heights));
    s.popout = b->popout;

    n = gen_moves(&s, p, moves);
    best = moves[0];
    if (n == 0) {
        best.col = -1;
        best.type = MOVE_DROP;
        return best;
    }

    for (i = 0; i < n; i++) {
        int res = apply(&s, moves[i], p, undo);
        if (res == 1)
            scores[i] = WIN_SCORE + depth;
        else if (res == -1)
            scores[i] = -WIN_SCORE - depth;
        else
            scores[i] = -negamax(&s, depth - 1, -WIN_SCORE * 3,
                                 WIN_SCORE * 3, other(p));
        undo_move(&s, moves[i].col, undo);
    }

    /* No fácil, às vezes joga uma coluna aleatória (exceto se puder
     * vencer imediatamente). */
    if (level == AI_EASY && (rnd(seed) % 100) < 30) {
        for (i = 0; i < n; i++)
            if (scores[i] > WIN_SCORE / 2)
                return moves[i];
        return moves[rnd(seed) % n];
    }

    /* Melhor pontuação; empates sorteados para variar as partidas. */
    for (i = 0; i < n; i++) {
        if (scores[i] > best_score) {
            best_score = scores[i];
            best = moves[i];
            ties = 1;
        } else if (scores[i] == best_score) {
            ties++;
            if (rnd(seed) % ties == 0)
                best = moves[i];
        }
    }
    return best;
}
