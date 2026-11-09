/*
 * 4 Conexões - ponto de entrada para PlayStation 2.
 */
#include "game.h"
#include "platform.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    plat_init();
    game_init(0x4C0DE5u);

    for (;;)
        game_frame();

    return 0;
}
