/*
 * crispus.c — stratum HTTP et interfacies crispus
 *
 * Praebet CRISPUS (facile), CRISPUSM (multi), et crispus_slist.
 * Multi implementatur per fork()+pipe() (purum POSIX, sine filis).
 *
 * Sine dependentiis externis.
 */

#include "crispus.h"
#include "internum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

/* ================================================================
 *  Structurae internae
 * ================================================================ */

struct crispus_facilis {
    /* optiones */
    char *url;
    char *campi_postae;
    struct crispus_slist *capita;
    size_t (*scribe_fn)(void *, size_t, size_t, void *);
    void *scribe_data;
    long tempus_maximum;

    /* responsum */
    long codex_responsi;
    CRISPUScode exitus;

    /* pro multi: fork */
    pid_t filius;
    int tubus_fd;          /* tubus lectionis a filio */
    int perfectum;
    uint8_t *alveus_tubi;
    size_t alveus_mag;
    size_t alveus_cap;
};

#define MULTI_MAX 64

struct crispus_multi {
    struct crispus_facilis *manubria[MULTI_MAX];
    int numerus;

    /* nuntii perfecti */
    CRISPUSMsg nuntii[MULTI_MAX];
    int nuntii_num;
    int nuntii_idx;
};

/* ================================================================
 *  Resolutio URL
 * ================================================================ */

struct url_partes {
    char hospes[256];
    char via[2048];
    int portus;
};

static int resolve_url(const char *url, struct url_partes *p)
{
    memset(p, 0, sizeof(*p));
    p->portus = 443;
    strcpy(p->via, "/");

    /* praetermitte schema */
    const char *s = url;
    if (strncmp(s, "https://", 8) == 0) s += 8;
    else if (strncmp(s, "http://", 7) == 0) { s += 7; p->portus = 80; }

    /* hospes */
    const char *obliquus = strchr(s, '/');
    const char *duobus = strchr(s, ':');
    size_t hospes_mag;

    if (duobus && (!obliquus || duobus < obliquus)) {
        hospes_mag = (size_t)(duobus - s);
        p->portus = atoi(duobus + 1);
    } else if (obliquus) {
        hospes_mag = (size_t)(obliquus - s);
    } else {
        hospes_mag = strlen(s);
    }

    if (hospes_mag >= sizeof(p->hospes)) hospes_mag = sizeof(p->hospes) - 1;
    memcpy(p->hospes, s, hospes_mag);
    p->hospes[hospes_mag] = '\0';

    if (obliquus)
        snprintf(p->via, sizeof(p->via), "%s", obliquus);

    return 0;
}

/* ================================================================
 *  Coniunctio TCP
 * ================================================================ */

static int coniunge(const char *hospes, int portus)
{
    struct addrinfo indicium, *effectus, *p;
    memset(&indicium, 0, sizeof(indicium));
    indicium.ai_family = AF_UNSPEC;
    indicium.ai_socktype = SOCK_STREAM;

    char portus_str[8];
    snprintf(portus_str, sizeof(portus_str), "%d", portus);

    if (getaddrinfo(hospes, portus_str, &indicium, &effectus) != 0)
        return -1;

    int fd = -1;
    for (p = effectus; p; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(effectus);
    return fd;
}

/* ================================================================
 *  Rogatum HTTP
 * ================================================================ */

/* age rogatum HTTP plenum, reddit CRISPUScode.
 * vocat scribe_fn cum corpore responsi.
 * ponit codex_responsi. */
static CRISPUScode age_rogatum(struct crispus_facilis *f)
{
    struct url_partes url;
    if (resolve_url(f->url, &url) < 0)
        return CRISPUSE_ERRATUM;

    /* coniunge */
    int fd = coniunge(url.hospes, url.portus);
    if (fd < 0) return CRISPUSE_CONIUNCTIO;

    /* tempus maximum per setsockopt */
    if (f->tempus_maximum > 0) {
        struct timeval tv;
        tv.tv_sec = f->tempus_maximum;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

    /* TLS */
    velum_t *vel = velum_crea(fd, url.hospes);
    if (!vel) { close(fd); return CRISPUSE_MEMORIA; }

    if (velum_saluta(vel) < 0) {
        velum_claude(vel);
        close(fd);
        return CRISPUSE_CONIUNCTIO;
    }

    /* construi rogatum HTTP */
    const char *methodus = f->campi_postae ? "POST" : "GET";
    size_t corpus_mag = f->campi_postae ? strlen(f->campi_postae) : 0;

    /* caput rogati */
    char caput[4096];
    int n = snprintf(caput, sizeof(caput),
                     "%s %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Connection: close\r\n",
                     methodus, url.via, url.hospes);

    /* capita usoris */
    for (struct crispus_slist *c = f->capita; c; c = c->proximus)
        n += snprintf(caput + n, sizeof(caput) - (size_t)n,
                      "%s\r\n", c->data);

    if (f->campi_postae)
        n += snprintf(caput + n, sizeof(caput) - (size_t)n,
                      "Content-Length: %zu\r\n", corpus_mag);

    n += snprintf(caput + n, sizeof(caput) - (size_t)n, "\r\n");

    /* mitte caput */
    if (velum_scribe(vel, caput, (size_t)n) < 0) {
        velum_claude(vel);
        close(fd);
        return CRISPUSE_ERRATUM;
    }

    /* mitte corpus */
    if (f->campi_postae && corpus_mag > 0) {
        if (velum_scribe(vel, f->campi_postae, corpus_mag) < 0) {
            velum_claude(vel);
            close(fd);
            return CRISPUSE_ERRATUM;
        }
    }

    /* lege responsum HTTP */
    uint8_t alveus[8192];
    int lectum;
    (void)0;
    uint8_t *resp = NULL;
    size_t resp_mag = 0;

    /* accumula totum responsum */
    while ((lectum = velum_lege(vel, alveus, sizeof(alveus))) > 0) {
        uint8_t *novum = realloc(resp, resp_mag + (size_t)lectum);
        if (!novum) { free(resp); velum_claude(vel); close(fd); return CRISPUSE_MEMORIA; }
        resp = novum;
        memcpy(resp + resp_mag, alveus, (size_t)lectum);
        resp_mag += (size_t)lectum;
        (void)lectum;
    }

    velum_claude(vel);
    close(fd);

    if (!resp || resp_mag == 0) {
        free(resp);
        return CRISPUSE_ERRATUM;
    }

    /* resolve caput responsi */
    f->codex_responsi = 0;

    /* quaere finem capitum (\r\n\r\n) */
    char *finis_capitum = NULL;
    for (size_t i = 0; i + 3 < resp_mag; i++) {
        if (resp[i] == '\r' && resp[i+1] == '\n' &&
            resp[i+2] == '\r' && resp[i+3] == '\n') {
            finis_capitum = (char *)resp + i + 4;
            break;
        }
    }

    if (!finis_capitum) {
        /* nulla capita inventa — trade totum */
        if (f->scribe_fn)
            f->scribe_fn(resp, 1, resp_mag, f->scribe_data);
        free(resp);
        return CRISPUSE_OK;
    }

    /* lege codicem status: "HTTP/1.1 200 OK\r\n" */
    if (resp_mag >= 12 && memcmp(resp, "HTTP/", 5) == 0) {
        const char *sp = memchr(resp, ' ', (size_t)(finis_capitum - (char *)resp));
        if (sp) f->codex_responsi = strtol(sp + 1, NULL, 10);
    }

    /* corpus est post capita */
    size_t corpus_off = (size_t)(finis_capitum - (char *)resp);
    size_t corpus_resp_mag = resp_mag - corpus_off;

    /* quaere Transfer-Encoding: chunked */
    int chunked = 0;
    {
        char *p = (char *)resp;
        char *end = finis_capitum;
        while (p < end) {
            if (strncasecmp(p, "Transfer-Encoding:", 18) == 0) {
                if (strstr(p, "chunked")) chunked = 1;
                break;
            }
            char *nl = strstr(p, "\r\n");
            if (!nl) break;
            p = nl + 2;
        }
    }

    if (chunked) {
        /* resolve chunked encoding */
        uint8_t *src = (uint8_t *)finis_capitum;
        size_t src_mag = corpus_resp_mag;
        size_t i = 0;
        while (i < src_mag) {
            /* lege magnitudinem chunk (hex) */
            size_t chunk_mag = 0;
            while (i < src_mag && src[i] != '\r') {
                uint8_t c = src[i];
                if (c >= '0' && c <= '9') chunk_mag = chunk_mag * 16 + (c - '0');
                else if (c >= 'a' && c <= 'f') chunk_mag = chunk_mag * 16 + 10 + (c - 'a');
                else if (c >= 'A' && c <= 'F') chunk_mag = chunk_mag * 16 + 10 + (c - 'A');
                i++;
            }
            /* praetermitte \r\n */
            if (i + 1 < src_mag) i += 2;
            if (chunk_mag == 0) break;
            if (i + chunk_mag > src_mag) chunk_mag = src_mag - i;
            if (f->scribe_fn)
                f->scribe_fn(src + i, 1, chunk_mag, f->scribe_data);
            i += chunk_mag;
            if (i + 1 < src_mag) i += 2;  /* \r\n post chunk */
        }
    } else {
        /* corpus simplex */
        if (f->scribe_fn && corpus_resp_mag > 0)
            f->scribe_fn(finis_capitum, 1, corpus_resp_mag, f->scribe_data);
    }

    free(resp);
    return CRISPUSE_OK;
}

/* ================================================================
 *  crispus_slist
 * ================================================================ */

struct crispus_slist *crispus_slist_adde(struct crispus_slist *index,
                                          const char *chorda)
{
    struct crispus_slist *novum = malloc(sizeof(*novum));
    if (!novum) return index;
    novum->data = strdup(chorda);
    if (!novum->data) { free(novum); return index; }
    novum->proximus = NULL;

    if (!index) return novum;
    struct crispus_slist *p = index;
    while (p->proximus) p = p->proximus;
    p->proximus = novum;
    return index;
}

void crispus_slist_libera(struct crispus_slist *index)
{
    while (index) {
        struct crispus_slist *prox = index->proximus;
        free(index->data);
        free(index);
        index = prox;
    }
}

/* ================================================================
 *  Interfacies facilis
 * ================================================================ */

CRISPUScode crispus_orbis_initia(long vexilla)
{
    (void)vexilla;
    return CRISPUSE_OK;
}

void crispus_orbis_fini(void)
{
}

CRISPUS *crispus_facilis_initia(void)
{
    struct crispus_facilis *f = calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->tubus_fd = -1;
    f->tempus_maximum = 60;
    return f;
}

void crispus_facilis_fini(CRISPUS *manubrium)
{
    struct crispus_facilis *f = manubrium;
    if (!f) return;
    free(f->url);
    free(f->campi_postae);
    free(f->alveus_tubi);
    if (f->tubus_fd >= 0) close(f->tubus_fd);
    free(f);
}

CRISPUScode crispus_facilis_pone(CRISPUS *manubrium, int optio, ...)
{
    struct crispus_facilis *f = manubrium;
    if (!f) return CRISPUSE_ERRATUM;

    va_list ap;
    va_start(ap, optio);

    switch (optio) {
    case CRISPUSOPT_URL:
        free(f->url);
        f->url = strdup(va_arg(ap, const char *));
        break;
    case CRISPUSOPT_CAMPI_POSTAE:
        free(f->campi_postae);
        f->campi_postae = strdup(va_arg(ap, const char *));
        break;
    case CRISPUSOPT_CAPITA_HTTP:
        f->capita = va_arg(ap, struct crispus_slist *);
        break;
    case CRISPUSOPT_FUNCTIO_SCRIBENDI:
        f->scribe_fn = va_arg(ap, size_t (*)(void *, size_t, size_t, void *));
        break;
    case CRISPUSOPT_DATA_SCRIBENDI:
        f->scribe_data = va_arg(ap, void *);
        break;
    case CRISPUSOPT_TEMPUS:
        f->tempus_maximum = va_arg(ap, long);
        break;
    default:
        va_end(ap);
        return CRISPUSE_ERRATUM;
    }

    va_end(ap);
    return CRISPUSE_OK;
}

CRISPUScode crispus_facilis_age(CRISPUS *manubrium)
{
    struct crispus_facilis *f = manubrium;
    if (!f || !f->url) return CRISPUSE_ERRATUM;
    f->exitus = age_rogatum(f);
    return f->exitus;
}

CRISPUScode crispus_facilis_info(CRISPUS *manubrium, int info, ...)
{
    struct crispus_facilis *f = manubrium;
    if (!f) return CRISPUSE_ERRATUM;

    va_list ap;
    va_start(ap, info);

    switch (info) {
    case CRISPUSINFO_CODEX_RESPONSI: {
        long *p = va_arg(ap, long *);
        *p = f->codex_responsi;
        break;
    }
    default:
        va_end(ap);
        return CRISPUSE_ERRATUM;
    }

    va_end(ap);
    return CRISPUSE_OK;
}

static const char *nuntii_errorum[] = {
    "OK",
    "erratum ignotum",
    NULL, NULL, NULL, NULL, NULL,
    "coniunctio defecit",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    "memoria defecit",
    "tempus excessum"
};

const char *crispus_facilis_error(CRISPUScode codex)
{
    if (codex >= 0 && codex <= 28 && nuntii_errorum[codex])
        return nuntii_errorum[codex];
    return "erratum ignotum";
}

/* ================================================================
 *  Interfacies multi (per fork + pipe)
 * ================================================================ */

CRISPUSM *crispus_multi_initia(void)
{
    struct crispus_multi *m = calloc(1, sizeof(*m));
    return m;
}

void crispus_multi_fini(CRISPUSM *multi)
{
    struct crispus_multi *m = multi;
    if (!m) return;

    for (int i = 0; i < m->numerus; i++) {
        struct crispus_facilis *f = m->manubria[i];
        if (f && f->filius > 0) {
            kill(f->filius, SIGTERM);
            waitpid(f->filius, NULL, 0);
            f->filius = 0;
        }
        if (f && f->tubus_fd >= 0) {
            close(f->tubus_fd);
            f->tubus_fd = -1;
        }
    }
    free(m);
}

/* functio scribendi pro filio (accumulat corpus in memoria) */
static size_t filius_scribe_fn(void *data, size_t mag, size_t nmemb, void *usor)
{
    size_t realis = mag * nmemb;
    struct { uint8_t *data; size_t mag; } *acc = usor;
    uint8_t *novum = realloc(acc->data, acc->mag + realis);
    if (!novum) return 0;
    acc->data = novum;
    memcpy(acc->data + acc->mag, data, realis);
    acc->mag += realis;
    return realis;
}

CRISPUSMcode crispus_multi_adde(CRISPUSM *multi, CRISPUS *facilis)
{
    struct crispus_multi *m = multi;
    struct crispus_facilis *f = facilis;
    if (!m || !f) return CRISPUSM_ERRATUM;
    if (m->numerus >= MULTI_MAX) return CRISPUSM_ERRATUM;

    /* crea tubum */
    int tubi[2];
    if (pipe(tubi) < 0) return CRISPUSM_ERRATUM;

    pid_t filius = fork();
    if (filius < 0) {
        close(tubi[0]);
        close(tubi[1]);
        return CRISPUSM_ERRATUM;
    }

    if (filius == 0) {
        /* --- processus filii --- */
        close(tubi[0]);

        /* claude omnes fd praeter tubum et std */
        for (int fd = 3; fd < 1024; fd++) {
            if (fd != tubi[1]) close(fd);
        }

        /* accumulator corporis in filio */
        struct { uint8_t *data; size_t mag; } acc = { NULL, 0 };

        f->scribe_fn = filius_scribe_fn;
        f->scribe_data = &acc;

        CRISPUScode rc = age_rogatum(f);

        /* scribe ad tubum: [4: rc] [4: codex] [corpus] */
        int32_t val;
        val = (int32_t)rc;
        (void)write(tubi[1], &val, 4);
        val = (int32_t)f->codex_responsi;
        (void)write(tubi[1], &val, 4);
        if (acc.data && acc.mag > 0)
            (void)write(tubi[1], acc.data, acc.mag);
        free(acc.data);

        close(tubi[1]);
        _exit(0);
    }

    /* processus parentis */
    close(tubi[1]);
    f->filius = filius;
    f->tubus_fd = tubi[0];
    f->perfectum = 0;
    f->alveus_tubi = NULL;
    f->alveus_mag = 0;
    f->alveus_cap = 0;

    m->manubria[m->numerus++] = f;
    m->nuntii_num = 0;
    m->nuntii_idx = 0;
    return CRISPUSM_OK;
}

CRISPUSMcode crispus_multi_remove(CRISPUSM *multi, CRISPUS *facilis)
{
    struct crispus_multi *m = multi;
    struct crispus_facilis *f = facilis;
    if (!m || !f) return CRISPUSM_ERRATUM;

    for (int i = 0; i < m->numerus; i++) {
        if (m->manubria[i] == f) {
            if (f->filius > 0) {
                kill(f->filius, SIGTERM);
                waitpid(f->filius, NULL, 0);
                f->filius = 0;
            }
            if (f->tubus_fd >= 0) {
                close(f->tubus_fd);
                f->tubus_fd = -1;
            }
            m->manubria[i] = m->manubria[--m->numerus];
            return CRISPUSM_OK;
        }
    }
    return CRISPUSM_ERRATUM;
}

CRISPUSMcode crispus_multi_age(CRISPUSM *m_pub, int *currentes)
{
    struct crispus_multi *m = (struct crispus_multi *)m_pub;
    if (!m) return CRISPUSM_ERRATUM;

    /* resice nuntios priores */
    m->nuntii_num = 0;
    m->nuntii_idx = 0;

    int vivi = 0;

    /* poll omnes tubos */
    struct pollfd pfds[MULTI_MAX];
    int npfd = 0;
    int indicium[MULTI_MAX];

    for (int i = 0; i < m->numerus; i++) {
        struct crispus_facilis *f = m->manubria[i];
        if (f->perfectum) continue;
        if (f->tubus_fd < 0) continue;
        indicium[npfd] = i;
        pfds[npfd].fd = f->tubus_fd;
        pfds[npfd].events = POLLIN;
        npfd++;
        vivi++;
    }

    if (npfd > 0)
        poll(pfds, (nfds_t)npfd, 0);

    for (int p = 0; p < npfd; p++) {
        int i = indicium[p];
        struct crispus_facilis *f = m->manubria[i];

        if (pfds[p].revents & (POLLIN | POLLHUP)) {
            /* lege quicquid est */
            uint8_t alveus[8192];
            ssize_t r = read(f->tubus_fd, alveus, sizeof(alveus));
            if (r > 0) {
                /* accumula in alveo */
                if (f->alveus_mag + (size_t)r > f->alveus_cap) {
                    size_t nova_cap = f->alveus_cap ? f->alveus_cap * 2 : 4096;
                    while (nova_cap < f->alveus_mag + (size_t)r)
                        nova_cap *= 2;
                    uint8_t *novum = realloc(f->alveus_tubi, nova_cap);
                    if (novum) {
                        f->alveus_tubi = novum;
                        f->alveus_cap = nova_cap;
                    }
                }
                if (f->alveus_tubi) {
                    memcpy(f->alveus_tubi + f->alveus_mag, alveus, (size_t)r);
                    f->alveus_mag += (size_t)r;
                }
            } else {
                /* tubus clausus = filius finivit */
                close(f->tubus_fd);
                f->tubus_fd = -1;

                int status;
                if (f->filius > 0) {
                    waitpid(f->filius, &status, 0);
                    f->filius = 0;
                }

                /* resolve data ex tubo */
                if (f->alveus_tubi && f->alveus_mag >= 8) {
                    int32_t rc_val, codex_val;
                    memcpy(&rc_val, f->alveus_tubi, 4);
                    memcpy(&codex_val, f->alveus_tubi + 4, 4);
                    f->exitus = rc_val;
                    f->codex_responsi = codex_val;

                    /* corpus responsi (post 8 octos) */
                    if (f->alveus_mag > 8 && f->scribe_fn) {
                        f->scribe_fn(f->alveus_tubi + 8, 1,
                                     f->alveus_mag - 8, f->scribe_data);
                    }
                } else {
                    f->exitus = CRISPUSE_ERRATUM;
                    f->codex_responsi = 0;
                }

                f->perfectum = 1;
                vivi--;

                /* adde nuntium */
                if (m->nuntii_num < MULTI_MAX) {
                    CRISPUSMsg *msg = &m->nuntii[m->nuntii_num++];
                    msg->msg = CRISPUSMSG_PERFECTUM;
                    msg->easy_handle = f;
                    msg->data.result = f->exitus;
                }
            }
        }
    }

    if (currentes) *currentes = vivi;
    return CRISPUSM_OK;
}

CRISPUSMsg *crispus_multi_lege(CRISPUSM *m_pub, int *residua)
{
    struct crispus_multi *m = (struct crispus_multi *)m_pub;
    if (!m) { if (residua) *residua = 0; return NULL; }

    if (m->nuntii_idx < m->nuntii_num) {
        CRISPUSMsg *msg = &m->nuntii[m->nuntii_idx++];
        if (residua) *residua = m->nuntii_num - m->nuntii_idx;
        return msg;
    }
    if (residua) *residua = 0;
    return NULL;
}
