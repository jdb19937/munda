/*
 * cliens.h — communes functiones clientium retialium
 *
 * TCP coniunctio et ECDHE handshake.
 */

#ifndef CLIENS_H
#define CLIENS_H

#include "retis.h"

/* conecte ad hospes:portus per TCP; reddit fd vel -1 */
int retis_conecte(const char *hospes, int portus);

/*
 * saluta servitorem: ECDHE handshake cum certificato.
 * populat sessio. reddit 0 = success, -1 = erratum.
 */
int retis_saluta(int fd, const char *genus_str, const char *via_cert,
                 sessio_t *sessio);

#endif /* CLIENS_H */
