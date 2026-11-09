# 4 Conexões (PlayStation 2)

Jogo de estratégia para PlayStation 2 em que dois jogadores tentam alinhar quatro peças numa grade vertical. Usa as mecânicas do clássico "ligue 4" (Connect 4, da Hasbro), com arte, código e nome próprios.

![Tela de título](docs/screens/titulo.png)

## Regras

Regras clássicas pesquisadas nas instruções do jogo original:

- A grade vertical tem **7 colunas e 6 linhas**.
- Cada jogador usa uma cor: **vermelho** (jogador 1) ou **amarelo** (jogador 2).
- Na sua vez, o jogador solta **uma peça** numa coluna. A peça cai até a casa livre mais baixa.
- Vence quem formar primeiro **4 peças da sua cor em linha**: na horizontal, na vertical ou na diagonal.
- Se a grade encher sem nenhuma linha de 4, a partida termina **empatada**.
- Não dá para jogar numa coluna cheia.
- O vermelho começa a primeira partida. Nas seguintes, quem começa alterna.

### Modo Retirada (variante "Pop Out")

A caixa do jogo original traz modos extras. O 4 Conexões tem um baseado no *Pop Out*:

- Na sua vez, você pode **soltar** uma peça ou **retirar** uma peça **sua** da linha de baixo. As peças daquela coluna descem uma casa.
- Se uma retirada formar 4 em linha para os dois jogadores ao mesmo tempo, **vence quem retirou**.
- Se uma retirada completar só a linha do adversário, vence o adversário.
- Regra da casa, para impedir partidas infinitas: a partida termina empatada se o jogador da vez não tiver nenhuma jogada válida ou depois de 120 jogadas.

| Partida | Vitória | Modo Retirada |
|---|---|---|
| ![](docs/screens/partida.png) | ![](docs/screens/vitoria.png) | ![](docs/screens/retirada.png) |

## Recursos

- **1 jogador contra o console**, com três níveis: Fácil, Médio e Difícil. A IA usa busca minimax com poda alfa-beta.
- **2 jogadores**. Com dois controles conectados, cada jogador usa o seu. Com um só, os dois revezam o mesmo controle.
- Placar de vitórias e empates durante a sessão.
- Animação da peça caindo (com quique) e da retirada.
- Prévia da casa onde a peça vai cair. A linha vencedora pisca no fim da partida.
- Tela "Como jogar" com regras e controles. Todos os textos estão em português, com acentos.
- Funciona em NTSC e PAL: o layout se ajusta à resolução.

## Controles

| Botão | Ação |
|---|---|
| Direcional ← → | Escolher coluna / alterar opção |
| Direcional ↑ ↓ | Navegar nos menus |
| ✕ (X) | Soltar peça / confirmar |
| □ (Quadrado) | Retirar peça (Modo Retirada) |
| ○ (Círculo) | Voltar |
| START | Pausar (continuar, reiniciar, menu) |

## Estrutura do projeto

```
src/
  board.c/h          regras: gravidade, turnos, vitória, empate, retirada
  ai.c/h             adversário: negamax com poda alfa-beta
  font.c/h           fonte bitmap 5x7 com acentos (UTF-8)
  game.c/h           telas, menus, animações e desenho
  platform.h         interface de vídeo e controles
  platform_ps2.c     implementação para PS2 (gsKit + libpad)
  main_ps2.c         ponto de entrada no PS2
host/                versão para PC (renderização por software) usada em
                     testes e capturas de tela
tests/test_board.c   testes de regras e IA
```

A lógica (`board`, `ai`, `game`, `font`) não depende do hardware. Toda a parte específica do PS2 fica em `platform_ps2.c`.

## Compilação

### PlayStation 2

É preciso ter o [PS2SDK](https://github.com/ps2dev/ps2dev) com gsKit. O jeito mais simples é a imagem Docker oficial:

```sh
docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest \
  sh -c "apk add --no-cache build-base && make"
```

O comando gera o `4conexoes.elf`. `make packed` gera uma versão comprimida com `ps2-packer`.

Toda vez que há um push, o GitHub Actions (`.github/workflows/build.yml`) compila o ELF e o publica como artefato `4conexoes-ps2`.

### Como rodar

- **Emulador:** no PCSX2, use *Sistema → Executar ELF* e escolha `4conexoes.elf`.
- **Console:** copie o ELF para um pendrive e abra pelo uLaunchELF (via FreeMcBoot, por exemplo).
- **ps2link:** `make run`, que usa `ps2client`.

### Testes no PC

```sh
make -f Makefile.host test   # testes de regras e IA
make -f Makefile.host host   # versão PC para gerar capturas
./4conexoes_host DX X w30 shot:tela.ppm
```

O executável do PC aceita comandos de controle: `L R U D X O Q T S`, `w<N>` para esperar N quadros e `shot:<arquivo>` para salvar a tela em PPM.

---

*Connect 4 é marca registrada da Hasbro. Este é um projeto de fã, independente, sem nenhuma afiliação com a Hasbro.*
