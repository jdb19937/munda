/*
 * nativus.h — transformer nativus: typi communes
 *
 * Architectura Llama-2: RMSNorm, RoPE, SwiGLU, GQA.
 * Operationes matriciae per phantasma/computo.h (pfr_matvec_f etc.).
 */

#ifndef NATIVUS_H
#define NATIVUS_H

#include <stddef.h>

/* ================================================================
 * configuratio
 * ================================================================ */

typedef struct {
    int dimensio;        /* d_model */
    int dimensio_occ;    /* d_ff (FFN hidden) */
    int strata;          /* num layers */
    int capita;          /* query heads */
    int capita_kv;       /* key/value heads */
    int vocab_magnitudo; /* vocabulary size */
    int longitudo_max;   /* max sequence length */
} nm_config_t;

/* ================================================================
 * pondera — pointers into mmap'd or malloc'd flat buffer
 *
 * Layout in buffer (consecutive, row-major):
 *   vestigia  (vocab_magnitudo, dimensio)
 *   rms_att   (strata, dimensio)
 *   rms_ffn   (strata, dimensio)
 *   wq        (strata, dimensio, dimensio)
 *   wk        (strata, dim_kv, dimensio)    dim_kv = (dimensio/capita)*capita_kv
 *   wv        (strata, dim_kv, dimensio)
 *   wo        (strata, dimensio, dimensio)
 *   w1        (strata, dimensio_occ, dimensio)
 *   w2        (strata, dimensio, dimensio_occ)
 *   w3        (strata, dimensio_occ, dimensio)
 *   rms_finis (dimensio)
 * ================================================================ */

typedef struct {
    float *vestigia;
    float *rms_att;
    float *rms_ffn;
    float *wq;
    float *wk;
    float *wv;
    float *wo;
    float *w1;
    float *w2;
    float *w3;
    float *rms_finis;
    float *wvoc;   /* == vestigia (shared weights) or separate alloc */
} nm_pondera_t;

/* ================================================================
 * status cursus — activationes et cache KV
 * ================================================================ */

typedef struct {
    float *x;            /* (dimensio,) */
    float *xb;           /* (dimensio,) */
    float *xb2;          /* (dimensio,) */
    float *hb;           /* (dimensio_occ,) */
    float *hb2;          /* (dimensio_occ,) */
    float *q;            /* (dimensio,) */
    float *k;            /* (dim_kv,) */
    float *v;            /* (dim_kv,) */
    float *att;          /* (capita, longitudo_max) */
    float *logitae;      /* (vocab_magnitudo,) */
    float *cache_clavis; /* (strata, longitudo_max, dim_kv) */
    float *cache_valor;  /* (strata, longitudo_max, dim_kv) */
} nm_status_t;

/* ================================================================
 * nm_t — exemplar completum
 * ================================================================ */

#define NM_SIGNUM_MAGICUM 0x4E4D4131u  /* 'NMA1' */

typedef struct {
    nm_config_t  config;
    nm_pondera_t pondera;
    nm_status_t  status;
    int          pondera_communes;  /* 1: wvoc == vestigia */
    float       *data;              /* flat buffer (mallocata) */
    size_t       magnitudo_data;
} nm_t;

/* ================================================================
 * nm_exercitatio_t — status exercitationis (gradientes + AdamW)
 * ================================================================ */

typedef struct {
    /* gradientes (acum. ante passus AdamW) */
    float       *grad_data;
    nm_pondera_t grad;
    /* impetus primum AdamW (m) */
    float       *impetus_data;
    nm_pondera_t impetus;
    /* vis secundum AdamW (v) */
    float       *vis_data;
    nm_pondera_t vis;

    /* activationes memoratae pro retropulsione (dimensionibus unius positionis) */
    float *memo_x;       /* (strata+1, dimensio) — x ante quodque stratum */
    float *memo_xb_att;  /* (strata, dimensio)   — post rmsnorm att */
    float *memo_xb_ffn;  /* (strata, dimensio)   — post rmsnorm ffn */
    float *memo_q;       /* (strata, dimensio)   — q post RoPE */
    float *memo_k;       /* (strata, dim_kv)     — k ad positionem currentem */
    float *memo_att;     /* (strata, capita, longitudo_max) — gradus attentionis */
    float *memo_hb;      /* (strata, dimensio_occ) — w1 @ xb_ffn (pre-SwiGLU) */
    float *memo_hb2;     /* (strata, dimensio_occ) — w3 @ xb_ffn */
    float *memo_xfin;    /* (dimensio) — x post rmsnorm finis */

    int  passus;           /* numerus passuum AdamW (pro bias correction) */
    long signa_processata; /* signa processata in toto */
} nm_exercitatio_t;

/* ================================================================
 * functiones (nativus_cursus.c)
 * ================================================================ */

/*
 * nm_magnitudo_ponderum — numeri floatium pro uno exemplo ponderum.
 * Si pondera_communes, wvoc non computatur.
 */
size_t nm_magnitudo_ponderum(const nm_config_t *c, int communes);

/*
 * nm_pondera_init_ptr — ponit omnes ptrs in p ut indicent in data.
 * Rediturus: pointer post ultimas pondera.
 */
float *nm_pondera_init_ptr(nm_pondera_t *p, const nm_config_t *c, float *data);

/*
 * nm_initia_temere — creat nm_t novum cum ponderibus temerariis.
 * semen: semen RNG pro initializatione. Reddit 0 si successum.
 */
int nm_initia_temere(nm_t *nm, const nm_config_t *config, unsigned int semen);

/*
 * nm_lege — legit nm_t ex plica binaria. Reddit 0 si successum.
 */
int nm_lege(nm_t *nm, const char *via);

/*
 * nm_serva — servat nm_t in plicam binariam. Reddit 0 si successum.
 */
int nm_serva(const nm_t *nm, const char *via);

/*
 * nm_fini — liberat omnes memorias in nm_t.
 */
void nm_fini(nm_t *nm);

/*
 * nm_status_restitue — ponit omnes activationes et cache KV in 0.
 */
void nm_status_restitue(nm_t *nm);

/*
 * nm_gpu_initia — allocat GPU buffers pro ponderibus et activationibus.
 * Pondera semel mittuntur; activationes syncuntur per-operationem.
 * Si GPU non adest, reddit -1 et omnes operationes in CPU manent.
 * Applicatio non debet curare — eadem API cum vel sine GPU.
 */
int nm_gpu_initia(nm_t *nm);

/*
 * nm_gpu_fini — liberat GPU buffers.
 */
void nm_gpu_fini(void);

/*
 * nm_cursus — passus ante pro inferentiis (updateat nm->status.logitae).
 * reddit nm->status.logitae (non allocatum separatim).
 */
float *nm_cursus(nm_t *nm, int signum, int positio);

/*
 * nm_cursus_memo — passus ante pro exercitatione; servat activationes in ex.
 * reddit nm->status.logitae.
 */
float *nm_cursus_memo(nm_t *nm, nm_exercitatio_t *ex, int signum, int positio);

/* ================================================================
 * functiones (nativus_exercitatio.c)
 * ================================================================ */

/*
 * nm_exercitatio_initia — allocat gradientes, momenta, memorias.
 * Reddit 0 si successum.
 */
int  nm_exercitatio_initia(nm_exercitatio_t *ex, const nm_config_t *c);

/*
 * nm_exercitatio_fini — liberat omnes memorias in ex.
 */
void nm_exercitatio_fini(nm_exercitatio_t *ex);

/*
 * nm_gradientes_pone_nihil — ponit omnes gradientes in 0 ante novum manipulum.
 */
void nm_gradientes_pone_nihil(nm_exercitatio_t *ex, const nm_config_t *c);

/*
 * nm_retropulsio — computat gradientes pro una positione.
 * signum: signum intratum (pro gradu vestigiorum).
 * signum_target: signum expectatum (index in vocabulario).
 * Accumulat in ex->grad. Reddit damnum (cross-entropy loss).
 */
float nm_retropulsio(nm_t *nm, nm_exercitatio_t *ex,
                     int signum, int signum_target, int positio);

/*
 * nm_gradientes_tonde — tondetur norma globaliter ad max_norma (gradient clipping).
 * reddit normam ante tonsionem.
 */
float nm_gradientes_tonde(nm_exercitatio_t *ex, const nm_config_t *c,
                          float max_norma);

/*
 * nm_passus_adami — passus AdamW: updateat pondera, resetat gradientes.
 */
void nm_passus_adami(nm_t *nm, nm_exercitatio_t *ex,
                     float gradus_discendi, float beta1, float beta2,
                     float epsilon, float desicatio);

#endif /* NATIVUS_H */
