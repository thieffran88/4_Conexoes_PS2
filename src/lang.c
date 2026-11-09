#include "lang.h"

static int current = LANG_PT;

/* Textos de várias linhas usam '\n'. */
static const char *const STR[S_COUNT][LANG_COUNT] = {
    [S_LANG_NAME]    = { "PORTUGUÊS", "ENGLISH", "ESPAÑOL" },
    [S_SUBTITLE]     = { "ALINHE QUATRO PEÇAS PARA VENCER",
                         "LINE UP FOUR DISCS TO WIN",
                         "ALINEA CUATRO FICHAS PARA GANAR" },
    [S_MENU_CPU]     = { "1 JOGADOR", "1 PLAYER", "1 JUGADOR" },
    [S_MENU_2P]      = { "2 JOGADORES", "2 PLAYERS", "2 JUGADORES" },
    [S_MODE]         = { "MODO", "MODE", "MODO" },
    [S_MODE_CLASSIC] = { "CLÁSSICO", "CLASSIC", "CLÁSICO" },
    [S_MODE_POP]     = { "RETIRADA", "PULL OUT", "RETIRADA" },
    [S_LEVEL]        = { "NÍVEL", "LEVEL", "NIVEL" },
    [S_EASY]         = { "FÁCIL", "EASY", "FÁCIL" },
    [S_MEDIUM]       = { "MÉDIO", "MEDIUM", "MEDIO" },
    [S_HARD]         = { "DIFÍCIL", "HARD", "DIFÍCIL" },
    [S_HELP]         = { "COMO JOGAR", "HOW TO PLAY", "CÓMO JUGAR" },
    [S_LANGUAGE]     = { "IDIOMA", "LANGUAGE", "IDIOMA" },
    [S_YOU]          = { "VOCÊ", "YOU", "TÚ" },
    [S_CPU]          = { "CONSOLE", "CPU", "CONSOLA" },
    [S_P1]           = { "JOGADOR 1", "PLAYER 1", "JUGADOR 1" },
    [S_P2]           = { "JOGADOR 2", "PLAYER 2", "JUGADOR 2" },
    [S_YOUR_TURN]    = { "SUA VEZ!", "YOUR TURN!", "¡TU TURNO!" },
    [S_THINKING]     = { "PENSANDO...", "THINKING...", "PENSANDO..." },
    [S_DRAWS]        = { "EMPATES", "DRAWS", "EMPATES" },
    [S_COL_FULL]     = { "COLUNA CHEIA!", "COLUMN FULL!", "¡COLUMNA LLENA!" },
    [S_POP_INVALID]  = { "SÓ PODE RETIRAR PEÇA SUA DA BASE",
                         "YOU CAN ONLY PULL YOUR OWN BOTTOM DISC",
                         "SOLO PUEDES RETIRAR TU FICHA DE ABAJO" },
    [S_WIN]          = { "VITÓRIA!", "VICTORY!", "¡VICTORIA!" },
    [S_LOSE]         = { "DERROTA!", "DEFEAT!", "¡DERROTA!" },
    [S_DRAW]         = { "EMPATE!", "DRAW!", "¡EMPATE!" },
    [S_WINS_FMT]     = { "%s VENCEU!", "%s WINS!", "¡GANA %s!" },
    [S_PLAY_AGAIN]   = { "JOGAR DE NOVO", "PLAY AGAIN", "JUGAR DE NUEVO" },
    [S_MENU]         = { "MENU", "MENU", "MENÚ" },
    [S_PAUSE]        = { "PAUSA", "PAUSED", "PAUSA" },
    [S_CONTINUE]     = { "CONTINUAR", "RESUME", "CONTINUAR" },
    [S_RESTART]      = { "REINICIAR", "RESTART", "REINICIAR" },
    [S_MAIN_MENU]    = { "MENU PRINCIPAL", "MAIN MENU", "MENÚ PRINCIPAL" },
    [S_H_CHOOSE]     = { "ESCOLHER", "SELECT", "ELEGIR" },
    [S_H_CHANGE]     = { "ALTERAR", "CHANGE", "CAMBIAR" },
    [S_H_CONFIRM]    = { "CONFIRMAR", "CONFIRM", "CONFIRMAR" },
    [S_H_BACK]       = { "VOLTAR", "BACK", "VOLVER" },
    [S_H_COLUMN]     = { "COLUNA", "COLUMN", "COLUMNA" },
    [S_H_DROP]       = { "SOLTAR", "DROP", "SOLTAR" },
    [S_H_POP]        = { "RETIRAR", "PULL", "RETIRAR" },
    [S_H_PAUSE]      = { "PAUSA", "PAUSE", "PAUSA" },
    [S_H_PAGES]      = { "PÁGINAS", "PAGES", "PÁGINAS" },
    [S_PAGE_FMT]     = { "PÁGINA %d/%d", "PAGE %d/%d", "PÁGINA %d/%d" },

    [S_HELP_P1_TITLE] = { "REGRAS", "RULES", "REGLAS" },
    [S_HELP_P1_TEXT] = {
        "- A GRADE TEM 7 COLUNAS E 6 LINHAS.\n"
        "- CADA JOGADOR TEM UMA COR: VERMELHO\n"
        "  OU AMARELO. O VERMELHO COMEÇA.\n"
        "- NA SUA VEZ, SOLTE UMA PEÇA EM UMA\n"
        "  COLUNA: ELA CAI ATÉ A CASA LIVRE\n"
        "  MAIS BAIXA.\n"
        "- VENCE QUEM ALINHAR 4 PEÇAS NA\n"
        "  HORIZONTAL, VERTICAL OU DIAGONAL.\n"
        "- GRADE CHEIA SEM VENCEDOR: EMPATE.",

        "- THE GRID HAS 7 COLUMNS AND 6 ROWS.\n"
        "- EACH PLAYER HAS A COLOR: RED OR\n"
        "  YELLOW. RED GOES FIRST.\n"
        "- ON YOUR TURN, DROP ONE DISC INTO A\n"
        "  COLUMN: IT FALLS TO THE LOWEST\n"
        "  EMPTY SPACE.\n"
        "- THE FIRST TO LINE UP 4 DISCS\n"
        "  HORIZONTALLY, VERTICALLY OR\n"
        "  DIAGONALLY WINS.\n"
        "- FULL GRID WITH NO WINNER: DRAW.",

        "- EL TABLERO TIENE 7 COLUMNAS Y 6 FILAS.\n"
        "- CADA JUGADOR TIENE UN COLOR: ROJO\n"
        "  O AMARILLO. EL ROJO EMPIEZA.\n"
        "- EN TU TURNO, SUELTA UNA FICHA EN UNA\n"
        "  COLUMNA: CAE HASTA EL HUECO LIBRE\n"
        "  MÁS BAJO.\n"
        "- GANA QUIEN ALINEE 4 FICHAS EN\n"
        "  HORIZONTAL, VERTICAL O DIAGONAL.\n"
        "- TABLERO LLENO SIN GANADOR: EMPATE.",
    },
    [S_HELP_P2_TITLE] = { "MODO RETIRADA", "PULL OUT MODE", "MODO RETIRADA" },
    [S_HELP_P2_TEXT] = {
        "- NA SUA VEZ, VOCÊ PODE SOLTAR UMA\n"
        "  PEÇA OU RETIRAR UMA PEÇA SUA DA\n"
        "  LINHA DE BAIXO. AS PEÇAS DESCEM.\n"
        "- SE A RETIRADA FORMAR 4 EM LINHA PARA\n"
        "  OS DOIS, VENCE QUEM RETIROU.\n"
        "- EMPATE SE NÃO HOUVER JOGADA OU\n"
        "  APÓS 120 JOGADAS.\n"
        "- A CADA NOVA PARTIDA, QUEM COMEÇA\n"
        "  ALTERNA.",

        "- ON YOUR TURN, YOU MAY DROP A DISC\n"
        "  OR PULL ONE OF YOUR OWN DISCS OUT\n"
        "  OF THE BOTTOM ROW. THE COLUMN\n"
        "  SLIDES DOWN.\n"
        "- IF A PULL MAKES 4 IN A ROW FOR BOTH\n"
        "  PLAYERS, THE ONE WHO PULLED WINS.\n"
        "- DRAW IF THERE IS NO VALID MOVE OR\n"
        "  AFTER 120 MOVES.\n"
        "- THE STARTING PLAYER ALTERNATES\n"
        "  EVERY ROUND.",

        "- EN TU TURNO, PUEDES SOLTAR UNA FICHA\n"
        "  O RETIRAR UNA FICHA TUYA DE LA FILA\n"
        "  DE ABAJO. LA COLUMNA BAJA.\n"
        "- SI LA RETIRADA FORMA 4 EN LÍNEA PARA\n"
        "  LOS DOS, GANA QUIEN RETIRÓ.\n"
        "- EMPATE SI NO HAY JUGADA VÁLIDA O\n"
        "  TRAS 120 JUGADAS.\n"
        "- QUIEN EMPIEZA SE ALTERNA EN CADA\n"
        "  PARTIDA.",
    },
    [S_HELP_P3_TITLE] = { "CONTROLES", "CONTROLS", "CONTROLES" },
    [S_HELP_P3_TEXT] = {
        "\x01 \x02  ESCOLHER COLUNA\n"
        "\x03  SOLTAR PEÇA\n"
        "\x04  RETIRAR PEÇA (MODO RETIRADA)\n"
        "\x05  PAUSA\n"
        "\n"
        "DOIS JOGADORES: COM DOIS CONTROLES,\n"
        "CADA UM USA O SEU. COM UM CONTROLE,\n"
        "PASSEM O CONTROLE A CADA JOGADA.",

        "\x01 \x02  CHOOSE COLUMN\n"
        "\x03  DROP DISC\n"
        "\x04  PULL DISC (PULL OUT MODE)\n"
        "\x05  PAUSE\n"
        "\n"
        "TWO PLAYERS: WITH TWO CONTROLLERS,\n"
        "EACH PLAYER USES THEIR OWN. WITH ONE,\n"
        "PASS THE CONTROLLER EVERY TURN.",

        "\x01 \x02  ELEGIR COLUMNA\n"
        "\x03  SOLTAR FICHA\n"
        "\x04  RETIRAR FICHA (MODO RETIRADA)\n"
        "\x05  PAUSA\n"
        "\n"
        "DOS JUGADORES: CON DOS MANDOS, CADA\n"
        "UNO USA EL SUYO. CON UN MANDO,\n"
        "PÁSENLO EN CADA TURNO.",
    },
};

void lang_set(int lang)
{
    if (lang >= 0 && lang < LANG_COUNT)
        current = lang;
}

int lang_get(void)
{
    return current;
}

const char *tr_in(int lang, StrId id)
{
    if (id < 0 || id >= S_COUNT || lang < 0 || lang >= LANG_COUNT)
        return "";
    return STR[id][lang] ? STR[id][lang] : STR[id][LANG_PT];
}

const char *tr(StrId id)
{
    return tr_in(current, id);
}
