# Compilar en una computadora con raylib instalado.
#   make RAYLIB=/ruta/a/raylib/src
# (RAYLIB es la carpeta que tiene raylib.h y libraylib.a)

RAYLIB ?= /usr/local/include
CC     ?= gcc
SRC     = $(wildcard src/*.c)
LOGICA  = src/items.c src/enemigos.c src/mapa.c src/finales.c src/logica.c src/debug.c
CFLAGS  = -std=c99 -Wall -Wextra -O2 -I$(RAYLIB)

ifeq ($(OS),Windows_NT)
    EXE  = EscapeZombi.exe
    LIBS = -L$(RAYLIB) -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
else
    EXE  = EscapeZombi
    LIBS = -L$(RAYLIB) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

$(EXE): $(SRC) src/juego.h
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) $(LIBS)

prueba: $(LOGICA) tests/fuzz.c
	$(CC) -std=c99 -Wall -Wextra -g $(LOGICA) tests/fuzz.c -o prueba
	./prueba 2000

clean:
	rm -f $(EXE) prueba

.PHONY: clean prueba
