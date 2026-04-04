/*
 * crudus.c — modus crudus terminalis pro clientibus
 *
 * Extractum ex terminalis.c — sine dependentia a tabula/cella/oraculum.
 */

#include "crudus.h"

#include <string.h>
#include <unistd.h>
#include <termios.h>

static struct termios prisca;
static int initiatum = 0;

volatile sig_atomic_t crudus_repinge     = 0;
volatile sig_atomic_t crudus_finis       = 0;
volatile sig_atomic_t crudus_continuatio = 0;

/* --- tractores signorum --- */

static void sigint_tracta(int sig)
{
    (void)sig;
    crudus_finis = 1;
}

static void sigtstp_tracta(int sig)
{
    (void)sig;
    if (initiatum) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &prisca);
        const char *seq = "\033[?25h\033[999;1H\n";
        write(STDOUT_FILENO, seq, strlen(seq));
    }
    signal(SIGTSTP, SIG_DFL);
    raise(SIGTSTP);
}

static void sigcont_tracta(int sig)
{
    (void)sig;
    signal(SIGTSTP, sigtstp_tracta);
    crudus_continuatio = 1;
}

static void sigwinch_tracta(int sig)
{
    (void)sig;
    crudus_repinge = 1;
}

void crudus_signa_installa(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = sigint_tracta;
    sa.sa_flags   = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigtstp_tracta;
    sa.sa_flags   = 0;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = sigcont_tracta;
    sa.sa_flags   = SA_RESTART;
    sigaction(SIGCONT, &sa, NULL);

    sa.sa_handler = sigwinch_tracta;
    sa.sa_flags   = 0;
    sigaction(SIGWINCH, &sa, NULL);
}

int crudus_initia(void)
{
    struct termios cruda;

    if (!isatty(STDIN_FILENO))
        return -1;

    if (!initiatum)
        tcgetattr(STDIN_FILENO, &prisca);

    cruda = prisca;
    cruda.c_lflag &= ~(unsigned long)(ECHO | ICANON);
    cruda.c_cc[VMIN]  = 0;
    cruda.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cruda);
    initiatum = 1;

    const char *seq = "\033[?25l\033[2J";
    write(STDOUT_FILENO, seq, strlen(seq));

    return 0;
}

void crudus_fini(void)
{
    if (initiatum) {
        const char *seq = "\033[?25h\033[999;1H\n";
        write(STDOUT_FILENO, seq, strlen(seq));
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &prisca);
        initiatum = 0;
    }
}

int crudus_lege(void)
{
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1)
        return (int)c;
    return -1;
}
