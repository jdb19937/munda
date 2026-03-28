/*
 * fac_certificatum.c — genera par clavium EC P-256
 *
 * scribit:
 *   certificatum.ison   — clavis publica (datur clientibus)
 *   clavis_daemoni.ison — clavis secreta + publica (servitor solum)
 */

#include "retis/retis.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    const char *via_pub = "certificatum.ison";
    const char *via_sec = "clavis_daemoni.ison";

    if (argc > 1) via_pub = argv[1];
    if (argc > 2) via_sec = argv[2];

    nm_t privata;
    ec_punctum_t publica;
    retis_genera_clavem(&privata, &publica);

    if (retis_scribe_certificatum(via_pub, &publica) < 0) {
        fprintf(stderr, "erratum: non possum scribere %s\n", via_pub);
        return 1;
    }

    if (retis_scribe_clavem_secretam(via_sec, &privata, &publica) < 0) {
        fprintf(stderr, "erratum: non possum scribere %s\n", via_sec);
        return 1;
    }

    fprintf(stderr, "certificatum scriptum: %s\n", via_pub);
    fprintf(stderr, "clavis secreta scripta: %s\n", via_sec);
    return 0;
}
