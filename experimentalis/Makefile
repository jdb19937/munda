# Makefile — vide, ludus SDL2 experimentalis
#
# Compilat fontes communes ex directorio parente (..)
# et fontes proprios SDL ex hoc directorio.

CC      = cc
CFLAGS  = -Wall -Wextra -O2 -I..

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS   = $(shell sdl2-config --libs)

# fontes communes (ex ..)
COMMUNES = ../tabula.c \
          ../cellae/fixa/vacuum.c ../cellae/fixa/saxum.c ../cellae/fixa/murus.c \
          ../cellae/cibi/rapum.c ../cellae/cibi/fungus.c \
          ../cellae/animae/feles.c ../cellae/animae/dalekus.c ../cellae/animae/ursus.c ../cellae/animae/corvus.c \
          ../cellae/dei/zodus.c ../cellae/dei/oculus.c ../cellae/deus.c \
          ../cella_ops.c ../cellae/fixum.c ../cellae/cibus.c ../cellae/animus.c \
          ../cogitatio.c ../ison.c ../utilia.c ../fictio.c
COMMUNES_OBJ = $(patsubst ../%.c,obj/%.o,$(COMMUNES))

LIBDELPHI = ../delphi/libdelphi.a

CRISPUS_OBJ = obj/crispus/crispus.o obj/crispus/velum.o
ARCANA_OBJ  = obj/arcana/summa.o obj/arcana/arca.o obj/arcana/numerus.o

# --- vide ---

omnia: vide

vide: obj/vide.o obj/fenestra.o $(COMMUNES_OBJ) $(LIBDELPHI) $(CRISPUS_OBJ) $(ARCANA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS)

obj/vide.o: vide.c fenestra.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

obj/fenestra.o: fenestra.c fenestra.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

obj/crispus/%.o: ../crispus/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

obj/arcana/%.o: ../arcana/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: ../%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

purga:
	rm -rf obj vide

.PHONY: omnia purga
