/*
 * crudus.h — modus crudus terminalis pro clientibus
 *
 * Initia/fini modus crudus, lege characterem, signa.
 * Sine dependentia a tabula.h, cella.h, oraculum.h.
 */

#ifndef CRUDUS_H
#define CRUDUS_H

#include <signal.h>

int  crudus_initia(void);
void crudus_fini(void);
int  crudus_lege(void);

extern volatile sig_atomic_t crudus_repinge;
extern volatile sig_atomic_t crudus_finis;
extern volatile sig_atomic_t crudus_continuatio;

void crudus_signa_installa(void);

#endif /* CRUDUS_H */
