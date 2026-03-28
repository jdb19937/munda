/*
 * coniunge.c — cliens interactivus cum terminali (ut lude, sed per retiam)
 *
 * Usus: ./coniunge [-h hospes] [-p portus] [-c certificatum] [-g genus]
 */

#include "retis/retis.h"
#include "retis/cliens.h"
#include "retis/visus.h"
#include "retis/crudus.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

static int mitte_actio(int fd, sessio_t *ses,
                       int modus, int directio, const char *sermo)
{
    char ison[256];
    if (sermo && sermo[0]) {
        snprintf(ison, sizeof(ison),
                 "{\"typus\":\"actio\",\"modus\":%d,\"directio\":%d,\"sermo\":\"%s\"}",
                 modus, directio, sermo);
    } else {
        snprintf(ison, sizeof(ison),
                 "{\"typus\":\"actio\",\"modus\":%d,\"directio\":%d}",
                 modus, directio);
    }
    return retis_mitte(fd, ses, ison, strlen(ison));
}

int main(int argc, char **argv)
{
    const char *hospes = "127.0.0.1";
    int portus = RETIS_PORTUS;
    const char *via_cert = "certificatum.ison";
    const char *genus_str = "ZODUS";
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

    crudus_signa_installa();
    if (crudus_initia() < 0) {
        fprintf(stderr, "erratum: terminalis non est tty\n");
        close(fd);
        return 1;
    }

    visus_t visus;
    visus_initia(&visus);

    alveus_retis_t alv;
    memset(&alv, 0, sizeof(alv));

    int currens = 1;
    while (currens && !crudus_finis) {

        if (crudus_continuatio) {
            crudus_continuatio = 0;
            crudus_initia();
            crudus_repinge = 1;
        }
        if (crudus_repinge) {
            crudus_repinge = 0;
            write(STDOUT_FILENO, "\033[2J", 4);
            if (visus.genera)
                visus_pinge(&visus);
        }

        struct pollfd fds[2];
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        fds[1].fd = STDIN_FILENO;
        fds[1].events = POLLIN;

        int res = poll(fds, 2, 100);
        if (res < 0) {
            if (!crudus_finis) continue;
            break;
        }

        /* inputum a terminali */
        if (fds[1].revents & POLLIN) {
            int ch;
            while ((ch = crudus_lege()) >= 0) {
                if (ch == 'q' || ch == 'Q') {
                    currens = 0;
                    break;
                }
                if (ch == 12) {
                    write(STDOUT_FILENO, "\033[2J", 4);
                    if (visus.genera)
                        visus_pinge(&visus);
                }

                if (ch == 't' || ch == 'T') {
                    int latus = visus.latus > 0 ? visus.latus : 16;
                    char sermo[32];
                    snprintf(sermo, sizeof(sermo), "%d %d",
                             rand() % latus, rand() % latus);
                    mitte_actio(fd, &sessio, TELEPORTA, DIR_NIHIL, sermo);
                }
                if (ch == '\033') {
                    int ch2 = crudus_lege();
                    if (ch2 == '[') {
                        int ch3 = crudus_lege();
                        switch (ch3) {
                        case 'A': mitte_actio(fd, &sessio, PELLE, SEPTENTRIO, NULL); break;
                        case 'B': mitte_actio(fd, &sessio, PELLE, MERIDIES, NULL);   break;
                        case 'C': mitte_actio(fd, &sessio, PELLE, ORIENS, NULL);     break;
                        case 'D': mitte_actio(fd, &sessio, PELLE, OCCIDENS, NULL);   break;
                        }
                    }
                }
            }
        }

        /* data a servitore */
        if (fds[0].revents & POLLIN) {
            ssize_t r = read(fd, alv.data + alv.pos,
                             sizeof(alv.data) - alv.pos);
            if (r <= 0) {
                currens = 0;
                break;
            }
            alv.pos += (size_t)r;

            uint8_t *payload;
            size_t payload_mag;
            while (retis_lege_frame(&alv, &payload, &payload_mag) == 1) {
                uint8_t *clarus;
                size_t clar_mag;
                if (retis_revela(&sessio, payload, payload_mag,
                                 &clarus, &clar_mag) < 0) {
                    currens = 0;
                    break;
                }

                char *ison = malloc(clar_mag + 1);
                if (!ison) { currens = 0; break; }
                memcpy(ison, clarus, clar_mag);
                ison[clar_mag] = '\0';

                char *typus = ison_da_chordam(ison, "typus");
                if (typus && strcmp(typus, "tabula") == 0) {
                    visus_ex_ison(&visus, ison, clar_mag);
                    visus_pinge(&visus);
                } else if (typus && strcmp(typus, "reiectum") == 0) {
                    char *causa = ison_da_chordam(ison, "causa");
                    crudus_fini();
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

        if (fds[0].revents & (POLLERR | POLLHUP))
            currens = 0;
    }

    crudus_fini();

finis:
    {
        const char *vale = "{\"typus\":\"vale\"}";
        retis_mitte(fd, &sessio, vale, strlen(vale));
    }

    close(fd);
    visus_libera(&visus);
    return 0;
}
