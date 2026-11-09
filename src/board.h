/*
 * 4 Conexões - lógica do tabuleiro (C puro, sem dependências do PS2).
 *
 * Regras clássicas:
 *  - Grade vertical de 7 colunas x 6 linhas.
 *  - Dois jogadores alternam turnos soltando uma peça numa coluna;
 *    a peça cai até a posição vazia mais baixa.
 *  - Vence quem alinhar 4 peças da sua cor na horizontal, vertical
 *    ou diagonal.
 *  - Se a grade encher sem nenhuma linha de 4, é empate.
 *
 * Variante "Retirada" (inspirada no Pop Out):
 *  - Na sua vez, o jogador pode soltar uma peça OU retirar uma peça
 *    da sua cor da linha de baixo; as peças da coluna descem uma casa.
 *  - Se uma retirada formar 4 em linha para os dois jogadores, vence
 *    quem fez a retirada.
 *  - Empate se o jogador da vez não tiver jogada válida ou se o limite
 *    de jogadas for atingido (evita partidas infinitas).
 */
#ifndef BOARD_H
#define BOARD_H

#define BOARD_COLS 7
#define BOARD_ROWS 6
#define CONNECT_N  4

/* Limite de jogadas na variante Retirada. */
#define POPOUT_MAX_MOVES 120

enum { EMPTY = 0, PLAYER1 = 1, PLAYER2 = 2 };

enum { MOVE_DROP = 0, MOVE_POP = 1 };

enum {
    RESULT_NONE = 0,
    RESULT_WIN1 = 1,
    RESULT_WIN2 = 2,
    RESULT_DRAW = 3
};

typedef struct {
    /* cells[linha][coluna]; linha 0 é a de BAIXO. */
    unsigned char cells[BOARD_ROWS][BOARD_COLS];
    unsigned char heights[BOARD_COLS];
    int current;   /* PLAYER1 ou PLAYER2 */
    int moves;     /* jogadas feitas */
    int popout;    /* 1 = variante Retirada habilitada */
    int result;    /* RESULT_* */
    /* casas da linha vencedora (válidas quando há vencedor) */
    int win_count;
    int win_r[BOARD_ROWS * BOARD_COLS];
    int win_c[BOARD_ROWS * BOARD_COLS];
} Board;

void board_init(Board *b, int first_player, int popout);

int board_can_drop(const Board *b, int col);
int board_can_pop(const Board *b, int col, int player);
int board_has_any_move(const Board *b, int player);

/* Aplica uma jogada do jogador da vez. Retorna a linha onde a peça
 * caiu (drop) ou 0 (pop); -1 se a jogada for inválida. */
int board_play(Board *b, int col, int type);

/* Verifica se 'player' tem alguma linha de 4 no tabuleiro. */
int board_has_line(const Board *b, int player);

/* Preenche win_r/win_c com todas as casas do jogador em linhas de 4. */
void board_collect_win(Board *b, int player);

int board_other(int player);

#endif
