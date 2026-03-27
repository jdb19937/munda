/*
 * fictio.c — adiutores pro probatione ficta
 */

#include "fictio.h"
#include <stdlib.h>

static const char *dir_nomina[] = {
    "nihil", "septentrio", "meridies", "occidens", "oriens"
};

const char *fictio_dir_nomen(directio_t dir)
{
    if (dir >= DIR_NIHIL && dir <= ORIENS)
        return dir_nomina[dir];
    return dir_nomina[0];
}

directio_t fictio_dir_fortuita(void)
{
    return (directio_t)(1 + rand() % 4);
}

directio_t fictio_quaere_proximum(const fictio_vicinitas_t *vic, char signum)
{
    int optima_dist = 9999;
    int optima_dx = 0, optima_dy = 0;

    for (int ry = 0; ry < vic->series; ry++) {
        for (int rx = 0; rx < vic->latus; rx++) {
            if (vic->graticula[ry][rx] == signum) {
                int dx = rx - vic->cx;
                int dy = ry - vic->cy;
                int dist = abs(dx) + abs(dy);
                if (dist > 0 && dist < optima_dist) {
                    optima_dist = dist;
                    optima_dx = dx;
                    optima_dy = dy;
                }
            }
        }
    }

    if (optima_dist == 9999) return DIR_NIHIL;

    if (abs(optima_dy) >= abs(optima_dx))
        return (optima_dy < 0) ? SEPTENTRIO : MERIDIES;
    else
        return (optima_dx < 0) ? OCCIDENS : ORIENS;
}

int fictio_vicinum_est(const fictio_vicinitas_t *vic,
                       char signum, directio_t dir)
{
    int tx = vic->cx, ty = vic->cy;

    switch (dir) {
    case SEPTENTRIO: ty--; break;
    case MERIDIES:   ty++; break;
    case OCCIDENS:   tx--; break;
    case ORIENS:     tx++; break;
    default: return 0;
    }

    if (tx < 0 || tx >= vic->latus || ty < 0 || ty >= vic->series)
        return 0;
    return vic->graticula[ty][tx] == signum;
}
