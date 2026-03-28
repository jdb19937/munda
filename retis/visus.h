/*
 * visus.h — imago tabulae pro clientibus
 *
 * Structura levis sine dependentia a cella.h vel tabula.h.
 * Cliens recipit JSON, populat visus_t, pingit ex ea.
 */

#ifndef VISUS_H
#define VISUS_H

#include <stddef.h>
#include "cellula.h"
#include "genera.h"

#define VISUS_ENTIA_MAX  512
#define VISUS_NOMEN_MAX  8
#define VISUS_MENS_MAX   64
#define VISUS_AUDITA_MAX 256

/* unum ens (anima vel deus) in visu */
typedef struct {
    int x, y;
    genus_t genus;
    char nomen[VISUS_NOMEN_MAX];

    /* ANIMA */
    int satietas;
    int vires;
    int vitalitas;

    /* DEI */
    int potentia;

    /* communes */
    int ultima_modus;
    int ultima_directio;
    int ultima_permissa;
    char audita[VISUS_AUDITA_MAX];
    char mens[VISUS_MENS_MAX];
} visus_ens_t;

/* imago tabulae */
typedef struct {
    int latus;
    unsigned long gradus;
    genus_t *genera;            /* latus × latus */
    visus_ens_t entia[VISUS_ENTIA_MAX];
    int entia_num;
} visus_t;

/* crea/libera */
void visus_initia(visus_t *v);
void visus_libera(visus_t *v);

/* populat visum ex JSON; reddit 0 = success */
int visus_ex_json(visus_t *v, const char *json, size_t mag);

/* pinge visum ad terminalem (ANSI, ut terminalis_pinge) */
void visus_pinge(const visus_t *v);

/* pinge visum simpliciter (ASCII, ut curre pinge) */
void visus_pinge_simplex(const visus_t *v);

#endif /* VISUS_H */
