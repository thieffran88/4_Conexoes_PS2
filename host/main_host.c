/*
 * Executor "host" do 4 Conexões: roda o jogo com uma sequência de
 * comandos e salva capturas de tela (PPM).
 *
 * Uso: ./4conexoes_host CMD [CMD ...]
 *   L R U D X O Q T S   aperta Esq/Dir/Cima/Baixo/X/Círculo/Quadrado/
 *                       Triângulo/Start por 1 quadro (e solta por 1)
 *   2:<cmd>             o mesmo, no controle 2
 *   pad2 / nopad2       conecta / desconecta o controle 2
 *   w<N>                espera N quadros
 *   shot:<arquivo.ppm>  salva a tela atual
 *   wav:<arquivo.wav>   salva todo o áudio gerado até agora
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game.h"
#include "../src/platform.h"

void host_set_buttons(int pad, unsigned int b);
void host_set_connected(int pad, int c);
int host_save_ppm(const char *path);
int host_save_wav(const char *path);

static unsigned int button_of(char c)
{
    switch (c) {
    case 'L': return BTN_LEFT;
    case 'R': return BTN_RIGHT;
    case 'U': return BTN_UP;
    case 'D': return BTN_DOWN;
    case 'X': return BTN_CROSS;
    case 'O': return BTN_CIRCLE;
    case 'Q': return BTN_SQUARE;
    case 'T': return BTN_TRIANGLE;
    case 'S': return BTN_START;
    default:  return 0;
    }
}

int main(int argc, char **argv)
{
    int i;

    plat_init();
    game_init(1234u);
    game_frame();

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        int pad = 0;

        if (strcmp(a, "pad2") == 0) {
            host_set_connected(1, 1);
            continue;
        }
        if (strcmp(a, "nopad2") == 0) {
            host_set_connected(1, 0);
            continue;
        }
        if (strncmp(a, "shot:", 5) == 0) {
            if (host_save_ppm(a + 5) != 0) {
                fprintf(stderr, "falha ao salvar %s\n", a + 5);
                return 1;
            }
            continue;
        }
        if (strncmp(a, "wav:", 4) == 0) {
            if (host_save_wav(a + 4) != 0) {
                fprintf(stderr, "falha ao salvar %s\n", a + 4);
                return 1;
            }
            continue;
        }
        if (a[0] == 'w') {
            int n = atoi(a + 1);
            while (n-- > 0)
                game_frame();
            continue;
        }
        if (strncmp(a, "2:", 2) == 0) {
            pad = 1;
            a += 2;
        }
        for (; *a; a++) {
            host_set_buttons(pad, button_of(*a));
            game_frame();
            host_set_buttons(pad, 0);
            game_frame();
        }
    }
    return 0;
}
