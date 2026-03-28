/*
 * serializa.c — serialisatio tabulae pro transmissione retiali
 *
 * tabula_ad_json: tabula_t → JSON chorda
 * json_ad_tabulam: JSON chorda → tabula_t
 */

#include "serializa.h"
#include "retis.h"
#include "genera.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- genus ad hex singulum --- */

static char genus_ad_hex(genus_t g)
{
    if (g < 10) return '0' + (char)g;
    return 'a' + (char)(g - 10);
}

/* --- effuge chordam JSON in alveo --- */

static int effuge_in_alveum(char *dest, size_t dest_mag, const char *src)
{
    size_t pos = 0;
    dest[pos++] = '"';
    for (const char *p = src; *p && pos < dest_mag - 2; p++) {
        if (*p == '"' || *p == '\\') {
            if (pos + 2 >= dest_mag - 1) break;
            dest[pos++] = '\\';
            dest[pos++] = *p;
        } else if (*p == '\n') {
            if (pos + 2 >= dest_mag - 1) break;
            dest[pos++] = '\\';
            dest[pos++] = 'n';
        } else {
            dest[pos++] = *p;
        }
    }
    dest[pos++] = '"';
    dest[pos] = '\0';
    return (int)pos;
}

/* --- tabula_ad_json --- */

char *tabula_ad_json(const tabula_t *tab, unsigned long gradus)
{
    int latus = tab->latus;
    int cellulae_num = latus * latus;

    /*
     * magnitudinem aestimamus:
     * - caput ~100
     * - genera: cellulae_num + 16
     * - entia: ~300 per ens, paucae centum entia max
     */
    size_t mag_max = 256 + (size_t)cellulae_num + 128 * 1024;
    char *buf = malloc(mag_max);
    if (!buf) return NULL;

    size_t pos = 0;

    /* caput */
    pos += (size_t)snprintf(buf + pos, mag_max - pos,
        "{\"typus\":\"tabula\",\"gradus\":%lu,\"latus\":%d,\"genera\":\"",
        gradus, latus);

    /* genera chorda — unum signum per cellulam */
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            buf[pos++] = genus_ad_hex(c->genus);
        }
    }
    pos += (size_t)snprintf(buf + pos, mag_max - pos, "\",\"entia\":[");

    /* entia — solum ANIMA et DEI */
    int primum = 1;
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            phylum_t ph = GENERA[c->genus].phylum;
            if (ph != ANIMA && ph != DEI) continue;

            if (!primum) buf[pos++] = ',';
            primum = 0;

            /* campi communes */
            pos += (size_t)snprintf(buf + pos, mag_max - pos,
                "{\"x\":%d,\"y\":%d,\"g\":%d", x, y, (int)c->genus);

            if (ph == ANIMA) {
                char audita_eff[AUDITA_MAX * 2 + 3];
                char mens_eff[MENS_MAX * 2 + 3];
                effuge_in_alveum(audita_eff, sizeof(audita_eff), c->animus.audita);
                effuge_in_alveum(mens_eff, sizeof(mens_eff), c->animus.mens);

                pos += (size_t)snprintf(buf + pos, mag_max - pos,
                    ",\"n\":\"%.7s\",\"sa\":%d,\"vi\":%d,\"vt\":%d"
                    ",\"um\":%d,\"ud\":%d,\"up\":%d"
                    ",\"au\":%s,\"me\":%s}",
                    c->animus.nomen,
                    c->animus.satietas, c->animus.vires, c->animus.vitalitas,
                    c->animus.ultima_modus, c->animus.ultima_directio,
                    c->animus.ultima_permissa,
                    audita_eff, mens_eff);
            } else { /* DEI */
                char audita_eff[AUDITA_MAX * 2 + 3];
                char mens_eff[MENS_MAX * 2 + 3];
                effuge_in_alveum(audita_eff, sizeof(audita_eff), c->deus.audita);
                effuge_in_alveum(mens_eff, sizeof(mens_eff), c->deus.mens);

                pos += (size_t)snprintf(buf + pos, mag_max - pos,
                    ",\"n\":\"%.7s\",\"po\":%d"
                    ",\"um\":%d,\"ud\":%d,\"up\":%d"
                    ",\"au\":%s,\"me\":%s}",
                    c->deus.nomen,
                    c->deus.potentia,
                    c->deus.ultima_modus, c->deus.ultima_directio,
                    c->deus.ultima_permissa,
                    audita_eff, mens_eff);
            }

            if (pos >= mag_max - 512) {
                /* reallocamus si prope finem */
                mag_max *= 2;
                char *novum = realloc(buf, mag_max);
                if (!novum) { free(buf); return NULL; }
                buf = novum;
            }
        }
    }

    pos += (size_t)snprintf(buf + pos, mag_max - pos, "]}");
    return buf;
}

