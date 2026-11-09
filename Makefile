# 4 Conexões - compilação para PlayStation 2 (PS2SDK + gsKit).
# Requer as variáveis PS2SDK e GSKIT (já definidas na imagem ps2dev/ps2dev).

EE_BIN = 4conexoes.elf
EE_BIN_PACKED = 4conexoes-packed.elf
EE_OBJS = src/main_ps2.o src/platform_ps2.o src/game.o src/board.o \
          src/ai.o src/font.o src/bigfont.o src/lang.o src/audio.o \
          src/assets.o src/irx_ps2.o

GSKIT ?= $(PS2DEV)/gsKit

EE_INCS += -I$(GSKIT)/include
EE_LDFLAGS += -L$(GSKIT)/lib
EE_LIBS = -lgskit -ldmakit -lpad -laudsrv -lpatches -lm
EE_CFLAGS += -O2 -Wall -DAUDSRV_IRX='"$(PS2SDK)/iop/irx/audsrv.irx"'

all: $(EE_BIN)

# Recompila quando a arte ou o módulo de áudio mudarem.
src/assets.o: assets/assets.pak
src/irx_ps2.o: $(PS2SDK)/iop/irx/audsrv.irx

packed: $(EE_BIN_PACKED)

$(EE_BIN_PACKED): $(EE_BIN)
	ps2-packer $< $@

clean:
	rm -f $(EE_BIN) $(EE_BIN_PACKED) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
