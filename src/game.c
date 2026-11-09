/*
 * 4 Conexões - telas, controles, animações e desenho.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ai.h"
#include "board.h"
#include "font.h"
#include "game.h"
#include "platform.h"

/* ---------------------------------------------------------------- */
/* Paleta                                                            */
/* ---------------------------------------------------------------- */
#define COL_BG_TOP     RGB(18, 28, 70)
#define COL_BG_BOTTOM  RGB(6, 10, 28)
#define COL_BOARD      RGB(28, 88, 210)
#define COL_BOARD_DARK RGB(16, 52, 140)
#define COL_BOARD_EDGE RGB(60, 130, 240)
#define COL_TEXT       RGB(240, 240, 250)
#define COL_DIM        RGB(150, 160, 190)
#define COL_ACCENT     RGB(255, 210, 60)
#define COL_SHADOW     RGBA(0, 0, 0, 160)

static const Color DISC_MAIN[3]  = { 0, RGB(225, 45, 45),  RGB(250, 200, 30) };
static const Color DISC_DARK[3]  = { 0, RGB(150, 20, 25),  RGB(190, 135, 10) };
static const Color DISC_LIGHT[3] = { 0, RGBA(255, 150, 150, 150),
                                     RGBA(255, 250, 190, 160) };

/* ---------------------------------------------------------------- */
/* Estado                                                            */
/* ---------------------------------------------------------------- */
enum { ST_TITLE, ST_HELP, ST_PLAY, ST_ANIM, ST_OVER, ST_PAUSE };
enum { MENU_CPU, MENU_2P, MENU_MODE, MENU_LEVEL, MENU_HELP, MENU_COUNT };
enum { PAUSE_CONTINUE, PAUSE_RESTART, PAUSE_MENU, PAUSE_COUNT };

#define CIRCLE_SEG 24
#define REPEAT_DELAY 14
#define REPEAT_RATE 5
#define CPU_THINK_FRAMES 30
#define CPU_STEP_FRAMES 7

static struct {
    int state;
    unsigned int frame;
    unsigned int seed;

    /* menu */
    int menu_sel;
    int popout;
    int level;
    int help_page;
    int pause_sel;

    /* partida */
    Board board;
    int vs_cpu;
    int cpu_player;
    int cursor;
    int first_player;
    int score[3];
    int draws;
    int scored;

    /* vez do console */
    int cpu_timer;
    int cpu_planned;
    AiMove cpu_move;

    /* animação */
    int anim_type;
    int anim_col;
    int anim_row;
    float anim_y;
    float anim_v;
    float anim_t;
    int anim_bounced;

    /* aviso temporário */
    const char *msg;
    int msg_timer;

    /* controles */
    unsigned int down[2];
    unsigned int pressed[2];
    int hold[2];

    /* layout */
    float cell, bx, by;

    float cos_t[CIRCLE_SEG + 1];
    float sin_t[CIRCLE_SEG + 1];
} G;

/* ---------------------------------------------------------------- */
/* Primitivas de desenho                                             */
/* ---------------------------------------------------------------- */
static void circle(float cx, float cy, float r, Color c)
{
    int i;
    for (i = 0; i < CIRCLE_SEG; i++)
        plat_tri(cx, cy,
                 cx + G.cos_t[i] * r, cy + G.sin_t[i] * r,
                 cx + G.cos_t[i + 1] * r, cy + G.sin_t[i + 1] * r, c);
}

static void ring(float cx, float cy, float r0, float r1, Color c)
{
    int i;
    for (i = 0; i < CIRCLE_SEG; i++) {
        float ax = G.cos_t[i], ay = G.sin_t[i];
        float bx = G.cos_t[i + 1], by = G.sin_t[i + 1];
        plat_tri(cx + ax * r0, cy + ay * r0, cx + ax * r1, cy + ay * r1,
                 cx + bx * r1, cy + by * r1, c);
        plat_tri(cx + ax * r0, cy + ay * r0, cx + bx * r1, cy + by * r1,
                 cx + bx * r0, cy + by * r0, c);
    }
}

/* Quadrado com um furo redondo (a "moldura" de cada casa). */
static void holed_square(float cx, float cy, float half, float r, Color c)
{
    int i;
    for (i = 0; i < CIRCLE_SEG; i++) {
        float ax = G.cos_t[i], ay = G.sin_t[i];
        float bx = G.cos_t[i + 1], by = G.sin_t[i + 1];
        float ma = fabsf(ax) > fabsf(ay) ? fabsf(ax) : fabsf(ay);
        float mb = fabsf(bx) > fabsf(by) ? fabsf(bx) : fabsf(by);
        float sax = cx + ax / ma * half, say = cy + ay / ma * half;
        float sbx = cx + bx / mb * half, sby = cy + by / mb * half;
        plat_tri(cx + ax * r, cy + ay * r, sax, say, sbx, sby, c);
        plat_tri(cx + ax * r, cy + ay * r, sbx, sby,
                 cx + bx * r, cy + by * r, c);
    }
}

static void disc(float cx, float cy, float r, int player, int alpha)
{
    Color main = DISC_MAIN[player], dark = DISC_DARK[player];
    Color light = DISC_LIGHT[player];
    if (alpha < 255) {
        main = (main & 0xFFFFFF) | ((Color)alpha << 24);
        dark = (dark & 0xFFFFFF) | ((Color)alpha << 24);
        light = (light & 0xFFFFFF) | ((Color)(alpha / 2) << 24);
    }
    circle(cx, cy, r, dark);
    circle(cx, cy, r * 0.80f, main);
    ring(cx, cy, r * 0.55f, r * 0.62f, dark);
    circle(cx - r * 0.30f, cy - r * 0.32f, r * 0.18f, light);
}

static void text_rect(float x, float y, float w, float h, void *user)
{
    plat_rect(x, y, w, h, *(Color *)user);
}

static void text(const char *s, float x, float y, float scale, Color c)
{
    Color sh = COL_SHADOW;
    font_draw(s, x + scale * 0.5f + 1, y + scale * 0.5f + 1, scale,
              text_rect, &sh);
    font_draw(s, x, y, scale, text_rect, &c);
}

static void text_center(const char *s, float cx, float y, float scale, Color c)
{
    text(s, cx - font_width(s, scale) / 2, y, scale, c);
}

static void panel(float x, float y, float w, float h, Color fill, Color edge)
{
    plat_rect(x - 2, y - 2, w + 4, 2, edge);
    plat_rect(x - 2, y + h, w + 4, 2, edge);
    plat_rect(x - 2, y, 2, h, edge);
    plat_rect(x + w, y, 2, h, edge);
    plat_rect(x, y, w, h, fill);
}

/* ---------------------------------------------------------------- */
/* Controles                                                         */
/* ---------------------------------------------------------------- */
static void read_input(void)
{
    int p;
    for (p = 0; p < 2; p++) {
        unsigned int now = plat_buttons(p);
        unsigned int dirs = BTN_LEFT | BTN_RIGHT | BTN_UP | BTN_DOWN;
        G.pressed[p] = now & ~G.down[p];
        if (now & dirs & G.down[p]) {
            G.hold[p]++;
            if (G.hold[p] >= REPEAT_DELAY &&
                (G.hold[p] - REPEAT_DELAY) % REPEAT_RATE == 0)
                G.pressed[p] |= now & dirs;
        } else {
            G.hold[p] = 0;
        }
        G.down[p] = now;
    }
}

/* Botões do controle de quem está jogando agora. Com dois controles
 * conectados no modo 2 jogadores, cada um usa o seu. */
static unsigned int input_of(int player)
{
    if (!G.vs_cpu && plat_pad_connected(1))
        return G.pressed[player == PLAYER1 ? 0 : 1];
    return G.pressed[0];
}

static unsigned int input_any(void)
{
    return G.pressed[0] | G.pressed[1];
}

/* ---------------------------------------------------------------- */
/* Lógica de partida                                                 */
/* ---------------------------------------------------------------- */
static const char *player_name(int p)
{
    if (G.vs_cpu)
        return p == G.cpu_player ? "CONSOLE" : "VOCÊ";
    return p == PLAYER1 ? "JOGADOR 1" : "JOGADOR 2";
}

static void show_msg(const char *m)
{
    G.msg = m;
    G.msg_timer = 90;
}

static void start_round(void)
{
    board_init(&G.board, G.first_player, G.popout);
    G.cursor = BOARD_COLS / 2;
    G.cpu_timer = 0;
    G.cpu_planned = 0;
    G.scored = 0;
    G.msg = NULL;
    G.state = ST_PLAY;
}

static void start_match(int vs_cpu)
{
    G.vs_cpu = vs_cpu;
    G.cpu_player = PLAYER2;
    G.score[1] = G.score[2] = 0;
    G.draws = 0;
    G.first_player = PLAYER1;
    G.seed ^= G.frame * 2654435761u;
    start_round();
}

static float col_x(int c) { return G.bx + c * G.cell + G.cell / 2; }
static float row_y(int r) { return G.by + (BOARD_ROWS - 1 - r) * G.cell + G.cell / 2; }
static float selector_y(void) { return G.by - G.cell / 2 - 6; }

static void begin_move(int col, int type)
{
    G.anim_type = type;
    G.anim_col = col;
    G.anim_t = 0;
    G.anim_v = 0;
    G.anim_bounced = 0;
    if (type == MOVE_DROP) {
        G.anim_row = G.board.heights[col];
        G.anim_y = selector_y();
    }
    G.state = ST_ANIM;
}

static void finish_move(void)
{
    board_play(&G.board, G.anim_col, G.anim_type);
    G.cpu_timer = 0;
    G.cpu_planned = 0;
    if (G.board.result != RESULT_NONE) {
        if (!G.scored) {
            if (G.board.result == RESULT_WIN1) G.score[1]++;
            else if (G.board.result == RESULT_WIN2) G.score[2]++;
            else G.draws++;
            G.scored = 1;
        }
        G.state = ST_OVER;
    } else {
        G.state = ST_PLAY;
    }
}

static void try_move(int col, int type)
{
    if (type == MOVE_DROP) {
        if (board_can_drop(&G.board, col))
            begin_move(col, MOVE_DROP);
        else
            show_msg("COLUNA CHEIA!");
    } else {
        if (!G.popout)
            return;
        if (board_can_pop(&G.board, col, G.board.current))
            begin_move(col, MOVE_POP);
        else
            show_msg("SÓ PODE RETIRAR PEÇA SUA DA BASE");
    }
}

static void update_cpu(void)
{
    G.cpu_timer++;
    if (!G.cpu_planned && G.cpu_timer >= CPU_THINK_FRAMES) {
        G.cpu_move = ai_choose(&G.board, G.level, &G.seed);
        G.cpu_planned = 1;
        G.cpu_timer = 0;
    }
    if (G.cpu_planned && G.cpu_timer % CPU_STEP_FRAMES == 0) {
        if (G.cursor < G.cpu_move.col) G.cursor++;
        else if (G.cursor > G.cpu_move.col) G.cursor--;
        else try_move(G.cpu_move.col, G.cpu_move.type);
    }
}

static void update_play(void)
{
    int p = G.board.current;
    unsigned int in;

    if (input_any() & BTN_START) {
        G.pause_sel = PAUSE_CONTINUE;
        G.state = ST_PAUSE;
        return;
    }

    if (G.vs_cpu && p == G.cpu_player) {
        update_cpu();
        return;
    }

    in = input_of(p);
    if ((in & BTN_LEFT) && G.cursor > 0) G.cursor--;
    if ((in & BTN_RIGHT) && G.cursor < BOARD_COLS - 1) G.cursor++;
    if (in & BTN_CROSS)
        try_move(G.cursor, MOVE_DROP);
    else if (in & BTN_SQUARE)
        try_move(G.cursor, MOVE_POP);
}

static void update_anim(void)
{
    if (G.anim_type == MOVE_DROP) {
        float target = row_y(G.anim_row);
        G.anim_v += 1.6f;
        G.anim_y += G.anim_v;
        if (G.anim_y >= target) {
            G.anim_y = target;
            if (!G.anim_bounced && G.anim_v > 6) {
                G.anim_v = -G.anim_v * 0.25f;
                G.anim_bounced = 1;
            } else {
                finish_move();
            }
        }
    } else {
        G.anim_v += 0.012f;
        G.anim_t += G.anim_v;
        if (G.anim_t >= 1.0f)
            finish_move();
    }
}

static void update_over(void)
{
    unsigned int in = input_any();
    if (in & BTN_CROSS) {
        /* As partidas seguintes alternam quem começa. */
        G.first_player = board_other(G.first_player);
        start_round();
    } else if (in & (BTN_START | BTN_TRIANGLE)) {
        G.state = ST_TITLE;
    }
}

static void update_pause(void)
{
    unsigned int in = input_any();
    if (in & BTN_UP) G.pause_sel = (G.pause_sel + PAUSE_COUNT - 1) % PAUSE_COUNT;
    if (in & BTN_DOWN) G.pause_sel = (G.pause_sel + 1) % PAUSE_COUNT;
    if (in & (BTN_START | BTN_CIRCLE | BTN_TRIANGLE)) {
        G.state = ST_PLAY;
        return;
    }
    if (in & BTN_CROSS) {
        if (G.pause_sel == PAUSE_CONTINUE) G.state = ST_PLAY;
        else if (G.pause_sel == PAUSE_RESTART) start_round();
        else G.state = ST_TITLE;
    }
}

static void update_title(void)
{
    unsigned int in = input_any();
    if (in & BTN_UP) G.menu_sel = (G.menu_sel + MENU_COUNT - 1) % MENU_COUNT;
    if (in & BTN_DOWN) G.menu_sel = (G.menu_sel + 1) % MENU_COUNT;

    if (in & (BTN_LEFT | BTN_RIGHT | BTN_CROSS)) {
        int dir = (in & BTN_LEFT) ? -1 : 1;
        switch (G.menu_sel) {
        case MENU_MODE:
            G.popout = !G.popout;
            break;
        case MENU_LEVEL:
            G.level = (G.level + 3 + dir) % 3;
            break;
        case MENU_CPU:
            if (in & BTN_CROSS) start_match(1);
            break;
        case MENU_2P:
            if (in & BTN_CROSS) start_match(0);
            break;
        case MENU_HELP:
            if (in & BTN_CROSS) {
                G.help_page = 0;
                G.state = ST_HELP;
            }
            break;
        }
    }
}

#define HELP_PAGES 3

static void update_help(void)
{
    unsigned int in = input_any();
    if (in & (BTN_RIGHT | BTN_CROSS)) {
        if (G.help_page < HELP_PAGES - 1) G.help_page++;
        else if (in & BTN_CROSS) G.state = ST_TITLE;
    }
    if ((in & BTN_LEFT) && G.help_page > 0) G.help_page--;
    if (in & (BTN_CIRCLE | BTN_TRIANGLE | BTN_START)) G.state = ST_TITLE;
}

/* ---------------------------------------------------------------- */
/* Desenho                                                           */
/* ---------------------------------------------------------------- */
static void draw_background(void)
{
    plat_gradient(0, 0, (float)plat_width(), (float)plat_height(),
                  COL_BG_TOP, COL_BG_BOTTOM);
}

static int is_win_cell(int r, int c)
{
    int i;
    for (i = 0; i < G.board.win_count; i++)
        if (G.board.win_r[i] == r && G.board.win_c[i] == c)
            return 1;
    return 0;
}

static void draw_board(void)
{
    float rad = G.cell * 0.40f, half = G.cell / 2;
    float w = BOARD_COLS * G.cell, h = BOARD_ROWS * G.cell;
    int r, c;
    int pop_anim = G.state == ST_ANIM && G.anim_type == MOVE_POP;

    /* fundo dos furos */
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++)
            circle(col_x(c), row_y(r), rad + 1, RGB(8, 13, 36));

    /* peças */
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++) {
            int v = G.board.cells[r][c];
            float y = row_y(r);
            int alpha = 255;
            if (!v)
                continue;
            if (pop_anim && c == G.anim_col) {
                float t = G.anim_t > 1 ? 1 : G.anim_t;
                y += t * G.cell;
                if (r == 0) {
                    y += t * G.cell * 0.8f;
                    alpha = (int)(255 * (1 - t));
                }
            }
            disc(col_x(c), y, rad, v, alpha);
        }

    /* peça caindo */
    if (G.state == ST_ANIM && G.anim_type == MOVE_DROP)
        disc(col_x(G.anim_col), G.anim_y, rad, G.board.current, 255);

    /* moldura azul por cima das peças */
    for (r = 0; r < BOARD_ROWS; r++)
        for (c = 0; c < BOARD_COLS; c++) {
            holed_square(col_x(c), row_y(r), half, rad + 1, COL_BOARD);
            ring(col_x(c), row_y(r), rad + 1, rad + 4, COL_BOARD_DARK);
        }
    plat_rect(G.bx - 8, G.by - 4, 8, h + 4, COL_BOARD_EDGE);
    plat_rect(G.bx + w, G.by - 4, 8, h + 4, COL_BOARD_DARK);
    plat_rect(G.bx - 8, G.by - 8, w + 16, 4, COL_BOARD_EDGE);
    plat_rect(G.bx - 8, G.by + h, w + 16, 6, COL_BOARD_DARK);
    /* pés da grade */
    plat_rect(G.bx - 20, G.by + h + 6, 40, 8, COL_BOARD_DARK);
    plat_rect(G.bx + w - 20, G.by + h + 6, 40, 8, COL_BOARD_DARK);

    /* destaque da linha vencedora */
    if (G.state == ST_OVER && G.board.win_count > 0) {
        int pulse = (int)(G.frame / 8) % 2;
        for (r = 0; r < BOARD_ROWS; r++)
            for (c = 0; c < BOARD_COLS; c++)
                if (is_win_cell(r, c))
                    ring(col_x(c), row_y(r), rad - 2, rad + 3,
                         pulse ? RGB(255, 255, 255) : COL_ACCENT);
    }
}

static void draw_selector(void)
{
    int p = G.board.current;
    float x = col_x(G.cursor);
    float y = selector_y() + sinf(G.frame * 0.12f) * 3;
    float rad = G.cell * 0.40f;

    if (G.state != ST_PLAY)
        return;

    /* prévia de onde a peça vai cair */
    if (board_can_drop(&G.board, G.cursor))
        ring(col_x(G.cursor), row_y(G.board.heights[G.cursor]),
             rad * 0.70f, rad * 0.85f,
             (DISC_MAIN[p] & 0xFFFFFF) | 0x90000000u);

    /* seta e peça na mão */
    plat_tri(x - 8, G.by - 14, x + 8, G.by - 14, x, G.by - 6, DISC_MAIN[p]);
    disc(x, y, rad, p, 255);

    /* na Retirada, marca a peça que pode sair */
    if (G.popout && board_can_pop(&G.board, G.cursor, p))
        ring(x, row_y(0), rad + 4, rad + 7, RGB(255, 255, 255));
}

static void draw_side(int player, float x, float w)
{
    char buf[32];
    float cx = x + w / 2;
    int active = (G.state == ST_PLAY || G.state == ST_ANIM) &&
                 G.board.current == player;
    float top = G.by;

    panel(x, top, w, 150, RGBA(0, 0, 0, 110),
          active ? DISC_MAIN[player] : RGBA(255, 255, 255, 40));
    {
        const char *name = player_name(player);
        float sc = (w - 12) / font_width(name, 1);
        if (sc > 2) sc = 2;
        text_center(name, cx, top + 12, sc, COL_TEXT);
    }
    disc(cx, top + 62, G.cell * 0.36f, player, 255);
    snprintf(buf, sizeof(buf), "%d", G.score[player]);
    text_center(buf, cx, top + 96, 5, COL_ACCENT);

    if (active) {
        const char *t = (G.vs_cpu && player == G.cpu_player)
                            ? "PENSANDO..." : "SUA VEZ";
        if (!(G.vs_cpu && player == G.cpu_player) || (G.frame / 20) % 2)
            text_center(t, cx, top + 160, 2, DISC_MAIN[player]);
    }
}

static void draw_hud(void)
{
    int W = plat_width(), H = plat_height();
    float side_w = G.bx - 36;
    char buf[48];

    snprintf(buf, sizeof(buf), "4 CONEXÕES - %s - EMPATES: %d",
             G.popout ? "RETIRADA" : "CLÁSSICO", G.draws);
    text_center(buf, W / 2.0f, 14, 2, COL_TEXT);

    draw_side(PLAYER1, 18, side_w);
    draw_side(PLAYER2, W - 18 - side_w, side_w);

    if (G.state == ST_PLAY || G.state == ST_ANIM) {
        const char *hint = G.popout
            ? "ESQ/DIR: COLUNA   X: SOLTAR   QUADRADO: RETIRAR   START: PAUSA"
            : "ESQ/DIR: ESCOLHER COLUNA   X: SOLTAR   START: PAUSA";
        text_center(hint, W / 2.0f, H - 20.0f, 1, COL_DIM);
    }

    if (G.msg && G.msg_timer > 0) {
        float w = font_width(G.msg, 2) + 24;
        panel(W / 2.0f - w / 2, G.by + BOARD_ROWS * G.cell / 2 - 16, w, 32,
              RGBA(0, 0, 0, 200), COL_ACCENT);
        text_center(G.msg, W / 2.0f, G.by + BOARD_ROWS * G.cell / 2 - 7, 2,
                    COL_ACCENT);
    }
}

static void draw_over(void)
{
    int W = plat_width(), H = plat_height();
    char buf[48];
    Color c = COL_ACCENT;
    float y = G.by + BOARD_ROWS * G.cell / 2 - 50;

    if (G.board.result == RESULT_DRAW) {
        snprintf(buf, sizeof(buf), "EMPATE!");
    } else {
        int w = G.board.result == RESULT_WIN1 ? PLAYER1 : PLAYER2;
        if (G.vs_cpu)
            snprintf(buf, sizeof(buf), w == G.cpu_player
                     ? "O CONSOLE VENCEU!" : "VOCÊ VENCEU!");
        else
            snprintf(buf, sizeof(buf), "%s VENCEU!", player_name(w));
        c = DISC_MAIN[w];
    }

    panel(G.bx - 6, y, BOARD_COLS * G.cell + 12, 92, RGBA(0, 0, 0, 200), c);
    text_center(buf, W / 2.0f, y + 18, 3, c);
    text_center("X: JOGAR DE NOVO  START: MENU", W / 2.0f, y + 62, 2,
                COL_TEXT);
    (void)H;
}

static void draw_pause(void)
{
    static const char *items[PAUSE_COUNT] = {
        "CONTINUAR", "REINICIAR PARTIDA", "MENU PRINCIPAL"
    };
    int W = plat_width(), H = plat_height(), i;
    float y = H / 2.0f - 80;

    plat_rect(0, 0, (float)W, (float)H, RGBA(0, 0, 0, 140));
    panel(W / 2.0f - 170, y, 340, 170, RGBA(10, 20, 60, 235), COL_BOARD_EDGE);
    text_center("PAUSA", W / 2.0f, y + 16, 4, COL_TEXT);
    for (i = 0; i < PAUSE_COUNT; i++) {
        Color c = i == G.pause_sel ? COL_ACCENT : COL_TEXT;
        if (i == G.pause_sel)
            text(">", W / 2.0f - 150, y + 66 + i * 32.0f, 2, c);
        text_center(items[i], W / 2.0f, y + 66 + i * 32.0f, 2, c);
    }
}

static void draw_title(void)
{
    static const char *levels[3] = { "FÁCIL", "MÉDIO", "DIFÍCIL" };
    int W = plat_width(), H = plat_height(), i;
    char buf[48];
    float y0 = 210;

    /* peças decorativas */
    for (i = 0; i < 9; i++) {
        float x = W / 2.0f - 4 * 46.0f + i * 46.0f;
        float y = 120 + sinf(G.frame * 0.05f + i * 0.7f) * 6;
        disc(x, y, 18, (i % 2) ? PLAYER2 : PLAYER1, 255);
    }

    text_center("4 CONEXÕES", W / 2.0f, 40, 6, COL_TEXT);
    text_center("ALINHE QUATRO PEÇAS PARA VENCER", W / 2.0f, 160, 2, COL_DIM);

    for (i = 0; i < MENU_COUNT; i++) {
        Color c = i == G.menu_sel ? COL_ACCENT : COL_TEXT;
        float y = y0 + i * 34.0f;
        switch (i) {
        case MENU_CPU:   snprintf(buf, sizeof(buf), "JOGAR CONTRA O CONSOLE"); break;
        case MENU_2P:    snprintf(buf, sizeof(buf), "DOIS JOGADORES"); break;
        case MENU_MODE:  snprintf(buf, sizeof(buf), "< MODO: %s >",
                                  G.popout ? "RETIRADA" : "CLÁSSICO"); break;
        case MENU_LEVEL: snprintf(buf, sizeof(buf), "< DIFICULDADE: %s >",
                                  levels[G.level]); break;
        default:         snprintf(buf, sizeof(buf), "COMO JOGAR"); break;
        }
        if (i == G.menu_sel)
            panel(W / 2.0f - 220, y - 8, 440, 30, RGBA(255, 255, 255, 25),
                  RGBA(255, 210, 60, 120));
        text_center(buf, W / 2.0f, y, 2, c);
    }

    text_center("CIMA/BAIXO: ESCOLHER   ESQ/DIR: ALTERAR   X: CONFIRMAR",
                W / 2.0f, H - 28.0f, 1, COL_DIM);
}

static void draw_help(void)
{
    static const char *pages[HELP_PAGES][11] = {
        { "REGRAS CLÁSSICAS",
          "",
          "- A GRADE TEM 7 COLUNAS E 6 LINHAS.",
          "- CADA JOGADOR TEM UMA COR: VERMELHO",
          "  OU AMARELO. O VERMELHO COMEÇA.",
          "- NA SUA VEZ, SOLTE UMA PEÇA EM UMA",
          "  COLUNA: ELA CAI ATÉ A CASA LIVRE MAIS",
          "  BAIXA.",
          "- VENCE QUEM ALINHAR 4 PEÇAS NA",
          "  HORIZONTAL, VERTICAL OU DIAGONAL.",
          "- GRADE CHEIA SEM VENCEDOR: EMPATE." },
        { "MODO RETIRADA",
          "",
          "- NA SUA VEZ, VOCÊ PODE SOLTAR UMA PEÇA",
          "  OU RETIRAR UMA PEÇA SUA DA LINHA DE",
          "  BAIXO. AS PEÇAS ACIMA DESCEM.",
          "- SE A RETIRADA FORMAR 4 EM LINHA PARA",
          "  OS DOIS, VENCE QUEM RETIROU.",
          "- EMPATE SE NÃO HOUVER JOGADA OU APÓS",
          "  120 JOGADAS.",
          "- A CADA NOVA PARTIDA, QUEM COMEÇA",
          "  ALTERNA." },
        { "CONTROLES",
          "",
          "ESQ/DIR ...... ESCOLHER COLUNA",
          "X ............ SOLTAR PEÇA",
          "QUADRADO ..... RETIRAR PEÇA (RETIRADA)",
          "START ........ PAUSA",
          "",
          "DOIS JOGADORES: COM DOIS CONTROLES,",
          "CADA UM USA O SEU. COM UM CONTROLE,",
          "PASSEM O CONTROLE A CADA JOGADA.",
          "" },
    };
    int W = plat_width(), H = plat_height(), i;
    char buf[32];
    float x = W / 2.0f - 250;

    panel(40, 30, W - 80.0f, H - 80.0f, RGBA(0, 0, 0, 150), COL_BOARD_EDGE);
    text_center(pages[G.help_page][0], W / 2.0f, 52, 3, COL_ACCENT);
    for (i = 1; i < 11; i++)
        text(pages[G.help_page][i], x, 72 + i * 26.0f, 2, COL_TEXT);

    snprintf(buf, sizeof(buf), "PÁGINA %d/%d", G.help_page + 1, HELP_PAGES);
    text_center(buf, W / 2.0f, H - 74.0f, 1, COL_DIM);
    text_center("ESQ/DIR: PÁGINAS   CÍRCULO: VOLTAR", W / 2.0f, H - 34.0f, 1,
                COL_DIM);
}

/* ---------------------------------------------------------------- */
/* API                                                               */
/* ---------------------------------------------------------------- */
void game_init(unsigned int seed)
{
    int i, W = plat_width(), H = plat_height();

    memset(&G, 0, sizeof(G));
    G.seed = seed ? seed : 12345u;
    G.level = AI_MEDIUM;
    G.state = ST_TITLE;
    G.first_player = PLAYER1;

    for (i = 0; i <= CIRCLE_SEG; i++) {
        float a = (float)i * 6.28318531f / CIRCLE_SEG;
        G.cos_t[i] = cosf(a);
        G.sin_t[i] = sinf(a);
    }

    G.cell = 52;
    G.bx = (W - BOARD_COLS * G.cell) / 2;
    G.by = (H - BOARD_ROWS * G.cell) / 2 + 30;
}

void game_frame(void)
{
    G.frame++;
    read_input();
    if (G.msg_timer > 0)
        G.msg_timer--;

    switch (G.state) {
    case ST_TITLE: update_title(); break;
    case ST_HELP:  update_help();  break;
    case ST_PLAY:  update_play();  break;
    case ST_ANIM:  update_anim();  break;
    case ST_OVER:  update_over();  break;
    case ST_PAUSE: update_pause(); break;
    }

    plat_begin_frame(COL_BG_BOTTOM);
    draw_background();
    switch (G.state) {
    case ST_TITLE:
        draw_title();
        break;
    case ST_HELP:
        draw_help();
        break;
    default:
        draw_board();
        draw_selector();
        draw_hud();
        if (G.state == ST_OVER)
            draw_over();
        else if (G.state == ST_PAUSE)
            draw_pause();
        break;
    }
    plat_end_frame();
}
