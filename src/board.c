#include <string.h>
#include "board.h"

static const int DIRS[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };

int board_other(int player)
{
    return player == PLAYER1 ? PLAYER2 : PLAYER1;
}

void board_init(Board *b, int first_player, int popout)
{
    memset(b, 0, sizeof(*b));
    b->current = first_player;
    b->popout = popout;
    b->result = RESULT_NONE;
}

int board_can_drop(const Board *b, int col)
{
    return col >= 0 && col < BOARD_COLS && b->heights[col] < BOARD_ROWS;
}

int board_can_pop(const Board *b, int col, int player)
{
    return b->popout && col >= 0 && col < BOARD_COLS &&
           b->heights[col] > 0 && b->cells[0][col] == player;
}

int board_has_any_move(const Board *b, int player)
{
    int c;
    for (c = 0; c < BOARD_COLS; c++)
        if (board_can_drop(b, c) || board_can_pop(b, c, player))
            return 1;
    return 0;
}

static int in_bounds(int r, int c)
{
    return r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS;
}

int board_has_line(const Board *b, int player)
{
    int r, c, d, k;
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++) {
            if (b->cells[r][c] != player)
                continue;
            for (d = 0; d < 4; d++) {
                for (k = 1; k < CONNECT_N; k++) {
                    int rr = r + DIRS[d][0] * k, cc = c + DIRS[d][1] * k;
                    if (!in_bounds(rr, cc) || b->cells[rr][cc] != player)
                        break;
                }
                if (k == CONNECT_N)
                    return 1;
            }
        }
    return 0;
}

void board_collect_win(Board *b, int player)
{
    unsigned char mark[BOARD_ROWS][BOARD_COLS];
    int r, c, d, k;

    memset(mark, 0, sizeof(mark));
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++) {
            if (b->cells[r][c] != player)
                continue;
            for (d = 0; d < 4; d++) {
                for (k = 1; k < CONNECT_N; k++) {
                    int rr = r + DIRS[d][0] * k, cc = c + DIRS[d][1] * k;
                    if (!in_bounds(rr, cc) || b->cells[rr][cc] != player)
                        break;
                }
                if (k == CONNECT_N)
                    for (k = 0; k < CONNECT_N; k++)
                        mark[r + DIRS[d][0] * k][c + DIRS[d][1] * k] = 1;
            }
        }

    b->win_count = 0;
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++)
            if (mark[r][c]) {
                b->win_r[b->win_count] = r;
                b->win_c[b->win_count] = c;
                b->win_count++;
            }
}

static void update_result(Board *b, int mover)
{
    int other = board_other(mover);
    int mover_line = board_has_line(b, mover);
    int other_line = b->popout ? board_has_line(b, other) : 0;

    if (mover_line) {
        /* Inclui o caso em que os dois formam linha numa retirada:
         * vence quem fez a jogada. */
        b->result = mover == PLAYER1 ? RESULT_WIN1 : RESULT_WIN2;
        board_collect_win(b, mover);
    } else if (other_line) {
        /* Uma retirada pode completar a linha do adversário. */
        b->result = other == PLAYER1 ? RESULT_WIN1 : RESULT_WIN2;
        board_collect_win(b, other);
    } else if (!b->popout && b->moves >= BOARD_ROWS * BOARD_COLS) {
        b->result = RESULT_DRAW;
    } else if (b->popout && (b->moves >= POPOUT_MAX_MOVES ||
                             !board_has_any_move(b, other))) {
        b->result = RESULT_DRAW;
    }
}

int board_play(Board *b, int col, int type)
{
    int mover = b->current, row, r;

    if (b->result != RESULT_NONE)
        return -1;

    if (type == MOVE_DROP) {
        if (!board_can_drop(b, col))
            return -1;
        row = b->heights[col]++;
        b->cells[row][col] = (unsigned char)mover;
    } else {
        if (!board_can_pop(b, col, mover))
            return -1;
        for (r = 0; r < BOARD_ROWS - 1; r++)
            b->cells[r][col] = b->cells[r + 1][col];
        b->cells[BOARD_ROWS - 1][col] = EMPTY;
        b->heights[col]--;
        row = 0;
    }

    b->moves++;
    update_result(b, mover);
    if (b->result == RESULT_NONE)
        b->current = board_other(mover);
    return row;
}
