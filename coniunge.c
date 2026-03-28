/*
 * coniunge.c — cliens interactivus cum terminali (ut lude, sed per retiam)
 *
 * Usus: ./coniunge [-h hospes] [-p portus] [-c certificatum] [-g genus]
 *
 * Nulla dependentia a cella.h, tabula.h, oraculum.h.
 */

#include "retis/retis.h"
#include "retis/visus.h"
#include "retis/crudus.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static int conecte(const char *hospes, int portus)
{
    struct addrinfo suggestiones, *res;
    memset(&suggestiones, 0, sizeof(suggestiones));
    suggestiones.ai_family = AF_INET;
    suggestiones.ai_socktype = SOCK_STREAM;

    char portus_str[16];
    snprintf(portus_str, sizeof(portus_str), "%d", portus);

    if (getaddrinfo(hospes, portus_str, &suggestiones, &res) != 0)
        return -1;

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return -1; }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

static int saluta(int fd, const char *genus_str, const char *via_cert,
                  sessio_t *sessio)
{
    ec_punctum_t pub_s;
    if (retis_lege_certificatum(via_cert, &pub_s) < 0) {
        fprintf(stderr, "erratum: non possum legere certificatum %s\n", via_cert);
        return -1;
    }

    nm_t e_c;
    ec_punctum_t E_c;
    retis_genera_clavem(&e_c, &E_c);

    char hex_c[131];
    retis_punctum_ad_hex(&E_c, hex_c);
    char salve[256];
    snprintf(salve, sizeof(salve),
             "{\"typus\":\"salve\",\"clavis\":\"%s\",\"genus\":\"%s\"}",
             hex_c, genus_str);
    if (retis_mitte_nudum(fd, salve, strlen(salve)) < 0)
        return -1;

    alveus_retis_t alv;
    memset(&alv, 0, sizeof(alv));

    for (;;) {
        ssize_t r = read(fd, alv.data + alv.pos,
                         sizeof(alv.data) - alv.pos);
        if (r <= 0) return -1;
        alv.pos += (size_t)r;

        uint8_t *payload;
        size_t payload_mag;
        int res = retis_lege_frame(&alv, &payload, &payload_mag);
        if (res < 0) return -1;
        if (res == 1) {
            char *json = malloc(payload_mag + 1);
            if (!json) return -1;
            memcpy(json, payload, payload_mag);
            json[payload_mag] = '\0';

            char *hex_s = json_da_chordam(json, "clavis");
            free(json);
            if (!hex_s) return -1;

            ec_punctum_t E_s;
            if (retis_hex_ad_punctum(hex_s, &E_s) < 0) {
                free(hex_s);
                return -1;
            }
            free(hex_s);

            ec_punctum_t eph_communis, stat_communis;
            ec_multiplica(&eph_communis, &e_c, &E_s);
            ec_multiplica(&stat_communis, &e_c, &pub_s);

            sessio_t ses_s;
            retis_deriva_claves(&eph_communis, &stat_communis,
                                &E_c, &E_s, sessio, &ses_s);

            retis_alveus_consume(&alv, payload_mag);
            break;
        }
    }

    /* lege ACCEPTUM */
    for (;;) {
        uint8_t *payload;
        size_t payload_mag;
        int res = retis_lege_frame(&alv, &payload, &payload_mag);
        if (res < 0) return -1;
        if (res == 1) {
            uint8_t *clarus;
            size_t clar_mag;
            if (retis_revela(sessio, payload, payload_mag,
                             &clarus, &clar_mag) < 0) {
                fprintf(stderr, "erratum: handshake defecit (servitor falsus?)\n");
                return -1;
            }
            retis_alveus_consume(&alv, payload_mag);
            return 0;
        }
        ssize_t r = read(fd, alv.data + alv.pos,
                         sizeof(alv.data) - alv.pos);
        if (r <= 0) return -1;
        alv.pos += (size_t)r;
    }
}

static int mitte_actio(int fd, sessio_t *ses,
                       int modus, int directio, const char *sermo)
{
    char json[256];
    if (sermo && sermo[0]) {
        snprintf(json, sizeof(json),
                 "{\"typus\":\"actio\",\"modus\":%d,\"directio\":%d,\"sermo\":\"%s\"}",
                 modus, directio, sermo);
    } else {
        snprintf(json, sizeof(json),
                 "{\"typus\":\"actio\",\"modus\":%d,\"directio\":%d}",
                 modus, directio);
    }
    return retis_mitte(fd, ses, json, strlen(json));
}

int main(int argc, char **argv)
{
    const char *hospes = "127.0.0.1";
    int portus = RETIS_PORTUS;
    const char *via_cert = "certificatum.json";
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

    int fd = conecte(hospes, portus);
    if (fd < 0) {
        fprintf(stderr, "erratum: non possum conectere ad %s:%d\n", hospes, portus);
        return 1;
    }

    sessio_t sessio;
    memset(&sessio, 0, sizeof(sessio));
    if (saluta(fd, genus_str, via_cert, &sessio) < 0) {
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

                char *json = malloc(clar_mag + 1);
                if (!json) { currens = 0; break; }
                memcpy(json, clarus, clar_mag);
                json[clar_mag] = '\0';

                char *typus = json_da_chordam(json, "typus");
                if (typus && strcmp(typus, "tabula") == 0) {
                    visus_ex_json(&visus, json, clar_mag);
                    visus_pinge(&visus);
                } else if (typus && strcmp(typus, "reiectum") == 0) {
                    char *causa = json_da_chordam(json, "causa");
                    crudus_fini();
                    fprintf(stderr, "reiectum: %s\n", causa ? causa : "?");
                    free(causa);
                    free(typus);
                    free(json);
                    goto finis;
                }
                free(typus);
                free(json);
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
