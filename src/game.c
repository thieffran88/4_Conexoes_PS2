/*
 * 4 Conexões - telas, controles, animações e desenho.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ai.h"
#include "assets.h"
#include "audio.h"
#include "bigfont.h"
#include "board.h"
#include "font.h"
#include "game.h"
#include "lang.h"
#include "platform.h"

/* ---------------------------------------------------------------- */
/* Paleta                                                            */
/* ---------------------------------------------------------------- */
#define WHITE          RGB(255, 255, 255)
#define BLACK          RGB(0, 0, 0)
#define COL_BURST_A    RGB(118, 40, 196)
#define COL_BURST_B    RGB(140, 62, 220)
#define COL_BURST_DA   RGB(42, 22, 92)
#define COL_BURST_DB   RGB(52, 30, 110)
#define COL_BOARD      RGB(30, 84, 214)
#define COL_BOARD_DARK RGB(14, 46, 150)
#define COL_BOARD_EDGE RGB(74, 140, 250)
#define COL_HOLE       RGB(18, 14, 52)
#define COL_TEXT       RGB(255, 255, 255)
#define COL_DIM        RGB(215, 205, 245)
#define COL_YELLOW     RGB(255, 212, 20)
#define COL_CARD       RGB(36, 26, 92)
#define COL_BTN        RGB(36, 92, 220)
#define COL_BTN_DARK   RGB(20, 54, 160)
#define COL_BTN_SEL    RGB(240, 70, 40)
#define COL_BTN_SEL_D  RGB(190, 30, 20)

static const Color PLAYER_COL[3] = { 0, RGB(228, 40, 40), RGB(255, 200, 20) };

/* ---------------------------------------------------------------- */
/* Estado                                                            */
/* ---------------------------------------------------------------- */
enum { ST_SPLASH, ST_LANG, ST_TITLE, ST_HELP, ST_PLAY, ST_ANIM, ST_OVER,
       ST_PAUSE };
enum { MENU_CPU, MENU_2P, MENU_MODE, MENU_LEVEL, MENU_MUSIC, MENU_HELP,
       MENU_LANG, MENU_COUNT };
enum { PAUSE_CONTINUE, PAUSE_RESTART, PAUSE_MENU, PAUSE_COUNT };

/* Poses do corpo inteiro (mesma ordem em assets.h). */
enum { POSE_IDLE, POSE_CHEER, POSE_SAD, POSE_THINK, POSE_SURPRISE };
/* Expressões (índice dos rostos em assets.h). */
enum { FACE_NEUTRAL = 0, FACE_GRIN = 1, FACE_FOCUS = 2, FACE_ANGRY = 3,
       FACE_SURPRISE = 4, FACE_SAD = 5, FACE_LAUGH = 6, FACE_HAPPY = 7 };

#define CIRCLE_SEG 24
#define REPEAT_DELAY 14
#define REPEAT_RATE 5
#define CPU_THINK_FRAMES 36
#define CPU_STEP_FRAMES 7
#define HELP_PAGES 3

static struct {
    int state;
    unsigned int frame;
    unsigned int seed;
    int state_frames;

    /* menus */
    int menu_sel;
    int lang_sel;
    int lang_return;
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

    /* reações dos personagens */
    int expr[3];
    int expr_timer[3];

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
    StrId msg;
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

static void set_state(int s)
{
    G.state = s;
    G.state_frames = 0;
}

/* ---------------------------------------------------------------- */
/* Primitivas de desenho                                             */
/* ---------------------------------------------------------------- */
static float SW(void) { return (float)plat_width(); }
static float SH(void) { return (float)plat_height(); }

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

/* Retângulo com cantos arredondados (cor opaca). */
static void rrect(float x, float y, float w, float h, float r, Color c)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    plat_rect(x + r, y, w - 2 * r, h, c);
    plat_rect(x, y + r, r, h - 2 * r, c);
    plat_rect(x + w - r, y + r, r, h - 2 * r, c);
    circle(x + r, y + r, r, c);
    circle(x + w - r, y + r, r, c);
    circle(x + r, y + h - r, r, c);
    circle(x + w - r, y + h - r, r, c);
}

/* Caixa no estilo cartum: contorno preto, sombra e realce. */
static void cartoon_box(float x, float y, float w, float h, float r, Color fill,
                        Color dark)
{
    rrect(x - 3, y - 3, w + 6, h + 9, r + 3, BLACK);
    rrect(x, y, w, h + 3, r, dark);
    rrect(x, y, w, h, r, fill);
}

static void sprite(int id, float x, float y, float w, float h, Color tint)
{
    const AssetSprite *s = &g_sprites[id];
    plat_sprite(id, x, y, w, h, 0, 0, s->w, s->h, tint);
}

/* Sprite com altura 'h', centrada em cx e com a base em 'bottom'. */
static void sprite_bottom(int id, float cx, float bottom, float h, Color tint)
{
    const AssetSprite *s = &g_sprites[id];
    float w = s->w * h / s->h;
    sprite(id, cx - w / 2, bottom - h, w, h, tint);
}

static void sprite_center(int id, float cx, float cy, float h, Color tint)
{
    const AssetSprite *s = &g_sprites[id];
    float w = s->w * h / s->h;
    sprite(id, cx - w / 2, cy - h / 2, w, h, tint);
}

static void disc(float cx, float cy, float rad, int player, int alpha)
{
    int id = player == PLAYER1 ? SPR_DISC_RED : SPR_DISC_YELLOW;
    sprite_center(id, cx, cy, rad * 2.25f, RGBA(255, 255, 255, alpha));
}

/* Fundo de raios girando, como na capa. */
static void burst(float cx, float cy, Color a, Color b, float rot)
{
    int i, n = 18;
    float R = 900;
    plat_rect(0, 0, SW(), SH(), a);
    for (i = 0; i < n; i += 2) {
        float a0 = rot + i * 6.2831853f / n, a1 = rot + (i + 1) * 6.2831853f / n;
        plat_tri(cx, cy, cx + cosf(a0) * R, cy + sinf(a0) * R,
                 cx + cosf(a1) * R, cy + sinf(a1) * R, b);
    }
}

/* Estrela de explosão (balão de "VITÓRIA!"). */
static void starburst(float cx, float cy, float rx, float ry, Color c)
{
    int i, n = 28;
    for (i = 0; i < n; i++) {
        float a0 = i * 6.2831853f / n, a1 = (i + 1) * 6.2831853f / n;
        float r0 = (i % 2) ? 0.80f : 1.0f, r1 = (i % 2) ? 1.0f : 0.80f;
        plat_tri(cx, cy, cx + cosf(a0) * rx * r0, cy + sinf(a0) * ry * r0,
                 cx + cosf(a1) * rx * r1, cy + sinf(a1) * ry * r1, c);
    }
}

/* Texto pequeno com contorno preto. */
static void text_rect(float x, float y, float w, float h, void *user)
{
    plat_rect(x, y, w, h, *(Color *)user);
}

static void text(const char *s, float x, float y, float scale, Color c)
{
    Color k = BLACK;
    float o = scale < 2 ? 1 : scale * 0.5f;
    font_draw(s, x - o, y, scale, text_rect, &k);
    font_draw(s, x + o, y, scale, text_rect, &k);
    font_draw(s, x, y - o, scale, text_rect, &k);
    font_draw(s, x, y + o, scale, text_rect, &k);
    font_draw(s, x + o, y + o, scale, text_rect, &k);
    font_draw(s, x, y, scale, text_rect, &c);
}

static void text_center(const char *s, float cx, float y, float scale, Color c)
{
    text(s, cx - font_width(s, scale) / 2, y, scale, c);
}

/* Escala que faz o texto caber em 'maxw' (até 'scale'). */
static float fit_scale(const char *s, float scale, float maxw)
{
    float w = font_width(s, 1);
    if (w * scale > maxw)
        scale = maxw / w;
    return scale;
}

static float fit_big(const char *s, float h, float maxw)
{
    float w = bigfont_width(s, h);
    if (w > maxw)
        h = h * maxw / w;
    return h;
}

/* Bandeiras desenhadas (tela de idioma e botão do menu). */
static void flag(int lang, float x, float y, float w, float h)
{
    int i;
    plat_rect(x - 2, y - 2, w + 4, h + 4, BLACK);
    if (lang == LANG_PT) {
        float cx = x + w / 2, cy = y + h / 2;
        plat_rect(x, y, w, h, RGB(0, 150, 60));
        plat_tri(cx, y + h * 0.1f, x + w * 0.08f, cy, cx, y + h * 0.9f,
                 RGB(255, 210, 0));
        plat_tri(cx, y + h * 0.1f, x + w * 0.92f, cy, cx, y + h * 0.9f,
                 RGB(255, 210, 0));
        circle(cx, cy, h * 0.24f, RGB(30, 50, 150));
    } else if (lang == LANG_EN) {
        for (i = 0; i < 7; i++)
            plat_rect(x, y + i * h / 7, w, h / 7 + 0.5f,
                      (i % 2) ? WHITE : RGB(200, 30, 45));
        plat_rect(x, y, w * 0.45f, h * 4 / 7, RGB(30, 45, 120));
    } else {
        plat_rect(x, y, w, h, RGB(200, 20, 30));
        plat_rect(x, y + h / 4, w, h / 2, RGB(255, 200, 0));
    }
}

/* ---------------------------------------------------------------- */
/* Barra de dicas com os botões do controle                         */
/* ---------------------------------------------------------------- */
enum { HI_NONE, HI_CROSS, HI_CIRCLE, HI_SQUARE, HI_TRIANGLE, HI_START,
       HI_LR, HI_UD };

typedef struct {
    int icon;
    StrId label;
} Hint;

static float hint_icon(int icon, float x, float cy, int draw)
{
    float s = 18;
    int id = -1;
    switch (icon) {
    case HI_CROSS: id = SPR_BTN_CROSS; break;
    case HI_CIRCLE: id = SPR_BTN_CIRCLE; break;
    case HI_SQUARE: id = SPR_BTN_SQUARE; break;
    case HI_TRIANGLE: id = SPR_BTN_TRIANGLE; break;
    default: break;
    }
    if (id >= 0) {
        if (draw)
            sprite_center(id, x + s / 2, cy, s, WHITE);
        return s;
    }
    {
        const char *t = icon == HI_START ? "START"
                      : icon == HI_LR ? "\x01\x02" : "\xE2\x86\x91\xE2\x86\x93";
        float w = font_width(t, 1) + 10;
        if (draw) {
            rrect(x - 1, cy - 8, w + 2, 16, 7, BLACK);
            rrect(x, cy - 7, w, 14, 6, RGB(225, 225, 235));
            font_draw(t, x + 5, cy - 3.5f, 1, text_rect, &(Color){ BLACK });
        }
        return w;
    }
}

static void hints(const Hint *h, int n, float cy)
{
    float total = 0, x;
    int i, pass;
    for (pass = 0; pass < 2; pass++) {
        x = SW() / 2 - total / 2;
        for (i = 0; i < n; i++) {
            const char *label = tr(h[i].label);
            float iw = hint_icon(h[i].icon, x, cy, pass);
            x += iw + 5;
            if (pass)
                text(label, x, cy - 3.5f, 1, WHITE);
            x += font_width(label, 1) + (i < n - 1 ? 16 : 0);
        }
        if (!pass)
            total = x - (SW() / 2 - total / 2);
    }
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
        return tr(p == G.cpu_player ? S_CPU : S_YOU);
    return tr(p == PLAYER1 ? S_P1 : S_P2);
}

static void react(int p, int expr, int frames)
{
    G.expr[p] = expr;
    G.expr_timer[p] = frames;
}

static void show_msg(StrId m)
{
    G.msg = m;
    G.msg_timer = 100;
    audio_sfx(SFX_INVALID);
    react(G.board.current, FACE_SURPRISE, 70);
}

static void start_round(void)
{
    board_init(&G.board, G.first_player, G.popout);
    G.cursor = BOARD_COLS / 2;
    G.cpu_timer = 0;
    G.cpu_planned = 0;
    G.scored = 0;
    G.msg_timer = 0;
    G.expr_timer[1] = G.expr_timer[2] = 0;
    set_state(ST_PLAY);
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
static float selector_y(void) { return G.by - G.cell / 2 - 8; }

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
        audio_sfx(SFX_DROP);
    } else {
        audio_sfx(SFX_POP);
    }
    set_state(ST_ANIM);
}

static void finish_move(void)
{
    int mover = G.board.current;
    if (G.anim_type == MOVE_DROP && !G.anim_bounced)
        audio_sfx(SFX_LAND);
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
        if (G.board.result == RESULT_DRAW)
            audio_music(MUS_DRAW);
        else if (G.vs_cpu && ((G.board.result == RESULT_WIN1) ==
                              (G.cpu_player == PLAYER1)))
            audio_music(MUS_LOSE);
        else
            audio_music(MUS_WIN);
        set_state(ST_OVER);
    } else {
        react(mover, FACE_GRIN, 45);
        set_state(ST_PLAY);
    }
}

static void try_move(int col, int type)
{
    if (type == MOVE_DROP) {
        if (board_can_drop(&G.board, col))
            begin_move(col, MOVE_DROP);
        else
            show_msg(S_COL_FULL);
    } else {
        if (!G.popout)
            return;
        if (board_can_pop(&G.board, col, G.board.current))
            begin_move(col, MOVE_POP);
        else
            show_msg(S_POP_INVALID);
    }
}

static void update_cpu(void)
{
    G.cpu_timer++;
    if (!G.cpu_planned && G.cpu_timer >= CPU_THINK_FRAMES) {
        plat_audio_long_task();
        G.cpu_move = ai_choose(&G.board, G.level, &G.seed);
        G.cpu_planned = 1;
        G.cpu_timer = 0;
    }
    if (G.cpu_planned && G.cpu_timer % CPU_STEP_FRAMES == 0) {
        if (G.cursor < G.cpu_move.col) { G.cursor++; audio_sfx(SFX_TICK); }
        else if (G.cursor > G.cpu_move.col) { G.cursor--; audio_sfx(SFX_TICK); }
        else try_move(G.cpu_move.col, G.cpu_move.type);
    }
}

static void update_play(void)
{
    int p = G.board.current;
    unsigned int in;

    if (input_any() & BTN_START) {
        audio_sfx(SFX_PAUSE);
        G.pause_sel = PAUSE_CONTINUE;
        set_state(ST_PAUSE);
        return;
    }

    if (G.vs_cpu && p == G.cpu_player) {
        update_cpu();
        return;
    }

    in = input_of(p);
    if ((in & BTN_LEFT) && G.cursor > 0) { G.cursor--; audio_sfx(SFX_MOVE); }
    if ((in & BTN_RIGHT) && G.cursor < BOARD_COLS - 1) { G.cursor++; audio_sfx(SFX_MOVE); }
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
                audio_sfx(SFX_LAND);
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
    if (G.state_frames < 20)
        return;
    if (in & BTN_CROSS) {
        audio_sfx(SFX_START);
        /* As partidas seguintes alternam quem começa. */
        G.first_player = board_other(G.first_player);
        start_round();
    } else if (in & (BTN_START | BTN_TRIANGLE)) {
        audio_sfx(SFX_BACK);
        set_state(ST_TITLE);
    }
}

static void update_pause(void)
{
    unsigned int in = input_any();
    if (in & (BTN_UP | BTN_DOWN)) audio_sfx(SFX_MOVE);
    if (in & BTN_UP) G.pause_sel = (G.pause_sel + PAUSE_COUNT - 1) % PAUSE_COUNT;
    if (in & BTN_DOWN) G.pause_sel = (G.pause_sel + 1) % PAUSE_COUNT;
    if (in & (BTN_START | BTN_CIRCLE | BTN_TRIANGLE)) {
        audio_sfx(SFX_BACK);
        set_state(ST_PLAY);
        return;
    }
    if (in & BTN_CROSS) {
        audio_sfx(SFX_SELECT);
        if (G.pause_sel == PAUSE_CONTINUE) set_state(ST_PLAY);
        else if (G.pause_sel == PAUSE_RESTART) start_round();
        else set_state(ST_TITLE);
    }
}

static void update_splash(void)
{
    if (G.state_frames > 10 && (input_any() & (BTN_START | BTN_CROSS))) {
        audio_sfx(SFX_START);
        G.lang_sel = lang_get();
        G.lang_return = 0;
        set_state(ST_LANG);
    }
}

static void update_lang(void)
{
    unsigned int in = input_any();
    if (in & (BTN_UP | BTN_DOWN)) audio_sfx(SFX_MOVE);
    if (in & BTN_UP) G.lang_sel = (G.lang_sel + LANG_COUNT - 1) % LANG_COUNT;
    if (in & BTN_DOWN) G.lang_sel = (G.lang_sel + 1) % LANG_COUNT;
    if (in & (BTN_CROSS | BTN_START)) {
        audio_sfx(SFX_SELECT);
        lang_set(G.lang_sel);
        set_state(ST_TITLE);
    } else if ((in & BTN_CIRCLE) && G.lang_return) {
        audio_sfx(SFX_BACK);
        set_state(ST_TITLE);
    }
}

static void update_title(void)
{
    unsigned int in = input_any();
    if (in & (BTN_UP | BTN_DOWN)) audio_sfx(SFX_MOVE);
    if (in & BTN_UP) G.menu_sel = (G.menu_sel + MENU_COUNT - 1) % MENU_COUNT;
    if (in & BTN_DOWN) G.menu_sel = (G.menu_sel + 1) % MENU_COUNT;

    if (in & (BTN_LEFT | BTN_RIGHT | BTN_CROSS)) {
        int dir = (in & BTN_LEFT) ? -1 : 1;
        if (G.menu_sel == MENU_CPU || G.menu_sel == MENU_2P) {
            if (in & BTN_CROSS)
                audio_sfx(SFX_START);
        } else if ((in & BTN_CROSS) || G.menu_sel != MENU_HELP) {
            audio_sfx(SFX_SELECT);
        }
        switch (G.menu_sel) {
        case MENU_MUSIC:
            audio_set_music_enabled(!audio_music_enabled());
            break;
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
                set_state(ST_HELP);
            }
            break;
        case MENU_LANG:
            if (in & BTN_CROSS) {
                G.lang_sel = lang_get();
                G.lang_return = 1;
                set_state(ST_LANG);
            } else {
                lang_set((lang_get() + LANG_COUNT + dir) % LANG_COUNT);
            }
            break;
        }
    }
}

static void update_help(void)
{
    unsigned int in = input_any();
    if (in & (BTN_LEFT | BTN_RIGHT)) audio_sfx(SFX_MOVE);
    if (in & (BTN_RIGHT | BTN_CROSS)) {
        if (G.help_page < HELP_PAGES - 1) G.help_page++;
        else if (in & BTN_CROSS) set_state(ST_TITLE);
    }
    if ((in & BTN_LEFT) && G.help_page > 0) G.help_page--;
    if (in & (BTN_CIRCLE | BTN_TRIANGLE | BTN_START)) {
        audio_sfx(SFX_BACK);
        set_state(ST_TITLE);
    }
}

/* ---------------------------------------------------------------- */
/* Desenho: personagens                                              */
/* ---------------------------------------------------------------- */
static int winner(void)
{
    if (G.board.result == RESULT_WIN1) return PLAYER1;
    if (G.board.result == RESULT_WIN2) return PLAYER2;
    return 0;
}

static int face_of(int p)
{
    if (G.state == ST_OVER) {
        int w = winner();
        if (!w) return FACE_SURPRISE;
        return w == p ? FACE_LAUGH : (p == PLAYER1 ? FACE_ANGRY : FACE_SAD);
    }
    if (G.expr_timer[p] > 0)
        return G.expr[p];
    if (G.board.current == p)
        return (G.vs_cpu && p == G.cpu_player) ? FACE_NEUTRAL : FACE_FOCUS;
    return FACE_NEUTRAL;
}

static int pose_of(int p)
{
    if (G.state == ST_OVER) {
        int w = winner();
        if (!w) return POSE_SURPRISE;
        return w == p ? POSE_CHEER : POSE_SAD;
    }
    if (G.expr_timer[p] > 0 && G.expr[p] == FACE_SURPRISE)
        return POSE_SURPRISE;
    if (G.board.current == p && (G.state == ST_PLAY || G.state == ST_ANIM) &&
        G.vs_cpu && p == G.cpu_player)
        return POSE_THINK;
    return POSE_IDLE;
}

static int face_sprite(int p, int expr)
{
    return (p == PLAYER1 ? SPR_BOY_FACE0 : SPR_GIRL_FACE0) + expr;
}

static int pose_sprite(int p, int pose)
{
    return (p == PLAYER1 ? SPR_BOY_IDLE : SPR_GIRL_IDLE) + pose;
}

static void draw_body(int p, float cx, float bottom)
{
    int pose = pose_of(p);
    int id = pose_sprite(p, pose);
    float h = (float)g_sprites[id].h;
    float bob = 0;
    if (pose == POSE_CHEER)
        bob = -fabsf(sinf(G.frame * 0.18f)) * 12;
    else if (pose == POSE_IDLE)
        bob = sinf(G.frame * 0.06f + p) * 1.5f;
    /* sombra no chão */
    plat_rect(cx - 30, bottom - 3, 60, 5, RGBA(0, 0, 0, 90));
    sprite_bottom(id, cx, bottom + bob, h, WHITE);
}

/* ---------------------------------------------------------------- */
/* Desenho: partida                                                  */
/* ---------------------------------------------------------------- */
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
    float L = G.bx - 10, R = G.bx + w + 10, B = G.by + h + 8;
    int r, c;
    int pop_anim = G.state == ST_ANIM && G.anim_type == MOVE_POP;

    /* pés (atrás da grade) */
    plat_tri(L - 30, B + 26, L + 4, B - 30, L + 22, B - 30, BLACK);
    plat_tri(L - 30, B + 26, L + 22, B - 30, L + 8, B + 26, BLACK);
    plat_tri(L - 26, B + 22, L + 6, B - 30, L + 18, B - 30, COL_BOARD_DARK);
    plat_tri(L - 26, B + 22, L + 18, B - 30, L + 4, B + 22, COL_BOARD_DARK);
    plat_tri(R + 30, B + 26, R - 4, B - 30, R - 22, B - 30, BLACK);
    plat_tri(R + 30, B + 26, R - 22, B - 30, R - 8, B + 26, BLACK);
    plat_tri(R + 26, B + 22, R - 6, B - 30, R - 18, B - 30, COL_BOARD_DARK);
    plat_tri(R + 26, B + 22, R - 18, B - 30, R - 4, B + 22, COL_BOARD_DARK);

    /* contorno preto da grade */
    rrect(L - 4, G.by - 14, R - L + 8, B - G.by + 18, 12, BLACK);

    /* fundo dos furos */
    plat_rect(G.bx, G.by, w, h, COL_HOLE);

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
            holed_square(col_x(c), row_y(r), half, rad + 2, COL_BOARD);
            ring(col_x(c), row_y(r), rad + 2, rad + 5, COL_BOARD_DARK);
        }
    rrect(L, G.by - 10, R - L, 10, 6, COL_BOARD_EDGE);
    plat_rect(L, G.by - 2, 10, h + 2, COL_BOARD_EDGE);
    plat_rect(G.bx + w, G.by - 2, 10, h + 2, COL_BOARD);
    rrect(L, G.by + h, R - L, B - G.by - h, 6, COL_BOARD_DARK);
    plat_rect(L + 6, G.by - 7, R - L - 12, 2, RGBA(255, 255, 255, 120));

    /* destaque da linha vencedora */
    if (G.state == ST_OVER && G.board.win_count > 0) {
        int pulse = (int)(G.frame / 8) % 2;
        for (r = 0; r < BOARD_ROWS; r++)
            for (c = 0; c < BOARD_COLS; c++)
                if (is_win_cell(r, c)) {
                    ring(col_x(c), row_y(r), rad - 1, rad + 4,
                         pulse ? WHITE : COL_YELLOW);
                    if ((G.frame / 6 + r + c) % 4 == 0)
                        sprite_center(SPR_ICON_STAR, col_x(c) + rad * 0.7f,
                                      row_y(r) - rad * 0.7f, 22, WHITE);
                }
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
             rad * 0.62f, rad * 0.8f,
             (PLAYER_COL[p] & 0xFFFFFF) | 0xA0000000u);

    disc(x, y, rad, p, 255);

    /* na Retirada, marca a peça que pode sair */
    if (G.popout && board_can_pop(&G.board, G.cursor, p))
        ring(x, row_y(0), rad + 4, rad + 7, WHITE);
}

static void draw_card(int p, float x, float w)
{
    char buf[16];
    float cx = x + w / 2, y = 62;
    int active = (G.state == ST_PLAY || G.state == ST_ANIM) &&
                 G.board.current == p;
    Color pc = PLAYER_COL[p];
    const char *name = player_name(p);

    if (active) {
        float glow = 3 + sinf(G.frame * 0.2f) * 2;
        rrect(x - 3 - glow, y - 3 - glow, w + 6 + 2 * glow, 186 + 2 * glow,
              14, (pc & 0xFFFFFF) | 0x70000000u);
    }
    cartoon_box(x, y, w, 176, 12, COL_CARD, RGB(20, 14, 60));
    /* cabeçalho com o nome */
    rrect(x, y, w, 26, 12, pc);
    plat_rect(x, y + 14, w, 12, pc);
    text_center(name, cx, y + 7, fit_scale(name, 2, w - 12), WHITE);

    /* rosto */
    circle(cx, y + 72, 38, BLACK);
    circle(cx, y + 72, 35, (pc & 0xFFFFFF) | 0xFF000000u);
    circle(cx, y + 72, 31, RGB(250, 240, 255));
    sprite_center(face_sprite(p, face_of(p)), cx, y + 72, 64, WHITE);

    /* placar */
    snprintf(buf, sizeof(buf), "%d", G.score[p]);
    bigfont_draw_center(buf, cx, y + 124, 34, WHITE);

    if (active) {
        const char *t = tr((G.vs_cpu && p == G.cpu_player) ? S_THINKING
                                                           : S_YOUR_TURN);
        float s = fit_scale(t, 2, w - 4);
        float tw = font_width(t, s) + 16;
        cartoon_box(cx - tw / 2, y + 190, tw, 22, 10, pc, BLACK);
        if (!(G.vs_cpu && p == G.cpu_player) || (G.frame / 20) % 2)
            text_center(t, cx, y + 201 - 3.5f * s, s, WHITE);
    }
}

static void draw_hud(void)
{
    float W = SW(), H = SH();
    float side_w = G.bx - 34;
    char buf[48];

    sprite(SPR_LOGO, 10, 4, 112, 52, WHITE);

    text(tr(G.popout ? S_MODE_POP : S_MODE_CLASSIC), W - side_w - 16, 14, 1,
         COL_YELLOW);
    snprintf(buf, sizeof(buf), "%s: %d", tr(S_DRAWS), G.draws);
    text(buf, W - side_w - 16, 28, 1, WHITE);
    if (G.vs_cpu) {
        static const StrId lv[3] = { S_EASY, S_MEDIUM, S_HARD };
        snprintf(buf, sizeof(buf), "%s: %s", tr(S_LEVEL), tr(lv[G.level]));
        text(buf, W - side_w - 16, 42, 1, COL_DIM);
    }

    draw_card(PLAYER1, 12, side_w);
    draw_card(PLAYER2, W - 12 - side_w, side_w);
    draw_body(PLAYER1, 12 + side_w / 2, H - 14);
    draw_body(PLAYER2, W - 12 - side_w / 2, H - 14);

    if (G.state == ST_PLAY || G.state == ST_ANIM) {
        static const Hint h1[] = {
            { HI_LR, S_H_COLUMN }, { HI_CROSS, S_H_DROP }, { HI_START, S_H_PAUSE }
        };
        static const Hint h2[] = {
            { HI_LR, S_H_COLUMN }, { HI_CROSS, S_H_DROP },
            { HI_SQUARE, S_H_POP }, { HI_START, S_H_PAUSE }
        };
        if (G.popout) hints(h2, 4, H - 12);
        else hints(h1, 3, H - 12);
    }

    if (G.msg_timer > 0) {
        const char *m = tr(G.msg);
        float s = fit_scale(m, 2, BOARD_COLS * G.cell - 30);
        float w = font_width(m, s) + 28;
        float y = G.by + BOARD_ROWS * G.cell / 2 - 18;
        cartoon_box(W / 2 - w / 2, y, w, 34, 14, COL_BTN_SEL, COL_BTN_SEL_D);
        text_center(m, W / 2, y + 17 - 3.5f * s, s, WHITE);
    }
}

static void draw_over(void)
{
    float W = SW();
    char buf[64];
    const char *msg;
    Color c = RGB(235, 50, 30), cd = RGB(255, 140, 20);
    int w = winner();
    float cy = G.by + 118, th, tw, rx, pop;

    if (!w) {
        msg = tr(S_DRAW);
        c = RGB(40, 120, 240);
        cd = RGB(110, 190, 255);
    } else if (G.vs_cpu) {
        msg = tr(w == G.cpu_player ? S_LOSE : S_WIN);
        if (w == G.cpu_player) {
            c = RGB(130, 50, 200);
            cd = RGB(190, 110, 250);
        }
    } else {
        snprintf(buf, sizeof(buf), tr(S_WINS_FMT), player_name(w));
        msg = buf;
    }

    /* entrada com "pulo" */
    pop = G.state_frames < 12 ? G.state_frames / 12.0f : 1.0f;
    pop = pop * (1.15f - 0.15f * pop);

    th = fit_big(msg, 36, 360);
    tw = bigfont_width(msg, th);
    rx = (tw / 2 + 56) * pop;
    starburst(W / 2, cy, rx + 7, 72 * pop + 7, BLACK);
    starburst(W / 2, cy, rx, 72 * pop, c);
    starburst(W / 2, cy, rx * 0.86f, 60 * pop, cd);
    starburst(W / 2, cy, rx * 0.80f, 54 * pop, c);
    if (pop > 0.9f)
        bigfont_draw_center(msg, W / 2, cy - th / 2, th, WHITE);

    if (G.state_frames > 20) {
        static const Hint h[] = { { HI_CROSS, S_PLAY_AGAIN }, { HI_START, S_MENU } };
        float y = cy + 92;
        cartoon_box(W / 2 - 150, y - 14, 300, 28, 12, RGB(30, 22, 80), BLACK);
        hints(h, 2, y);
    }
}

static void draw_pause(void)
{
    static const StrId items[PAUSE_COUNT] = { S_CONTINUE, S_RESTART, S_MAIN_MENU };
    float W = SW(), H = SH();
    float y = H / 2 - 110;
    int i;

    plat_rect(0, 0, W, H, RGBA(0, 0, 0, 150));
    cartoon_box(W / 2 - 160, y, 320, 214, 18, COL_CARD, RGB(20, 14, 60));
    bigfont_draw_center(tr(S_PAUSE), W / 2, y + 14, 34, WHITE);
    for (i = 0; i < PAUSE_COUNT; i++) {
        int sel = i == G.pause_sel;
        float by = y + 70 + i * 46.0f;
        cartoon_box(W / 2 - 130, by, 260, 34, 14, sel ? COL_BTN_SEL : COL_BTN,
                    sel ? COL_BTN_SEL_D : COL_BTN_DARK);
        bigfont_draw_center(tr(items[i]), W / 2, by + 8,
                            fit_big(tr(items[i]), 19, 230), WHITE);
    }
}

/* ---------------------------------------------------------------- */
/* Desenho: menus                                                    */
/* ---------------------------------------------------------------- */
static void draw_menu_bg(void)
{
    burst(SW() / 2, SH() * 0.42f, COL_BURST_A, COL_BURST_B, G.frame * 0.003f);
}

static void draw_logo(float cy, float w)
{
    const AssetSprite *s = &g_sprites[SPR_LOGO];
    float h = s->h * w / s->w;
    float bob = sinf(G.frame * 0.05f) * 3;
    sprite(SPR_LOGO, SW() / 2 - w / 2, cy - h / 2 + bob, w, h, WHITE);
}

static void draw_side_characters(void)
{
    float H = SH(), W = SW();
    float b = sinf(G.frame * 0.07f) * 2;
    sprite_bottom(SPR_BOY_BIG, 80, H - 6 + b, 230, WHITE);
    sprite_bottom(SPR_GIRL_BIG, W - 82, H - 6 - b, 230, WHITE);
}

static void menu_button(float cx, float y, float w, float h, int sel,
                        int icon, int flag_lang, const char *label, int arrows)
{
    float x = cx - w / 2;
    float th;
    if (sel) {
        float g = 1 + sinf(G.frame * 0.2f) * 1.5f;
        x -= g;
        w += 2 * g;
    }
    cartoon_box(x, y, w, h, h / 2, sel ? COL_BTN_SEL : COL_BTN,
                sel ? COL_BTN_SEL_D : COL_BTN_DARK);
    plat_rect(x + h / 2, y + 4, w - h, 3, RGBA(255, 255, 255, 70));
    if (icon >= 0)
        sprite_center(icon, x + 26, y + h / 2, 28, WHITE);
    else if (flag_lang >= 0)
        flag(flag_lang, x + 12, y + h / 2 - 9, 28, 18);
    th = fit_big(label, 19, w - 96);
    bigfont_draw_center(label, cx + 10, y + h / 2 - th / 2 - 1, th, WHITE);
    if (arrows && sel) {
        float ay = y + h / 2;
        plat_tri(x + w - 34, ay - 7, x + w - 34, ay + 7, x + w - 44, ay, WHITE);
        plat_tri(x + w - 18, ay - 7, x + w - 18, ay + 7, x + w - 8, ay, WHITE);
    }
}

static void draw_title(void)
{
    static const StrId levels[3] = { S_EASY, S_MEDIUM, S_HARD };
    float W = SW(), H = SH();
    char buf[64];
    int i;

    draw_menu_bg();
    draw_side_characters();
    draw_logo(80, 330);

    for (i = 0; i < MENU_COUNT; i++) {
        int icon = -1, fl = -1, arrows = 0;
        float y = 158 + i * 37.0f;
        switch (i) {
        case MENU_CPU:
            snprintf(buf, sizeof(buf), "%s", tr(S_MENU_CPU));
            icon = SPR_ICON_1P;
            break;
        case MENU_2P:
            snprintf(buf, sizeof(buf), "%s", tr(S_MENU_2P));
            icon = SPR_ICON_2P;
            break;
        case MENU_MODE:
            snprintf(buf, sizeof(buf), "%s: %s", tr(S_MODE),
                     tr(G.popout ? S_MODE_POP : S_MODE_CLASSIC));
            icon = SPR_ICON_PAD;
            arrows = 1;
            break;
        case MENU_LEVEL:
            snprintf(buf, sizeof(buf), "%s: %s", tr(S_LEVEL),
                     tr(levels[G.level]));
            icon = SPR_ICON_TROPHY;
            arrows = 1;
            break;
        case MENU_MUSIC:
            snprintf(buf, sizeof(buf), "%s: %s", tr(S_MUSIC),
                     tr(audio_music_enabled() ? S_ON : S_OFF));
            icon = SPR_ICON_SOUND;
            arrows = 1;
            break;
        case MENU_HELP:
            snprintf(buf, sizeof(buf), "%s", tr(S_HELP));
            icon = SPR_ICON_HELP;
            break;
        default:
            snprintf(buf, sizeof(buf), "%s", tr(S_LANGUAGE));
            fl = lang_get();
            arrows = 1;
            break;
        }
        menu_button(W / 2, y, 290, 30, i == G.menu_sel, icon, fl, buf, arrows);
    }

    {
        static const Hint h[] = {
            { HI_UD, S_H_CHOOSE }, { HI_LR, S_H_CHANGE }, { HI_CROSS, S_H_CONFIRM }
        };
        hints(h, 3, H - 12);
    }
}

static void draw_lang(void)
{
    float W = SW(), H = SH();
    int i;

    draw_menu_bg();
    draw_side_characters();
    draw_logo(76, 300);
    text_center("IDIOMA  -  LANGUAGE  -  IDIOMA", W / 2, 158, 2, WHITE);

    for (i = 0; i < LANG_COUNT; i++) {
        int sel = i == G.lang_sel;
        float y = 190 + i * 62.0f, w = 270, x = W / 2 - w / 2, th;
        const char *name = tr_in(i, S_LANG_NAME);
        if (sel) {
            float g = 1 + sinf(G.frame * 0.2f) * 2;
            x -= g;
            w += 2 * g;
        }
        cartoon_box(x, y, w, 46, 20, sel ? COL_BTN_SEL : COL_BTN,
                    sel ? COL_BTN_SEL_D : COL_BTN_DARK);
        flag(i, x + 16, y + 9, 42, 28);
        th = fit_big(name, 24, w - 90);
        bigfont_draw_center(name, x + w / 2 + 26, y + 23 - th / 2, th, WHITE);
    }

    {
        /* dicas no idioma em destaque */
        static const Hint h[] = { { HI_UD, S_H_CHOOSE }, { HI_CROSS, S_H_CONFIRM } };
        int old = lang_get();
        lang_set(G.lang_sel);
        hints(h, 2, H - 12);
        lang_set(old);
    }
}

static void draw_splash(void)
{
    float W = SW(), H = SH();
    sprite(SPR_COVER, 0, 0, W, H, WHITE);
    if ((G.frame / 24) % 2 == 0) {
        float w = 220;
        cartoon_box(W / 2 - w / 2, H - 56, w, 34, 16, COL_BTN_SEL, COL_BTN_SEL_D);
        bigfont_draw_center("PRESS START", W / 2, H - 48, 20, WHITE);
    }
}

/* Linha de texto com ícones: \x03 = X, \x04 = quadrado. */
static void rich_line(const char *s, float x, float y, float scale)
{
    char buf[96];
    int n = 0;
    while (1) {
        char ch = *s;
        if (ch == '\x03' || ch == '\x04' || ch == 0) {
            buf[n] = 0;
            if (n) {
                text(buf, x, y, scale, WHITE);
                x += font_width(buf, scale) + scale * 6;
                n = 0;
            }
            if (!ch)
                break;
            sprite_center(ch == '\x03' ? SPR_BTN_CROSS : SPR_BTN_SQUARE,
                          x + 11, y + 3.5f * scale, 24, WHITE);
            x += 24;
            s++;
            continue;
        }
        if (n < (int)sizeof(buf) - 1)
            buf[n++] = ch;
        s++;
    }
}

static void draw_help(void)
{
    static const StrId titles[HELP_PAGES] = { S_HELP_P1_TITLE, S_HELP_P2_TITLE,
                                              S_HELP_P3_TITLE };
    static const StrId texts[HELP_PAGES] = { S_HELP_P1_TEXT, S_HELP_P2_TEXT,
                                             S_HELP_P3_TEXT };
    float W = SW(), H = SH();
    const char *p;
    char line[96];
    char buf[32];
    float y = 106;
    int i;

    burst(W / 2, H * 0.42f, COL_BURST_DA, COL_BURST_DB, G.frame * 0.003f);
    cartoon_box(36, 26, W - 72, H - 80, 18, COL_CARD, RGB(20, 14, 60));
    bigfont_draw_center(tr(titles[G.help_page]), W / 2, 42, 32, WHITE);
    sprite_center(SPR_ICON_HELP, 76, 58, 40, WHITE);
    sprite_center(G.help_page == 2 ? SPR_ICON_PAD : SPR_DISC_RED, W - 76, 58,
                  40, WHITE);

    p = tr(texts[G.help_page]);
    while (*p) {
        int n = 0;
        while (*p && *p != '\n' && n < (int)sizeof(line) - 1)
            line[n++] = *p++;
        line[n] = 0;
        if (*p == '\n')
            p++;
        if (line[0] == '\x01') {
            /* linha das setas */
            hint_icon(HI_LR, 70, y + 7, 1);
            text(line + 4, 70 + hint_icon(HI_LR, 0, 0, 0) + 10, y, 2, WHITE);
        } else if (line[0] == '\x05') {
            hint_icon(HI_START, 70, y + 7, 1);
            text(line + 1, 70 + hint_icon(HI_START, 0, 0, 0) + 8, y, 2, WHITE);
        } else {
            rich_line(line, 70, y, 2);
        }
        y += 26;
    }

    for (i = 0; i < HELP_PAGES; i++)
        circle(W / 2 - 20 + i * 20.0f, H - 70, i == G.help_page ? 6 : 4,
               i == G.help_page ? COL_YELLOW : COL_DIM);
    snprintf(buf, sizeof(buf), tr(S_PAGE_FMT), G.help_page + 1, HELP_PAGES);
    (void)buf;

    {
        static const Hint h[] = { { HI_LR, S_H_PAGES }, { HI_CIRCLE, S_H_BACK } };
        hints(h, 2, H - 26);
    }
}

/* ---------------------------------------------------------------- */
/* API                                                               */
/* ---------------------------------------------------------------- */
void game_init(unsigned int seed)
{
    int i, W = plat_width(), H = plat_height();

    memset(&G, 0, sizeof(G));
    audio_init(AUDIO_RATE);
    G.seed = seed ? seed : 12345u;
    G.level = AI_MEDIUM;
    G.first_player = PLAYER1;
    set_state(ST_SPLASH);

    for (i = 0; i <= CIRCLE_SEG; i++) {
        float a = (float)i * 6.28318531f / CIRCLE_SEG;
        G.cos_t[i] = cosf(a);
        G.sin_t[i] = sinf(a);
    }

    G.cell = 50;
    G.bx = (W - BOARD_COLS * G.cell) / 2;
    G.by = (H - BOARD_ROWS * G.cell) / 2 + 22;
}

void game_frame(void)
{
    int p;

    G.frame++;
    G.state_frames++;
    read_input();
    if (G.msg_timer > 0)
        G.msg_timer--;
    for (p = 1; p <= 2; p++)
        if (G.expr_timer[p] > 0)
            G.expr_timer[p]--;

    switch (G.state) {
    case ST_SPLASH: update_splash(); break;
    case ST_LANG:   update_lang();   break;
    case ST_TITLE:  update_title();  break;
    case ST_HELP:   update_help();   break;
    case ST_PLAY:   update_play();   break;
    case ST_ANIM:   update_anim();   break;
    case ST_OVER:   update_over();   break;
    case ST_PAUSE:  update_pause();  break;
    }

    switch (G.state) {
    case ST_SPLASH: case ST_LANG: case ST_TITLE: case ST_HELP:
        audio_music(MUS_TITLE);
        break;
    case ST_PLAY: case ST_ANIM: case ST_PAUSE:
        audio_music(MUS_GAME);
        break;
    default:
        break;
    }

    plat_begin_frame(BLACK);
    switch (G.state) {
    case ST_SPLASH:
        draw_splash();
        break;
    case ST_LANG:
        draw_lang();
        break;
    case ST_TITLE:
        draw_title();
        break;
    case ST_HELP:
        draw_help();
        break;
    default:
        burst(SW() / 2, G.by + BOARD_ROWS * G.cell / 2, COL_BURST_DA,
              COL_BURST_DB, G.frame * 0.002f);
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
