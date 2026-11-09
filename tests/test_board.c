/*
 * Testes da lógica de regras e da IA (rodam no PC: make -f Makefile.host test).
 */
#include <stdio.h>
#include <stdlib.h>

#include "../src/ai.h"
#include "../src/board.h"

static int failures = 0, checks = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        printf("FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

/* Joga uma sequência de colunas (soltar). */
static void play_seq(Board *b, const char *cols)
{
    for (; *cols; cols++)
        board_play(b, *cols - '0', MOVE_DROP);
}

static void test_gravity_and_turns(void)
{
    Board b;
    board_init(&b, PLAYER1, 0);
    CHECK(board_play(&b, 3, MOVE_DROP) == 0);
    CHECK(b.cells[0][3] == PLAYER1);
    CHECK(b.current == PLAYER2);
    CHECK(board_play(&b, 3, MOVE_DROP) == 1);
    CHECK(b.cells[1][3] == PLAYER2);
    CHECK(b.current == PLAYER1);
}

static void test_full_column(void)
{
    Board b;
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "000000");
    CHECK(!board_can_drop(&b, 0));
    CHECK(board_play(&b, 0, MOVE_DROP) == -1);
    CHECK(b.result == RESULT_NONE);
}

static void test_wins(void)
{
    Board b;

    /* horizontal */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "0011223");
    CHECK(b.result == RESULT_WIN1);
    CHECK(b.win_count == 4);

    /* vertical */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "0101010");
    CHECK(b.result == RESULT_WIN1);

    /* diagonal / */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "01122323363");
    CHECK(b.result == RESULT_WIN1);

    /* diagonal \ */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "3221101060");
    CHECK(b.result == RESULT_NONE);
    play_seq(&b, "0");
    CHECK(b.result == RESULT_WIN1);

    /* sem jogadas após o fim */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "0101010");
    CHECK(board_play(&b, 5, MOVE_DROP) == -1);
}

static void test_draw(void)
{
    /* Sequência que enche a grade sem 4 em linha. */
    Board b;
    int c, r;
    board_init(&b, PLAYER1, 0);
    /* padrão de colunas em pares (0,1),(2,3),(4,5) e 6 alternando */
    {
        const char *seq =
            "010101" "101010"
            "232323" "323232"
            "454545" "545454"
            "666666";
        play_seq(&b, seq);
    }
    for (c = 0; c < BOARD_COLS; c++)
        CHECK(b.heights[c] == BOARD_ROWS);
    if (b.result != RESULT_DRAW) {
        for (r = BOARD_ROWS - 1; r >= 0; r--) {
            for (c = 0; c < BOARD_COLS; c++)
                printf("%d", b.cells[r][c]);
            printf("\n");
        }
    }
    CHECK(b.result == RESULT_DRAW);
}

static void test_popout(void)
{
    Board b;

    board_init(&b, PLAYER1, 1);
    play_seq(&b, "01");
    /* jogador 1 pode retirar da coluna 0, não da 1 */
    CHECK(board_can_pop(&b, 0, PLAYER1));
    CHECK(!board_can_pop(&b, 1, PLAYER1));
    CHECK(board_play(&b, 1, MOVE_POP) == -1);

    /* retirada faz a coluna descer */
    board_init(&b, PLAYER1, 1);
    play_seq(&b, "00"); /* P1 embaixo, P2 em cima */
    play_seq(&b, "6");  /* P1 */
    play_seq(&b, "5");  /* P2 */
    CHECK(b.current == PLAYER1);
    CHECK(board_play(&b, 0, MOVE_POP) == 0);
    CHECK(b.cells[0][0] == PLAYER2);
    CHECK(b.cells[1][0] == EMPTY);
    CHECK(b.heights[0] == 1);
    CHECK(b.current == PLAYER2);

    /* retirada indisponível no modo clássico */
    board_init(&b, PLAYER1, 0);
    play_seq(&b, "01");
    CHECK(!board_can_pop(&b, 0, PLAYER1));
}

static void test_popout_both_lines(void)
{
    /* Monta à mão: retirar a peça de P1 da base da coluna 3 faz
     * descer uma peça que completa 4 para P1 e 4 para P2. */
    Board b;
    board_init(&b, PLAYER1, 1);
    /* linha 0: P2 P2 P2 [P1] . . .  -> após retirar, (0,3)=P2 */
    b.cells[0][0] = PLAYER2; b.cells[0][1] = PLAYER2; b.cells[0][2] = PLAYER2;
    b.cells[0][3] = PLAYER1; b.cells[1][3] = PLAYER2; b.cells[2][3] = PLAYER1;
    /* linha 1 (vira linha 1 após descer): P1 P1 P1 em 0..2, (1,3)<-P1 */
    b.cells[1][0] = PLAYER1; b.cells[1][1] = PLAYER1; b.cells[1][2] = PLAYER1;
    b.heights[0] = 2; b.heights[1] = 2; b.heights[2] = 2; b.heights[3] = 3;
    b.current = PLAYER1;
    CHECK(board_play(&b, 3, MOVE_POP) == 0);
    CHECK(b.cells[0][3] == PLAYER2 && b.cells[1][3] == PLAYER1);
    CHECK(board_has_line(&b, PLAYER1) && board_has_line(&b, PLAYER2));
    CHECK(b.result == RESULT_WIN1); /* vence quem retirou */
}

static void test_ai(void)
{
    Board b;
    unsigned int seed = 7;
    int level;

    for (level = AI_EASY; level <= AI_HARD; level++) {
        AiMove m;
        /* IA (P1) vence na hora: três na base, coluna 3 livre */
        board_init(&b, PLAYER1, 0);
        play_seq(&b, "041526");
        m = ai_choose(&b, level, &seed);
        CHECK(m.col == 3 && m.type == MOVE_DROP);
    }

    for (level = AI_MEDIUM; level <= AI_HARD; level++) {
        AiMove m;
        /* IA (P2) precisa bloquear a coluna 3 */
        board_init(&b, PLAYER1, 0);
        play_seq(&b, "06162");
        m = ai_choose(&b, level, &seed);
        CHECK(m.col == 3);
    }

    /* Partidas IA x IA terminam sem jogadas inválidas. */
    {
        int g;
        for (g = 0; g < 6; g++) {
            int guard = 0;
            board_init(&b, PLAYER1, g % 2);
            while (b.result == RESULT_NONE && guard++ < 400) {
                AiMove m = ai_choose(&b, g % 3, &seed);
                CHECK(board_play(&b, m.col, m.type) >= 0);
            }
            CHECK(b.result != RESULT_NONE);
        }
    }

    /* Difícil vence o fácil na maioria das partidas. */
    {
        int g, hard_wins = 0;
        for (g = 0; g < 10; g++) {
            int hard = (g % 2) ? PLAYER1 : PLAYER2;
            board_init(&b, PLAYER1, 0);
            while (b.result == RESULT_NONE) {
                int lvl = b.current == hard ? AI_HARD : AI_EASY;
                AiMove m = ai_choose(&b, lvl, &seed);
                board_play(&b, m.col, m.type);
            }
            if ((b.result == RESULT_WIN1 && hard == PLAYER1) ||
                (b.result == RESULT_WIN2 && hard == PLAYER2))
                hard_wins++;
        }
        printf("difícil x fácil: %d/10 vitórias do difícil\n", hard_wins);
        CHECK(hard_wins >= 8);
    }
}

int main(void)
{
    test_gravity_and_turns();
    test_full_column();
    test_wins();
    test_draw();
    test_popout();
    test_popout_both_lines();
    test_ai();
    printf("%d verificações, %d falhas\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
