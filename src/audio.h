/*
 * Áudio do 4 Conexões: sintetizador chiptune (músicas e efeitos).
 * Independente de plataforma: a plataforma chama audio_mix() para
 * obter amostras PCM 16 bits mono.
 */
#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_RATE 22050

enum {
    MUS_NONE = -1,
    MUS_TITLE = 0,
    MUS_GAME,
    MUS_WIN,
    MUS_LOSE,
    MUS_DRAW,
    MUS_COUNT
};

enum {
    SFX_MOVE = 0,   /* cursor / opção de menu */
    SFX_TICK,       /* cursor do console */
    SFX_SELECT,     /* confirmar / alterar opção */
    SFX_BACK,
    SFX_START,      /* PRESS START */
    SFX_DROP,       /* soltar peça */
    SFX_LAND,       /* peça encaixa */
    SFX_POP,        /* retirar peça */
    SFX_INVALID,
    SFX_PAUSE,
    SFX_COUNT
};

void audio_init(int rate);
void audio_music(int song);         /* troca a música (reinicia se mudar) */
int  audio_music_playing(void);     /* 0 quando uma vinheta terminou */
void audio_sfx(int id);
void audio_set_music_enabled(int on);
int  audio_music_enabled(void);
void audio_mix(short *out, int n);

/* Para testes: número de passos (semicolcheias) de um canal. */
int audio_song_steps(int song, int channel);

#endif
