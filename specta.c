/*
 * specta.c — cliens sine capite (ut curre, sed per retiam)
 *
 * Usus: ./specta [-h hospes] [-p portus] [-c certificatum] [-g genus] [-n gradus]
 */

#include "retis/retis.h"
#include "retis/cliens.h"
#include "retis/visus.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

int main(int argc, char **argv)
{
    const char *hospes = "127.0.0.1";
    int portus = RETIS_PORTUS;
    const char *via_cert = "certificatum.ison";
    const char *genus_str = "OCULUS";
    int gradus_max = 0;
    int argi = 1;

    while (argi < argc) {
        if (strcmp(argv[argi], "-h") == 0 && argi + 1 < argc)
            hospes = argv[++argi];
        else if (strcmp(argv[argi], "-p") == 0 && argi + 1 < argc)
            portus = atoi(argv[++argi]);
        else if (strcmp(argv[argi], "-c") == 0 && argi + 1 < argc)
            via_cert = argv[++argi];
        else if (strcmp(argv[argi], "-g") == 0 && argi + 1 < argc)
            genus_str = argv[++argi];
        else if (strcmp(argv[argi], "-n") == 0 && argi + 1 < argc)
            gradus_max = atoi(argv[++argi]);
        argi++;
    }

    int fd = retis_conecte(hospes, portus);
    if (fd < 0) {
        fprintf(stderr, "erratum: non possum conectere ad %s:%d\n", hospes, portus);
        return 1;
    }

    sessio_t sessio;
    memset(&sessio, 0, sizeof(sessio));
    if (retis_saluta(fd, genus_str, via_cert, &sessio) < 0) {
        close(fd);
        return 1;
    }

    visus_t visus;
    visus_initia(&visus);

    alveus_retis_t alv;
    memset(&alv, 0, sizeof(alv));
    int gradus_recepti = 0;

    while (gradus_max == 0 || gradus_recepti < gradus_max) {
        struct pollfd pfd = { .fd = fd, .events = POLLIN };
        int res = poll(&pfd, 1, 5000);
        if (res < 0) break;
        if (res == 0) continue;

        ssize_t r = read(fd, alv.data + alv.pos,
                         sizeof(alv.data) - alv.pos);
        if (r <= 0) break;
        alv.pos += (size_t)r;

        uint8_t *payload;
        size_t payload_mag;
        while (retis_lege_frame(&alv, &payload, &payload_mag) == 1) {
            uint8_t *clarus;
            size_t clar_mag;
            if (retis_revela(&sessio, payload, payload_mag,
                             &clarus, &clar_mag) < 0) {
                fprintf(stderr, "erratum decryptione\n");
                goto finis;
            }

            char *ison = malloc(clar_mag + 1);
            if (!ison) goto finis;
            memcpy(ison, clarus, clar_mag);
            ison[clar_mag] = '\0';

            char *typus = ison_da_chordam(ison, "typus");
            if (typus && strcmp(typus, "tabula") == 0) {
                visus_ex_ison(&visus, ison, clar_mag);
                visus_pinge_simplex(&visus);
                gradus_recepti++;
            } else if (typus && strcmp(typus, "reiectum") == 0) {
                char *causa = ison_da_chordam(ison, "causa");
                fprintf(stderr, "reiectum: %s\n", causa ? causa : "?");
                free(causa);
                free(typus);
                free(ison);
                goto finis;
            }
            free(typus);
            free(ison);
            retis_alveus_consume(&alv, payload_mag);
        }
    }

finis:
    {
        const char *vale = "{\"typus\":\"vale\"}";
        retis_mitte(fd, &sessio, vale, strlen(vale));
    }

    close(fd);
    visus_libera(&visus);
    return 0;
}
