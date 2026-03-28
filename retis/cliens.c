/*
 * cliens.c — communes functiones clientium retialium
 *
 * TCP coniunctio et ECDHE handshake.
 */

#include "cliens.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int retis_conecte(const char *hospes, int portus)
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

int retis_saluta(int fd, const char *genus_str, const char *via_cert,
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

    /* lege SALVE servitoris */
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

    /* lege ACCEPTUM (occultum) — verificat authenticatio servitoris */
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
