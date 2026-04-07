/*
 * daemonium.c — servitor TCP tabulae mundae
 *
 * Usus: ./daemonium [munda [portus [tempus_ms]]]
 *
 * Currit simulationem et transmittit statum tabulae
 * ad clientes per protocollum retis.
 */

#include "cella.h"
#include "tabula.h"
#include "oraculum.h"
#include "provisor.h"
#include "utilia.h"
#include "retis/retis.h"
#include "retis/serializa.h"
#include "ison.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* definitio globalis tabulae operationum */
genus_ops_t genera_ops[GENERA_NUMERUS];

#define TEMPUS_PRAEFINITUM  100

static volatile sig_atomic_t finis_daemoni = 0;

static void tracta_signum(int sig)
{
    (void)sig;
    finis_daemoni = 1;
}

/* --- cliens retialis --- */

typedef struct {
    int       fd;
    int       activus;     /* 1 = post handshake, -1 = claudendus */
    genus_t   genus;
    char      nomen[8];
    int       deus_x, deus_y;
    actio_t   actio_pendens;
    sessio_t  sessio;
    alveus_retis_t alveus;

    /* handshake: punctum clientis ephemerum (ante derivationem) */
    ec_punctum_t E_c;
} cliens_t;

static cliens_t clienti[RETIS_CLIENTI_MAX];
static int clienti_num = 0;

/* clavis servitoris */
static nm_t servitor_privata;
static ec_punctum_t servitor_publica;

/* --- imperata retis (cogito dispatch) --- */

static actio_t imperata[RETIS_CLIENTI_MAX];
static char    imperata_nomina[RETIS_CLIENTI_MAX][8];
static int     imperata_num = 0;

static void imperata_munda(void)
{
    for (int i = 0; i < imperata_num; i++)
        imperata[i] = ACTIO_NIHIL;
}

static void imperata_pone(const char *nomen, actio_t actio)
{
    for (int i = 0; i < imperata_num; i++) {
        if (strcmp(imperata_nomina[i], nomen) == 0) {
            imperata[i] = actio;
            return;
        }
    }
}

static actio_t imperata_da(const char *nomen)
{
    for (int i = 0; i < imperata_num; i++) {
        if (strcmp(imperata_nomina[i], nomen) == 0) {
            actio_t a   = imperata[i];
            imperata[i] = ACTIO_NIHIL;
            return a;
        }
    }
    return ACTIO_NIHIL;
}

static void imperata_adde(const char *nomen)
{
    if (imperata_num >= RETIS_CLIENTI_MAX)
        return;
    snprintf(imperata_nomina[imperata_num], 8, "%s", nomen);
    imperata[imperata_num] = ACTIO_NIHIL;
    imperata_num++;
}

static void imperata_remove(const char *nomen)
{
    for (int i = 0; i < imperata_num; i++) {
        if (strcmp(imperata_nomina[i], nomen) == 0) {
            imperata_num--;
            if (i < imperata_num) {
                memcpy(imperata_nomina[i], imperata_nomina[imperata_num], 8);
                imperata[i] = imperata[imperata_num];
            }
            return;
        }
    }
}

/* --- cogito retialis (substituit genera_ops[].cogito) --- */

static actio_t retis_cogito(const struct tabula *tab, int x, int y)
{
    const cella_t *c = tabula_da_const(tab, x, y);
    phylum_t ph      = genera_ops[c->genus].phylum;
    if (ph == DEI)
        return imperata_da(c->p.deus.nomen);
    return ACTIO_NIHIL;
}

/* --- quaere vacuum pro deo novo --- */

static int quaere_vacuum(const tabula_t *tab, int *rx, int *ry)
{
    int latus = tab->latus;
    int cx    = latus / 2, cy = latus / 2;

    /* spirale a centro */
    for (int r = 0; r < latus; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dy) != r)
                    continue;
                int x = (cx + dx + latus) % latus;
                int y = (cy + dy + latus) % latus;
                if (tabula_da_const(tab, x, y)->genus == VACUUM) {
                    *rx = x;
                    *ry = y;
                    return 0;
                }
            }
        }
    }
    return -1;
}

/* --- nomina deorum retialium --- */

static int proximus_nomen[GENERA_NUMERUS] = {0};

static void genera_nomen(genus_t genus, char *nomen, size_t mag)
{
    char praefixum = (genus == ZODUS) ? 'Z' : 'O';
    snprintf(nomen, mag, "%c%d", praefixum, proximus_nomen[genus]++);
}

/* --- tracta handshake SALVE clientis --- */

static int tracta_salve(
    cliens_t *cl, const char *ison, size_t mag,
    tabula_t *tab
) {
    (void)mag;

    /* extrahe clavis publica clientis */
    char *hex = ison_da_chordam(ison, "clavis");
    if (!hex)
        return -1;

    if (retis_hex_ad_punctum(hex, &cl->E_c) < 0) {
        free(hex);
        return -1;
    }
    free(hex);

    /* extrahe genus */
    char *genus_str = ison_da_chordam(ison, "genus");
    if (!genus_str)
        return -1;
    if (strcmp(genus_str, "ZODUS") == 0)
        cl->genus = ZODUS;
    else if (strcmp(genus_str, "OCULUS") == 0)
        cl->genus = OCULUS;
    else {
        free(genus_str);
        return -1;
    }
    free(genus_str);

    /* genera ephemeral keypair servitoris */
    nm_t e_s;
    ec_punctum_t E_s;
    retis_genera_clavem(&e_s, &E_s);

    /* mitte SALVE servitoris */
    char hex_s[131];
    retis_punctum_ad_hex(&E_s, hex_s);
    char resp[256];
    snprintf(resp, sizeof(resp), "{\"typus\":\"salve\",\"clavis\":\"%s\"}", hex_s);
    if (retis_mitte_nudum(cl->fd, resp, strlen(resp)) < 0)
        return -1;

    /* computa claves */
    ec_punctum_t eph_communis, stat_communis;
    ec_multiplica(&eph_communis, &e_s, &cl->E_c);
    ec_multiplica(&stat_communis, &servitor_privata, &cl->E_c);

    sessio_t ses_c;
    retis_deriva_claves(
        &eph_communis, &stat_communis,
        &cl->E_c, &E_s, &ses_c, &cl->sessio
    );

    /* pone deum in tabula */
    int dx, dy;
    if (quaere_vacuum(tab, &dx, &dy) < 0) {
        /* tabula plena */
        const char *rej = "{\"typus\":\"reiectum\",\"causa\":\"tabula plena\"}";
        retis_mitte_nudum(cl->fd, rej, strlen(rej));
        return -1;
    }

    genera_nomen(cl->genus, cl->nomen, sizeof(cl->nomen));
    tabula_pone(tab, dx, dy, cl->genus);
    cella_t *c = tabula_da(tab, dx, dy);
    snprintf(c->p.deus.nomen, sizeof(c->p.deus.nomen), "%s", cl->nomen);
    cl->deus_x = dx;
    cl->deus_y = dy;

    /* registra in imperata */
    imperata_adde(cl->nomen);

    /* mitte ACCEPTUM (occultum) */
    char acc[256];
    snprintf(
        acc, sizeof(acc),
        "{\"typus\":\"acceptum\",\"nomen\":\"%s\",\"x\":%d,\"y\":%d,\"latus\":%d}",
        cl->nomen, dx, dy, tab->latus
    );
    if (retis_mitte(cl->fd, &cl->sessio, acc, strlen(acc)) < 0)
        return -1;

    cl->activus = 1;
    fprintf(
        stderr, "[daemonium] %s (%s) conectus ad (%d,%d)\n",
        cl->nomen, cl->genus == ZODUS ? "ZODUS" : "OCULUS", dx, dy
    );

    return 0;
}

/* --- tracta nuntium clientis (post handshake) --- */

static void tracta_nuntium(cliens_t *cl, const char *ison, size_t mag)
{
    (void)mag;

    char *typus = ison_da_chordam(ison, "typus");
    if (!typus)
        return;

    if (strcmp(typus, "actio") == 0) {
        actio_t actio  = ACTIO_NIHIL;
        actio.modus    = (modus_t)ison_da_numerum(ison, "modus");
        actio.directio = (directio_t)ison_da_numerum(ison, "directio");

        char *sermo = ison_da_chordam(ison, "sermo");
        if (sermo) {
            snprintf(actio.sermo, SERMO_MAX, "%s", sermo);
            free(sermo);
        }
        char *mens = ison_da_chordam(ison, "mens");
        if (mens) {
            snprintf(actio.mens, MENS_MAX, "%s", mens);
            free(mens);
        }

        cl->actio_pendens = actio;
    } else if (strcmp(typus, "vale") == 0) {
        cl->activus = -1;
    }

    free(typus);
}

/* --- remove clientem et dele deum --- */

static void remove_clientem(cliens_t *cl, tabula_t *tab)
{
    fprintf(stderr, "[daemonium] %s disconnectus\n", cl->nomen);

    /* dele deum de tabula */
    if (cl->nomen[0]) {
        int latus = tab->latus;
        for (int y = 0; y < latus; y++) {
            for (int x = 0; x < latus; x++) {
                cella_t *c = tabula_da(tab, x, y);
                if (
                    genera_ops[c->genus].phylum == DEI &&
                    strcmp(c->p.deus.nomen, cl->nomen) == 0
                ) {
                    memset(c, 0, sizeof(*c));
                    c->genus = VACUUM;
                }
            }
        }
        imperata_remove(cl->nomen);
    }

    close(cl->fd);

    /* comprime array */
    int idx = (int)(cl - clienti);
    clienti_num--;
    if (idx < clienti_num)
        clienti[idx] = clienti[clienti_num];
}

/* --- quaere novas positiones deorum post gradum --- */

static void renova_positiones(tabula_t *tab)
{
    int latus = tab->latus;
    for (int i = 0; i < clienti_num; i++) {
        cliens_t *cl = &clienti[i];
        if (cl->activus != 1)
            continue;

        int inventum = 0;
        for (int y = 0; y < latus && !inventum; y++) {
            for (int x = 0; x < latus && !inventum; x++) {
                const cella_t *c = tabula_da_const(tab, x, y);
                if (
                    genera_ops[c->genus].phylum == DEI &&
                    strcmp(c->p.deus.nomen, cl->nomen) == 0
                ) {
                    cl->deus_x = x;
                    cl->deus_y = y;
                    inventum   = 1;
                }
            }
        }
        if (!inventum) {
            /* deus periit — mitte reiectum */
            const char *rej = "{\"typus\":\"reiectum\",\"causa\":\"deus tuus periit\"}";
            retis_mitte(cl->fd, &cl->sessio, rej, strlen(rej));
            cl->activus = -1;
        }
    }
}

/* --- pinge statum in stderr (pro debug) --- */

static void pinge_statum(const tabula_t *tab)
{
    int latus = tab->latus;
    int census[GENERA_NUMERUS] = {0};
    for (int y = 0; y < latus; y++)
        for (int x = 0; x < latus; x++)
            census[tabula_da_const(tab, x, y)->genus]++;

    fprintf(
        stderr, "\r[%lu] F:%d U:%d B:%d C:%d cli:%d  ",
        tab->gradus_num,
        census[FELES], census[URSUS], census[DALEKUS],
        census[CORVUS], clienti_num
    );
}

/* --- main --- */

int main(int argc, char **argv)
{
    const char *munda = "mundae/imperium";
    int portus = RETIS_PORTUS;
    int tempus_ms = TEMPUS_PRAEFINITUM;
    const char *via_clavis = "clavis_daemoni.ison";
    int argi = 1;

    if (argi < argc)
        munda     = argv[argi++];
    if (argi < argc)
        portus    = atoi(argv[argi++]);
    if (argi < argc)
        tempus_ms = atoi(argv[argi++]);
    if (tempus_ms < 10)
        tempus_ms = 10;

    /* lege clavem secretam */
    if (
        retis_lege_clavem_secretam(
            via_clavis, &servitor_privata,
            &servitor_publica
        ) < 0
    ) {
        fprintf(stderr, "erratum: non possum legere %s\n", via_clavis);
        fprintf(stderr, "curre ./fac_certificatum primum\n");
        return 1;
    }

    /* installa signa */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = tracta_signum;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    /* initia genera cellularum */
    memset(genera_ops, 0, sizeof(genera_ops));
    vacuum_initia();
    saxum_initia();
    feles_initia();
    dalekus_initia();
    ursus_initia();
    murus_initia();
    rapum_initia();
    fungus_initia();
    zodus_initia();
    oculus_initia();
    corvus_initia();

    /* substitue cogito pro deis retialibus */
    genera_ops[ZODUS].cogito  = retis_cogito;
    genera_ops[OCULUS].cogito = retis_cogito;

    /* initia oraculum */
    if (oraculum_initia() < 0)
        MORIRE("oraculum initiari non potuit");
    oraculum_adde_provisorem(&provisor_fictus);

    srand((unsigned)time(NULL));

    tabula_t *tab = tabula_crea(munda);
    if (!tab)
        MORIRE("memoria defecit");
    tabula_imple(tab);

    /* crea socket servitoris */
    int servitor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servitor_fd < 0)
        MORIRE("socket defecit");

    int opt = 1;
    setsockopt(servitor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)portus);

    if (bind(servitor_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        MORIRE("bind defecit");
    if (listen(servitor_fd, 8) < 0)
        MORIRE("listen defecit");

    /* non-blocking */
    fcntl(servitor_fd, F_SETFL, O_NONBLOCK);

    fprintf(
        stderr, "[daemonium] ausculto in portu %d, munda=%s, tempus=%dms\n",
        portus, munda, tempus_ms
    );

    /* ansa principalis */
    while (!finis_daemoni) {

        /* aedifica pollfd */
        struct pollfd fds[1 + RETIS_CLIENTI_MAX];
        fds[0].fd     = servitor_fd;
        fds[0].events = POLLIN;
        for (int i = 0; i < clienti_num; i++) {
            fds[1 + i].fd     = clienti[i].fd;
            fds[1 + i].events = POLLIN;
        }

        int nfds = 1 + clienti_num;
        poll(fds, (nfds_t)nfds, tempus_ms);

        /* accept novos clientes */
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in cl_addr;
            socklen_t cl_mag = sizeof(cl_addr);
            int cl_fd        = accept(servitor_fd, (struct sockaddr *)&cl_addr, &cl_mag);
            if (cl_fd >= 0) {
                if (clienti_num >= RETIS_CLIENTI_MAX) {
                    const char *rej = "{\"typus\":\"reiectum\",\"causa\":\"nimis multi\"}";
                    retis_mitte_nudum(cl_fd, rej, strlen(rej));
                    close(cl_fd);
                } else {
                    fcntl(cl_fd, F_SETFL, O_NONBLOCK);
                    cliens_t *cl = &clienti[clienti_num++];
                    memset(cl, 0, sizeof(*cl));
                    cl->fd = cl_fd;
                    cl->activus = 0;
                    cl->actio_pendens = ACTIO_NIHIL;
                }
            }
        }

        /* lege a clientibus */
        for (int i = 0; i < clienti_num; i++) {
            if (!(fds[1 + i].revents & (POLLIN | POLLERR | POLLHUP)))
                continue;

            cliens_t *cl = &clienti[i];

            if (fds[1 + i].revents & (POLLERR | POLLHUP)) {
                cl->activus = -1;
                continue;
            }

            /* lege in alveum */
            size_t spatium = sizeof(cl->alveus.data) - cl->alveus.pos;
            if (spatium == 0) {
                cl->activus = -1;
                continue;
            }

            ssize_t r = read(cl->fd, cl->alveus.data + cl->alveus.pos, spatium);
            if (r <= 0) {
                cl->activus = -1;
                continue;
            }
            cl->alveus.pos += (size_t)r;

            /* processa frames completos */
            uint8_t *payload;
            size_t payload_mag;
            while (retis_lege_frame(&cl->alveus, &payload, &payload_mag) == 1) {
                if (cl->activus == 0) {
                    /* handshake */
                    /* NUL-terminat pro ISON */
                    char *ison = malloc(payload_mag + 1);
                    if (ison) {
                        memcpy(ison, payload, payload_mag);
                        ison[payload_mag] = '\0';
                        if (tracta_salve(cl, ison, payload_mag, tab) < 0)
                            cl->activus = -1;
                        free(ison);
                    }
                } else if (cl->activus == 1) {
                    /* decrypta et processa */
                    uint8_t *clarus;
                    size_t clar_mag;
                    if (
                        retis_revela(
                            &cl->sessio, payload, payload_mag,
                            &clarus, &clar_mag
                        ) == 0
                    ) {
                        /* NUL-terminat */
                        char *ison = malloc(clar_mag + 1);
                        if (ison) {
                            memcpy(ison, clarus, clar_mag);
                            ison[clar_mag] = '\0';
                            tracta_nuntium(cl, ison, clar_mag);
                            free(ison);
                        }
                    } else {
                        cl->activus = -1;
                    }
                }
                retis_alveus_consume(&cl->alveus, payload_mag);
            }
        }

        /* remove clientes disconnexos */
        for (int i = clienti_num - 1; i >= 0; i--) {
            if (clienti[i].activus == -1)
                remove_clientem(&clienti[i], tab);
        }

        /* copia imperata clientium */
        imperata_munda();
        for (int i = 0; i < clienti_num; i++) {
            if (clienti[i].activus == 1) {
                imperata_pone(clienti[i].nomen, clienti[i].actio_pendens);
                clienti[i].actio_pendens = ACTIO_NIHIL;
            }
        }

        /* praecogita AI entia */
        dalekus_praecogita(tab);
        ursus_praecogita(tab);
        corvus_praecogita(tab);

        /* gradus simulationis */
        tabula_gradus(tab);

        /* renova positiones deorum */
        renova_positiones(tab);

        /* serializa et broadcast */
        char *ison = tabula_ad_ison(tab, tab->gradus_num);
        if (ison) {
            size_t ison_mag = strlen(ison);
            for (int i = 0; i < clienti_num; i++) {
                if (clienti[i].activus == 1) {
                    if (
                        retis_mitte(
                            clienti[i].fd, &clienti[i].sessio,
                            ison, ison_mag
                        ) < 0
                    ) {
                        clienti[i].activus = -1;
                    }
                }
            }
            free(ison);
        }

        pinge_statum(tab);
    }

    fprintf(stderr, "\n[daemonium] claudo\n");

    /* mitte reiectum ad omnes */
    for (int i = 0; i < clienti_num; i++) {
        if (clienti[i].activus == 1) {
            const char *rej = "{\"typus\":\"reiectum\",\"causa\":\"servitor clauditur\"}";
            retis_mitte(clienti[i].fd, &clienti[i].sessio, rej, strlen(rej));
        }
        close(clienti[i].fd);
    }

    close(servitor_fd);
    oraculum_fini();
    tabula_libera(tab);
    return 0;
}
