/*
 * terminalis.h — tractatio terminalis VT100
 *
 * Modus crudus, signa, pictura tabulae.
 */

#ifndef TERMINALIS_H
#define TERMINALIS_H

#include <signal.h>

struct tabula;

/* initia terminalem — modus crudus, cursor occultus */
int terminalis_initia(void);

/* restaura terminalem ad statum pristinum */
void terminalis_fini(void);

/* pinge tabulam in terminalem (necessitat tabula.h) */
void terminalis_pinge(const struct tabula *tab);

/* lege unum characterem sine mora — reddit -1 si nihil */
int terminalis_lege(void);

/* vexilla signorum */
extern volatile sig_atomic_t repinge_opus;
extern volatile sig_atomic_t finis_opus;
extern volatile sig_atomic_t continuatio_opus;

/* installa tractores signorum */
void signa_installa(void);

#endif /* TERMINALIS_H */
