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
          cogitatio.c ison.c utilia.c fictio.c
COMMUNES_OBJ = $(COMMUNES:.c=.o)

CAPITA  = cellula.h cella.h tabula.h terminalis.h oraculum.h ison.h cogitatio.h utilia.h fictio.h \
          cellae/animae/feles.h cellae/animae/dalekus.h cellae/animae/ursus.h cellae/animae/corvus.h \
          cellae/fixum.h cellae/cibus.h cellae/animus.h cellae/deus.h \
          cellae/dei/zodus.h cellae/dei/oculus.h

ORACULA_OBJ = oracula/openai.o oracula/xai.o oracula/anthropic.o oracula/fictus.o
ORACULA_DEP = oracula/provisor.h ison.h

# --- omnia (default target) ---

omnia: curre lude fare proba valida daemonium coniunge specta fac_certificatum

# --- arcana (primitiva cryptographica communia) ---

ARCANA_OBJ = arcana/summa.o arcana/arca.o arcana/numerus.o
ARCANA_DEP = arcana/arcana.h

arcana/%.o: arcana/%.c $(ARCANA_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- crispus (HTTPS, TLS — pendet ab arcana) ---

CRISPUS_OBJ = crispus/crispus.o crispus/velum.o
CRISPUS_DEP = crispus/crispus.h crispus/internum.h $(ARCANA_DEP)

crispus/%.o: crispus/%.c $(CRISPUS_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- retis (protocollum TCP) ---

RETIS_OBJ = retis/retis.o retis/serializa.o retis/visus.o
RETIS_DEP = retis/retis.h $(ARCANA_DEP)

retis/retis.o: retis/retis.c $(RETIS_DEP) ison.h
	$(CC) $(CFLAGS) -c $< -o $@

retis/serializa.o: retis/serializa.c retis/serializa.h $(RETIS_DEP) $(CAPITA) genera.h
	$(CC) $(CFLAGS) -c $< -o $@

retis/visus.o: retis/visus.c retis/visus.h genera.h cellula.h ison.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- curre (sine terminali) ---

curre: curre.o $(COMMUNES_OBJ) oraculum.o $(ORACULA_OBJ) $(CRISPUS_OBJ) $(ARCANA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# --- lude (cum terminali) ---

lude: lude.o terminalis.o $(COMMUNES_OBJ) oraculum.o $(ORACULA_OBJ) $(CRISPUS_OBJ) $(ARCANA_OBJ)
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

fare: fare.o oraculum.o $(ORACULA_OBJ) ison.o utilia.o fictio.o $(CRISPUS_OBJ) $(ARCANA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# --- valida (validator schematum) ---

valida.o: valida.c ison.h
	$(CC) $(CFLAGS) -c $< -o $@

valida: valida.o ison.o
	$(CC) $(CFLAGS) -o $@ $^

# --- probatio crispus ---

proba: proba.o crispus/proba.o $(CRISPUS_OBJ) $(ARCANA_OBJ) utilia.o ison.o
	$(CC) $(CFLAGS) -o $@ $^

proba.o: proba.c crispus/proba.h
	$(CC) $(CFLAGS) -c $< -o $@

crispus/proba.o: crispus/proba.c crispus/proba.h $(CRISPUS_DEP)
	$(CC) $(CFLAGS) -c $< -o $@

# --- daemonium (servitor TCP) ---

daemonium: daemonium.o retis/retis.o retis/serializa.o $(COMMUNES_OBJ) oraculum.o $(ORACULA_OBJ) $(CRISPUS_OBJ) $(ARCANA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# --- coniunge (cliens interactivus — sine cella.h/tabula.h) ---

CLIENS_OBJ = retis/retis.o retis/cliens.o retis/visus.o retis/crudus.o ison.o utilia.o $(ARCANA_OBJ)

retis/cliens.o: retis/cliens.c retis/cliens.h $(RETIS_DEP) ison.h
	$(CC) $(CFLAGS) -c $< -o $@

retis/crudus.o: retis/crudus.c retis/crudus.h
	$(CC) $(CFLAGS) -c $< -o $@

coniunge: coniunge.o $(CLIENS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

coniunge.o: coniunge.c retis/retis.h retis/cliens.h retis/visus.h retis/crudus.h genera.h cellula.h ison.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- specta (cliens sine capite — sine cella.h/tabula.h) ---

specta: specta.o $(CLIENS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

specta.o: specta.c retis/retis.h retis/cliens.h retis/visus.h genera.h cellula.h ison.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- fac_certificatum ---

fac_certificatum: fac_certificatum.o retis/retis.o ison.o utilia.o $(ARCANA_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

fac_certificatum.o: fac_certificatum.c retis/retis.h
	$(CC) $(CFLAGS) -c $< -o $@

# --- mundatio ---

purga:
	rm -f curre.o lude.o terminalis.o $(COMMUNES_OBJ)
	rm -f oraculum.o fare.o daemonium.o coniunge.o specta.o fac_certificatum.o
	rm -f $(ORACULA_OBJ) $(CRISPUS_OBJ) $(ARCANA_OBJ)
	rm -f retis/retis.o retis/serializa.o retis/visus.o retis/crudus.o retis/cliens.o
	rm -f crispus/proba.o proba.o valida.o
	rm -f curre lude fare proba valida
	rm -f daemonium coniunge specta fac_certificatum

.PHONY: omnia purga
