/*
 * Textos do jogo em Português, Inglês e Espanhol.
 */
#ifndef LANG_H
#define LANG_H

enum { LANG_PT = 0, LANG_EN = 1, LANG_ES = 2, LANG_COUNT };

typedef enum {
    S_LANG_NAME,      /* nome do idioma no próprio idioma */
    S_SUBTITLE,
    S_MENU_CPU,
    S_MENU_2P,
    S_MODE,
    S_MODE_CLASSIC,
    S_MODE_POP,
    S_LEVEL,
    S_EASY,
    S_MEDIUM,
    S_HARD,
    S_HELP,
    S_MUSIC,
    S_ON,
    S_OFF,
    S_LANGUAGE,
    S_YOU,
    S_CPU,
    S_P1,
    S_P2,
    S_YOUR_TURN,
    S_THINKING,
    S_DRAWS,
    S_COL_FULL,
    S_CONNECT_PAD_FMT,
    S_POP_INVALID,
    S_WIN,
    S_LOSE,
    S_DRAW,
    S_WINS_FMT,
    S_PLAY_AGAIN,
    S_MENU,
    S_PAUSE,
    S_CONTINUE,
    S_RESTART,
    S_MAIN_MENU,
    S_H_CHOOSE,
    S_H_CHANGE,
    S_H_CONFIRM,
    S_H_BACK,
    S_H_COLUMN,
    S_H_DROP,
    S_H_POP,
    S_H_PAUSE,
    S_H_PAGES,
    S_PAGE_FMT,
    S_HELP_P1_TITLE,
    S_HELP_P1_TEXT,
    S_HELP_P2_TITLE,
    S_HELP_P2_TEXT,
    S_HELP_P3_TITLE,
    S_HELP_P3_TEXT,
    S_COUNT
} StrId;

void lang_set(int lang);
int lang_get(void);
const char *tr(StrId id);
/* Texto de um idioma específico (tela de escolha de idioma). */
const char *tr_in(int lang, StrId id);

#endif
