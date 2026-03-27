/*
 * terminalis.c — tractatio terminalis VT100
 *
 * Modus crudus, signa (SIGINT, SIGTSTP, SIGCONT, SIGWINCH),
 * et pictura tabulae in terminalem.
 */

#include "terminalis.h"
#include "oraculum.h"
#include "utilia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>

/* --- status terminalis --- */

static struct termios prisca;   /* pristina configuratio */
static int initiatum = 0;      /* an modus crudus activus sit */

/* --- vexilla signorum --- */

volatile sig_atomic_t repinge_opus   = 0;
volatile sig_atomic_t finis_opus     = 0;
volatile sig_atomic_t continuatio_opus = 0;

/* --- tractores signorum --- */

static void sigint_tracta(int sig)
{
    (void)sig;
    finis_opus = 1;
}

static void sigtstp_tracta(int sig)
{
    (void)sig;

    /* restaura terminalem antequam sistatur */
    if (initiatum) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &prisca);
        /* ostende cursorem, cursor ad novam lineam */
        const char *seq = "\033[?25h\033[999;1H\n";
        write(STDOUT_FILENO, seq, strlen(seq));
    }

    /* restitue tractorem pristinum, re-mitte signum */
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

static void sigcont_tracta(int sig)
{
    (void)sig;

    /* re-installa tractorem SIGTSTP */
    signal(SIGTSTP, sigtstp_tracta);

    continuatio_opus = 1;
}

static void sigwinch_tracta(int sig)
{
    (void)sig;
    repinge_opus = 1;
}

void signa_installa(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigint_tracta;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigtstp_tracta;
    sa.sa_flags = 0;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = sigcont_tracta;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCONT, &sa, NULL);

    sa.sa_handler = sigwinch_tracta;
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
}

/* --- modus crudus --- */

int terminalis_initia(void)
{
    struct termios cruda;

    if (!isatty(STDIN_FILENO))
        return -1;

    if (!initiatum) {
        /* serva configurationem pristinam semel tantum */
        tcgetattr(STDIN_FILENO, &prisca);
    }

    cruda = prisca;
    cruda.c_lflag &= ~(unsigned long)(ECHO | ICANON);
    /* ISIG manet — ^C et ^Z signa generant */
    cruda.c_cc[VMIN]  = 0;
    cruda.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cruda);
    initiatum = 1;

    /* occulta cursorem, munda screenum */
    const char *seq = "\033[?25l\033[2J";
    write(STDOUT_FILENO, seq, strlen(seq));

    return 0;
}

void terminalis_fini(void)
{
    if (initiatum) {
        const char *seq = "\033[?25h\033[999;1H\n";
        write(STDOUT_FILENO, seq, strlen(seq));
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &prisca);
        initiatum = 0;
    }
}

/* --- pictura --- */

void terminalis_pinge(const tabula_t *tab)
{
    int latus = tab->latus;

    /*
     * Aedifica totam imaginem in memoria, deinde scribe semel
     * ut vacillatio minuatur.
     */
    size_t mag = 8192 + (size_t)(latus + 4) * ((size_t)latus * 12 + 32)
                 + (size_t)(latus * latus) * 128;
    char *buf = malloc(mag);
    if (!buf)
        return;

    char *p = buf;

    /* cursor domum */
    p += sprintf(p, "\033[H");

    /* margo superior */
    p += sprintf(p, "┌");
    for (int x = 0; x < latus; x++)
        p += sprintf(p, "──");
    p += sprintf(p, "┐\n");

    /* cellulae */
    for (int y = 0; y < latus; y++) {
        p += sprintf(p, "│");
        for (int x = 0; x < latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            const char *pic = genera_ops[c->genus].pictura;
            if (pic)
                p += sprintf(p, "%s", pic);
            else
                p += sprintf(p, "  ");
        }
        p += sprintf(p, "│\n");
    }

    /* margo inferior */
    p += sprintf(p, "└");
    for (int x = 0; x < latus; x++)
        p += sprintf(p, "──");
    p += sprintf(p, "┘\n");

    /* panel oraculi ad dextram */
    oraculum_numeri_t on;
    oraculum_numeri(&on);
    oraculum_numeri_modelli_t modelli[8];
    int nmod = oraculum_numeri_per_sapientum(modelli, 8);

    int col = latus * 2 + 4; /* columna post tabulam */
    int versus = 1;

    /* computa latitudines maximas pro dextrorsum iustificatione */
    int lat1 = 1, lat2 = 1, lat3 = 1;
    for (int i = 0; i < nmod; i++) {
        int v;
        v = snprintf(NULL, 0, "%d", modelli[i].volantes);  if (v > lat1) lat1 = v;
        v = snprintf(NULL, 0, "%d", modelli[i].paratae);   if (v > lat1) lat1 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].missae);    if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].successae); if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].errores);   if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].signa_accepta);   if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].signa_recondita); if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].signa_emissa);    if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", modelli[i].signa_cogitata);  if (v > lat3) lat3 = v;
    }
    if (nmod > 1) {
        int v;
        v = snprintf(NULL, 0, "%d", on.pendentes); if (v > lat1) lat1 = v;
        v = snprintf(NULL, 0, "%d", on.paratae);   if (v > lat1) lat1 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_missae);    if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_successae); if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_errores);   if (v > lat2) lat2 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_signa_accepta);   if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_signa_recondita); if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_signa_emissa);    if (v > lat3) lat3 = v;
        v = snprintf(NULL, 0, "%ld", on.summa_signa_cogitata);  if (v > lat3) lat3 = v;
    }

    /* summa solum si plures modelli */
    if (nmod > 1) {
        p += sprintf(p, "\033[%d;%dH" ANSI_BLD ANSI_DIM "── summa oracula ──"
                     ANSI_RST "\033[K", versus++, col);
        p += sprintf(p, "\033[%d;%dH"
                     " " ANSI_CYN "volantes:" ANSI_RST "  %*d"
                     "  " ANSI_GRN "missae:" ANSI_RST "    %*ld"
                     "  " ANSI_MAG "accepta:" ANSI_RST "   %*ldt\033[K",
                     versus++, col,
                     lat1, on.pendentes,
                     lat2, on.summa_missae,
                     lat3, on.summa_signa_accepta);
        p += sprintf(p, "\033[%d;%dH"
                     " " ANSI_CYN "paratae:" ANSI_RST "   %*d"
                     "  " ANSI_GRN "successae:" ANSI_RST " %*ld"
                     "  " ANSI_MAG "recondita:" ANSI_RST " %*ldt\033[K",
                     versus++, col,
                     lat1, on.paratae,
                     lat2, on.summa_successae,
                     lat3, on.summa_signa_recondita);
        p += sprintf(p, "\033[%d;%dH"
                     "             "
                     "  " ANSI_RED "errores:" ANSI_RST "   %*ld"
                     "  " ANSI_MAG "emissa:" ANSI_RST  "    %*ldt\033[K",
                     versus++, col,
                     lat2, on.summa_errores,
                     lat3, on.summa_signa_emissa);
        p += sprintf(p, "\033[%d;%dH"
                     "             "
                     "             "
                     "   " ANSI_MAG "cogitata:" ANSI_RST "  %*ldt\033[K",
                     versus++, col,
                     lat3, on.summa_signa_cogitata);
    }

    /* numeri per sapientum */
    for (int i = 0; i < nmod; i++) {
        p += sprintf(p, "\033[%d;%dH" ANSI_BLD ANSI_YEL "── %s ──"
                     ANSI_RST "\033[K", versus++, col, modelli[i].sapientum);
        p += sprintf(p, "\033[%d;%dH"
                     " " ANSI_CYN "volantes:" ANSI_RST "  %*d"
                     "  " ANSI_GRN "missae:" ANSI_RST "    %*ld"
                     "  " ANSI_MAG "accepta:" ANSI_RST "   %*ldt\033[K",
                     versus++, col,
                     lat1, modelli[i].volantes,
                     lat2, modelli[i].missae,
                     lat3, modelli[i].signa_accepta);
        p += sprintf(p, "\033[%d;%dH"
                     " " ANSI_CYN "paratae:" ANSI_RST "   %*d"
                     "  " ANSI_GRN "successae:" ANSI_RST " %*ld"
                     "  " ANSI_MAG "recondita:" ANSI_RST " %*ldt\033[K",
                     versus++, col,
                     lat1, modelli[i].paratae,
                     lat2, modelli[i].successae,
                     lat3, modelli[i].signa_recondita);
        p += sprintf(p, "\033[%d;%dH"
                     "             "
                     "  " ANSI_RED "errores:" ANSI_RST "   %*ld"
                     "  " ANSI_MAG "emissa:" ANSI_RST "    %*ldt\033[K",
                     versus++, col,
                     lat2, modelli[i].errores,
                     lat3, modelli[i].signa_emissa);
        p += sprintf(p, "\033[%d;%dH"
                     "              "
                     "              "
                     " " ANSI_MAG "cogitata:" ANSI_RST "  %*ldt\033[K",
                     versus++, col,
                     lat3, modelli[i].signa_cogitata);
    }
    /* purga reliquas lineas priorum picturarum */
    for (int r = 0; r < 8; r++)
        p += sprintf(p, "\033[%d;%dH\033[K", versus++, col);

    /* linea status infra tabulam */
    p += sprintf(p, "\033[%d;1H gradus: %lu   latus: %d\033[K\n",
                 latus + 3, tab->gradus_num, latus);
    p += sprintf(p, "\033[K");

    /* animi et mentes eorum — collige et ordina per nomen */
    int versus_animi = latus + 5;
    p += sprintf(p, "\033[%d;1H" ANSI_BLD "── mentes animorum ──" ANSI_RST
                 "\033[K", versus_animi++);

    /* collige omnes animos in indicem */
    typedef struct {
        const char *pictura;
        const char *nomen;
        const char *mens;
        unsigned long mens_gradus;
    } visus_animi_t;
    int cap_animi = latus * latus;
    visus_animi_t *indices = malloc((size_t)cap_animi * sizeof *indices);
    int num_animi = 0;
    for (int y = 0; y < latus; y++) {
        for (int x = 0; x < latus; x++) {
            const cella_t *c = tabula_da_const(tab, x, y);
            phylum_t ph = genera_ops[c->genus].phylum;
            if (ph != ANIMA && ph != DEI)
                continue;
            const char *nom, *men;
            unsigned long mg;
            if (ph == DEI) {
                nom = c->deus.nomen;
                men = c->deus.mens;
                mg  = c->deus.mens_gradus;
            } else {
                nom = c->animus.nomen;
                men = c->animus.mens;
                mg  = c->animus.mens_gradus;
            }
            if (!nom[0])
                continue;
            const char *pic = genera_ops[c->genus].pictura;
            if (!pic) pic = "??";
            indices[num_animi++] = (visus_animi_t){
                .pictura     = pic,
                .nomen       = nom,
                .mens        = men,
                .mens_gradus = mg
            };
        }
    }

    /* ordina per nomen */
    for (int i = 1; i < num_animi; i++) {
        visus_animi_t tmp = indices[i];
        int j = i - 1;
        while (j >= 0 && strcmp(indices[j].nomen, tmp.nomen) > 0) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = tmp;
    }

    /* exhibe */
    for (int i = 0; i < num_animi; i++) {
        if (indices[i].mens[0]) {
            p += sprintf(p, "\033[%d;1H %s " ANSI_CYN "%-4s" ANSI_RST
                         " " ANSI_DIM "[%lu] %s" ANSI_RST "\033[K",
                         versus_animi++, indices[i].pictura,
                         indices[i].nomen, indices[i].mens_gradus,
                         indices[i].mens);
        } else {
            p += sprintf(p, "\033[%d;1H %s " ANSI_CYN "%-4s" ANSI_RST
                         " " ANSI_DIM "—" ANSI_RST "\033[K",
                         versus_animi++, indices[i].pictura,
                         indices[i].nomen);
        }
    }
    free(indices);
    /* purga reliquas lineas priorum picturarum */
    for (int r = 0; r < 4; r++)
        p += sprintf(p, "\033[%d;1H\033[K", versus_animi++);

    write(STDOUT_FILENO, buf, (size_t)(p - buf));
    free(buf);
}

/* --- lectio --- */

int terminalis_lege(void)
{
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1)
        return (int)c;
    return -1;
}
