# Makefile — tabula munda

CC      = cc
CFLAGS  = -Wall -Wextra -O2 -I.

# fontes communes
COMMUNES = tabula.c \
          cellae/fixa/vacuum.c cellae/fixa/saxum.c cellae/fixa/murus.c \
          cellae/cibi/rapum.c cellae/cibi/fungus.c \
          cellae/animae/feles.c cellae/animae/dalekus.c cellae/animae/ursus.c cellae/animae/corvus.c \
          cellae/dei/zodus.c cellae/dei/oculus.c cellae/deus.c \
          cella_ops.c cellae/fixum.c cellae/cibus.c cellae/animus.c \
          cogitatio.c json.c utilia.c fictio.c
COMMUNES_OBJ = $(COMMUNES:.c=.o)

CAPITA  = cellula.h cella.h tabula.h terminalis.h oraculum.h json.h cogitatio.h utilia.h fictio.h \
          cellae/animae/feles.h cellae/animae/dalekus.h cellae/animae/ursus.h cellae/animae/corvus.h \
          cellae/fixum.h cellae/cibus.h cellae/animus.h cellae/deus.h \
          cellae/dei/zodus.h cellae/dei/oculus.h

ORACULA_OBJ = oracula/openai.o oracula/xai.o oracula/anthropic.o oracula/fictus.o
ORACULA_FARE = oracula/openai.o oracula/xai.o oracula/anthropic.o oracula/fictus.o
ORACULA_DEP = oracula/provisor.h json.h

# --- crispus (HTTPS sine dependentiis externis) ---

CRISPUS_OBJ = crispus/crispus.o crispus/summa.o crispus/arca.o crispus/numerus.o crispus/velum.o
CRISPUS_DEP = crispus/crispus.h crispus/internum.h

crispus/%.o: crispus/%.c $(CRISPUS_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- omnia ---

omnia: curre lude fare proba valida
all: omnia

# --- curre (sine terminali) ---

curre: curre.o $(COMMUNES_OBJ) oraculum.o $(ORACULA_OBJ) $(CRISPUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# --- lude (cum terminali) ---

lude: lude.o terminalis.o $(COMMUNES_OBJ) oraculum.o $(ORACULA_OBJ) $(CRISPUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(CAPITA)
	$(CC) $(CFLAGS) -c $< -o $@

# --- oraculum ---

oraculum.o: oraculum.c oraculum.h $(ORACULA_DEP) utilia.h $(CRISPUS_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

oracula/%.o: oracula/%.c $(ORACULA_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- fare (imperativum oraculi) ---

fare.o: fare.c oraculum.h
	$(CC) $(CFLAGS) -c $< -o $@

fare: fare.o oraculum.o $(ORACULA_FARE) json.o utilia.o fictio.o $(CRISPUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# --- valida (validator schematum) ---

valida.o: valida.c json.h
	$(CC) $(CFLAGS) -c $< -o $@

valida: valida.o json.o
	$(CC) $(CFLAGS) -o $@ $^

# --- probatio crispus ---

proba: proba.o crispus/proba.o $(CRISPUS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

proba.o: proba.c crispus/proba.h
	$(CC) $(CFLAGS) -c $< -o $@

crispus/proba.o: crispus/proba.c crispus/proba.h $(CRISPUS_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- mundatio ---

mundus:
	rm -f curre.o lude.o terminalis.o $(COMMUNES_OBJ)
	rm -f oraculum.o fare.o
	rm -f $(ORACULA_OBJ) $(CRISPUS_OBJ)
	rm -f crispus/proba.o proba.o
	rm -f valida.o
	rm -f curre lude fare proba valida
clean: mundus

.PHONY: omnia all mundus clean
