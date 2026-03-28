/*
 * fenestra.c — stratum graphicum SDL2 cum imaginibus proceduralibus
 *
 * Quaeque creatura habet formam propriam recognoscibilem:
 * feles cum auribus et oculis, ursus cum rostro, dalekus metallicus,
 * corvus cum alis, fungus cum pileo, rapum cum foliis, etc.
 *
 * Lumen procedit a superiore-sinistro.
 */

#include "fenestra.h"
#include "cella.h"
#include "tabula.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --- typi --- */

typedef struct { Uint8 r, g, b; } color_t;

/* --- status SDL --- */

static SDL_Window   *fenestra      = NULL;
static SDL_Renderer *pictor        = NULL;
static SDL_Texture  *textura       = NULL;
static int           clausa_est    = 0;
static int           mag_cellulae  = 32;
static Uint32       *punctula      = NULL;
static int           lat_img       = 0;
static unsigned long numerus_picturae = 0;

/* --- strepitus (pseudo-fortuitus determinatus) --- */

static float strepitus(int x, int y, int semen)
{
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263 + semen * 1274126177);
    h = (h ^ (h >> 13)) * 1103515245u;
    h = h ^ (h >> 16);
    return (float)(h & 0xFFFFu) / 65535.0f;
}

/* --- auxiliares pictoriae --- */

static inline Uint32 fac_colorem(Uint8 r, Uint8 g, Uint8 b)
{
    return 0xFF000000u | ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

static inline void pone_punctulum(int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (x < 0 || y < 0 || x >= lat_img || y >= lat_img) return;
    int idx = y * lat_img + x;
    if (a == 255) {
        punctula[idx] = fac_colorem(r, g, b);
        return;
    }
    if (a == 0) return;
    Uint32 f = punctula[idx];
    Uint8 fr = (f >> 16) & 0xFF, fg = (f >> 8) & 0xFF, fb = f & 0xFF;
    Uint8 nr = (Uint8)((r * a + fr * (255 - a)) / 255);
    Uint8 ng = (Uint8)((g * a + fg * (255 - a)) / 255);
    Uint8 nb = (Uint8)((b * a + fb * (255 - a)) / 255);
    punctula[idx] = fac_colorem(nr, ng, nb);
}

/* pinge ellipsem mollem (antialiased) */
static void pinge_ellipsem(int cx, int cy, float rx, float ry,
                           Uint8 r, Uint8 g, Uint8 b, Uint8 alpha_max)
{
    int irx = (int)(rx + 1.5f), iry = (int)(ry + 1.5f);
    for (int dy = -iry; dy <= iry; dy++) {
        for (int dx = -irx; dx <= irx; dx++) {
            float ex = (float)dx / rx;
            float ey = (float)dy / ry;
            float d = sqrtf(ex * ex + ey * ey);
            Uint8 a;
            if (d < 0.9f)
                a = alpha_max;
            else if (d > 1.1f)
                a = 0;
            else
                a = (Uint8)(alpha_max * (1.0f - (d - 0.9f) * 5.0f));
            if (a > 0)
                pone_punctulum(cx + dx, cy + dy, r, g, b, a);
        }
    }
}

/* pinge ellipsem cum gradiente (lumen a superiore-sinistro) */
static void pinge_ellipsem_umbratam(int cx, int cy, float rx, float ry,
                                    color_t basis, color_t lux, color_t umbra)
{
    int irx = (int)(rx + 1.5f), iry = (int)(ry + 1.5f);
    for (int dy = -iry; dy <= iry; dy++) {
        for (int dx = -irx; dx <= irx; dx++) {
            float ex = (float)dx / rx;
            float ey = (float)dy / ry;
            float d = sqrtf(ex * ex + ey * ey);
            if (d > 1.1f) continue;
            Uint8 a = (d < 0.9f) ? 255 :
                      (Uint8)(255.0f * (1.0f - (d - 0.9f) * 5.0f));
            if (a == 0) continue;

            /* gradiens luminosus: (-0.5, -0.7) est directio luminis */
            float lum = 0.5f - 0.35f * ex - 0.45f * ey;
            if (lum < 0) lum = 0;
            if (lum > 1) lum = 1;

            Uint8 r, g, b;
            if (lum > 0.5f) {
                float t = (lum - 0.5f) * 2.0f;
                r = (Uint8)(basis.r + t * (lux.r - basis.r));
                g = (Uint8)(basis.g + t * (lux.g - basis.g));
                b = (Uint8)(basis.b + t * (lux.b - basis.b));
            } else {
                float t = lum * 2.0f;
                r = (Uint8)(umbra.r + t * (basis.r - umbra.r));
                g = (Uint8)(umbra.g + t * (basis.g - umbra.g));
                b = (Uint8)(umbra.b + t * (basis.b - umbra.b));
            }
            pone_punctulum(cx + dx, cy + dy, r, g, b, a);
        }
    }
}

/* pinge triangulum plenum */
static void pinge_triangulum(int x0, int y0, int x1, int y1, int x2, int y2,
                             Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int min_x = x0, max_x = x0, min_y = y0, max_y = y0;
    if (x1 < min_x) min_x = x1; if (x1 > max_x) max_x = x1;
    if (x2 < min_x) min_x = x2; if (x2 > max_x) max_x = x2;
    if (y1 < min_y) min_y = y1; if (y1 > max_y) max_y = y1;
    if (y2 < min_y) min_y = y2; if (y2 > max_y) max_y = y2;

    for (int py = min_y; py <= max_y; py++) {
        for (int px = min_x; px <= max_x; px++) {
            /* barycentricae coordinatae */
            float d = (float)((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));
            if (fabsf(d) < 0.001f) continue;
            float la = ((float)((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2))) / d;
            float lb = ((float)((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2))) / d;
            float lc = 1.0f - la - lb;
            if (la >= -0.01f && lb >= -0.01f && lc >= -0.01f)
                pone_punctulum(px, py, r, g, b, a);
        }
    }
}

/* pinge lineam (Bresenham) */
static void pinge_lineam(int x0, int y0, int x1, int y1,
                         Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pone_punctulum(x0, y0, r, g, b, a);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* scala: converti coordinatas spritae (0..32) ad punctula cellulae */
#define S(v) ((int)((v) * mag_cellulae / 32.0f))
#define SF(v) ((float)(v) * mag_cellulae / 32.0f)

/* --- imagines singulorum generum --- */

/* herba / vacuum — terra viridis cum gramine */
static void pinge_herbam(int px, int py)
{
    int m = mag_cellulae;
    /* basis terrae */
    for (int dy = 0; dy < m; dy++) {
        for (int dx = 0; dx < m; dx++) {
            float n = strepitus(px + dx, py + dy, 0);
            float n2 = strepitus(px + dx, py + dy, 42);
            Uint8 r = (Uint8)(28 + n * 14);
            Uint8 g = (Uint8)(45 + n * 18 + n2 * 8);
            Uint8 b = (Uint8)(22 + n * 10);
            pone_punctulum(px + dx, py + dy, r, g, b, 255);
        }
    }
    /* herbae culmi — punctula lucidiora */
    for (int i = 0; i < m * m / 12; i++) {
        int gx = (int)(strepitus(px + i, py, 7) * (float)m);
        int gy = (int)(strepitus(px, py + i, 11) * (float)m);
        float br = strepitus(px + gx, py + gy, 19);
        Uint8 r = (Uint8)(40 + br * 30);
        Uint8 g = (Uint8)(70 + br * 50);
        Uint8 b = (Uint8)(25 + br * 15);
        pone_punctulum(px + gx, py + gy, r, g, b, 200);
        /* culmus sursum */
        if (br > 0.5f)
            pone_punctulum(px + gx, py + gy - 1,
                           (Uint8)(50 + br * 40), (Uint8)(90 + br * 50), 30, 160);
    }
}

/* saxum — lapis irregularis cum fissuris */
static void pinge_saxum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;
    float rx = SF(11), ry = SF(10);

    /* umbra */
    pinge_ellipsem(cx + S(2), cy + S(2), rx, ry, 10, 15, 8, 100);

    /* corpus lapidis cum textura */
    int irx = (int)(rx + 2), iry = (int)(ry + 2);
    for (int dy = -iry; dy <= iry; dy++) {
        for (int dx = -irx; dx <= irx; dx++) {
            /* forma irregularis: ellipsis perturbata strepitu */
            float ex = (float)dx / rx;
            float ey = (float)dy / ry;
            float n = strepitus(cx + dx, cy + dy, 3) * 0.25f;
            float d = sqrtf(ex * ex + ey * ey) + n - 0.12f;
            if (d > 1.05f) continue;
            Uint8 a = (d < 0.9f) ? 255 : (Uint8)(255.0f * fmaxf(0, 1.0f - (d - 0.9f) * 6.67f));
            if (a == 0) continue;

            float lum = 0.55f - 0.3f * ex - 0.35f * ey;
            lum += strepitus(cx + dx, cy + dy, 5) * 0.15f;
            if (lum < 0) lum = 0; if (lum > 1) lum = 1;
            Uint8 r = (Uint8)(80 + lum * 90);
            Uint8 g = (Uint8)(78 + lum * 82);
            Uint8 b = (Uint8)(70 + lum * 70);
            pone_punctulum(cx + dx, cy + dy, r, g, b, a);
        }
    }

    /* fissurae */
    pinge_lineam(cx - S(3), cy - S(2), cx + S(1), cy + S(4), 50, 48, 42, 140);
    pinge_lineam(cx + S(1), cy - S(4), cx + S(3), cy + S(1), 55, 52, 46, 120);
}

/* murus — lateritius cum cemento */
static void pinge_murum(int px, int py)
{
    int m = mag_cellulae;
    int alt_lateris = m / 4;
    if (alt_lateris < 2) alt_lateris = 2;
    int lat_lateris = m / 2;
    if (lat_lateris < 4) lat_lateris = 4;

    /* cementum (fundum) */
    for (int dy = 0; dy < m; dy++)
        for (int dx = 0; dx < m; dx++) {
            float n = strepitus(px + dx, py + dy, 20);
            Uint8 g = (Uint8)(58 + n * 12);
            pone_punctulum(px + dx, py + dy, g, (Uint8)(g - 5), (Uint8)(g - 10), 255);
        }

    /* lateres */
    for (int ry = 0; ry < 4; ry++) {
        int yy = ry * alt_lateris;
        int x_offset = (ry % 2) ? lat_lateris / 2 : 0;
        for (int rx = -1; rx < 3; rx++) {
            int xx = rx * lat_lateris + x_offset;
            /* color lateris cum variatione */
            float n = strepitus(px + xx, py + yy, 25);
            Uint8 br = (Uint8)(140 + n * 40);
            Uint8 bg = (Uint8)(65 + n * 25);
            Uint8 bb = (Uint8)(50 + n * 20);

            for (int dy = 1; dy < alt_lateris - 1; dy++) {
                for (int dx = 1; dx < lat_lateris - 1; dx++) {
                    int fx = xx + dx, fy = yy + dy;
                    if (fx < 0 || fx >= m || fy < 0 || fy >= m) continue;
                    /* gradiens per laterem */
                    float t = (float)dy / (float)alt_lateris;
                    float lum = 1.0f - t * 0.3f;
                    /* margo superiore lucidior */
                    if (dy == 1) lum += 0.15f;
                    float nn = strepitus(px + fx, py + fy, 27) * 0.08f;
                    lum += nn;
                    if (lum > 1.15f) lum = 1.15f;
                    pone_punctulum(px + fx, py + fy,
                                   (Uint8)(br * lum > 255 ? 255 : br * lum),
                                   (Uint8)(bg * lum > 255 ? 255 : bg * lum),
                                   (Uint8)(bb * lum > 255 ? 255 : bb * lum), 255);
                }
            }
        }
    }
}

/* feles — feles aurantia cum auribus, oculis, vibrissisque */
static void pinge_felem(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;
    color_t corpus  = { 230, 155, 45 };
    color_t lux     = { 255, 210, 130 };
    color_t umbra_c = { 160, 95,  20 };

    /* cauda (arcus ad dextram) */
    for (float t = 0; t < 1.0f; t += 0.02f) {
        float tx = cx + SF(4) + t * SF(8);
        float ty = cy + SF(4) - sinf(t * 3.14159f) * SF(6);
        pinge_ellipsem((int)tx, (int)ty, SF(1.2f), SF(1.2f),
                       corpus.r, corpus.g, corpus.b, 220);
    }

    /* umbra */
    pinge_ellipsem(cx + S(2), cy + S(3), SF(10), SF(8), 10, 15, 8, 80);

    /* corpus */
    pinge_ellipsem_umbratam(cx, cy + S(3), SF(9), SF(7),
                            corpus, lux, umbra_c);

    /* caput */
    pinge_ellipsem_umbratam(cx, cy - S(4), SF(8), SF(7),
                            corpus, lux, umbra_c);

    /* aures — trianguli */
    /* auris sinistra */
    pinge_triangulum(cx - S(7), cy - S(5), cx - S(5), cy - S(13), cx - S(2), cy - S(7),
                     umbra_c.r, umbra_c.g, umbra_c.b, 255);
    pinge_triangulum(cx - S(6), cy - S(6), cx - S(5), cy - S(11), cx - S(3), cy - S(7),
                     220, 140, 160, 255);

    /* auris dextra */
    pinge_triangulum(cx + S(7), cy - S(5), cx + S(5), cy - S(13), cx + S(2), cy - S(7),
                     umbra_c.r, umbra_c.g, umbra_c.b, 255);
    pinge_triangulum(cx + S(6), cy - S(6), cx + S(5), cy - S(11), cx + S(3), cy - S(7),
                     220, 140, 160, 255);

    /* oculi */
    /* sclera */
    pinge_ellipsem(cx - S(3), cy - S(5), SF(2.5f), SF(2.0f), 255, 255, 240, 255);
    pinge_ellipsem(cx + S(3), cy - S(5), SF(2.5f), SF(2.0f), 255, 255, 240, 255);
    /* iris */
    pinge_ellipsem(cx - S(3), cy - S(5), SF(1.5f), SF(1.5f), 50, 180, 50, 255);
    pinge_ellipsem(cx + S(3), cy - S(5), SF(1.5f), SF(1.5f), 50, 180, 50, 255);
    /* pupilla */
    pinge_ellipsem(cx - S(3), cy - S(5), SF(0.8f), SF(1.2f), 10, 10, 10, 255);
    pinge_ellipsem(cx + S(3), cy - S(5), SF(0.8f), SF(1.2f), 10, 10, 10, 255);
    /* punctum luminis in oculo */
    pone_punctulum(cx - S(2), cy - S(6), 255, 255, 255, 240);
    pone_punctulum(cx + S(4), cy - S(6), 255, 255, 255, 240);

    /* nasus */
    pinge_triangulum(cx - S(1), cy - S(2), cx + S(1), cy - S(2), cx, cy - S(1),
                     255, 140, 160, 255);

    /* os */
    pinge_lineam(cx, cy - S(1), cx - S(1), cy, 120, 70, 50, 160);
    pinge_lineam(cx, cy - S(1), cx + S(1), cy, 120, 70, 50, 160);

    /* vibrissae */
    pinge_lineam(cx - S(3), cy - S(2), cx - S(8), cy - S(3), 200, 200, 200, 140);
    pinge_lineam(cx - S(3), cy - S(1), cx - S(8), cy - S(1), 200, 200, 200, 140);
    pinge_lineam(cx + S(3), cy - S(2), cx + S(8), cy - S(3), 200, 200, 200, 140);
    pinge_lineam(cx + S(3), cy - S(1), cx + S(8), cy - S(1), 200, 200, 200, 140);
}

/* dalekus — autocinetum metallicum cum oculo et sphaerolis */
static void pinge_dalekum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;

    /* umbra */
    pinge_ellipsem(cx + S(2), cy + S(3), SF(10), SF(5), 10, 15, 8, 80);

    /* corpus inferius (trapezoideum) — amplior infra */
    for (int dy = S(2); dy < S(14); dy++) {
        float t = (float)dy / SF(14);
        float lat = SF(7) + t * SF(4);
        float lum = 0.7f - t * 0.3f;
        for (int dx = -(int)lat; dx <= (int)lat; dx++) {
            float edge = fabsf((float)dx) / lat;
            float l = lum + (1.0f - edge) * 0.15f;
            Uint8 r = (Uint8)(140 * l + 40);
            Uint8 g = (Uint8)(130 * l + 35);
            Uint8 b = (Uint8)(145 * l + 50);
            pone_punctulum(cx + dx, cy + dy, r, g, b, 255);
        }
    }

    /* cingulum sphaerolarum */
    int num_sph = 5;
    int sph_y = cy + S(6);
    for (int i = 0; i < num_sph; i++) {
        int sx = cx - S(7) + (i * S(14)) / (num_sph - 1);
        pinge_ellipsem(sx, sph_y, SF(1.8f), SF(1.8f), 80, 80, 90, 255);
        pinge_ellipsem(sx - S(1), sph_y - S(1), SF(0.7f), SF(0.7f), 160, 160, 180, 200);
    }

    /* caput (hemisphaera) */
    color_t cap_basis = { 100, 100, 120 };
    color_t cap_lux   = { 180, 185, 200 };
    color_t cap_umbra = {  50,  50,  65 };
    pinge_ellipsem_umbratam(cx, cy - S(3), SF(8), SF(8), cap_basis, cap_lux, cap_umbra);

    /* trunculus oculi */
    pinge_lineam(cx + S(5), cy - S(5), cx + S(9), cy - S(9), 120, 120, 140, 255);
    pinge_lineam(cx + S(5), cy - S(4), cx + S(9), cy - S(8), 90, 90, 110, 255);

    /* oculus (lucerna rubra) */
    float pulsus = 0.6f + 0.4f * sinf((float)numerus_picturae * 0.15f);
    Uint8 glow = (Uint8)(180 + 75 * pulsus);
    pinge_ellipsem(cx + S(9), cy - S(9), SF(2.5f), SF(2.5f), glow, 30, 20, 255);
    pinge_ellipsem(cx + S(9), cy - S(9), SF(1.2f), SF(1.2f), 255, 200, 180, 200);
    /* fulgur circa oculum */
    pinge_ellipsem(cx + S(9), cy - S(9), SF(4), SF(4),
                   255, 40, 20, (Uint8)(40 * pulsus));

    /* bracchium armatum (sinistrum) */
    pinge_lineam(cx - S(7), cy + S(1), cx - S(12), cy + S(4), 110, 110, 130, 255);
    pinge_lineam(cx - S(7), cy + S(2), cx - S(12), cy + S(5), 80, 80, 100, 255);
    /* manus */
    pinge_ellipsem(cx - S(12), cy + S(4), SF(1.5f), SF(1.2f), 140, 140, 160, 255);
}

/* ursus — ursus brunneus grandis */
static void pinge_ursum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;
    color_t corpus  = { 150, 95, 40 };
    color_t lux     = { 200, 155, 100 };
    color_t umbra_c = { 90,  55,  20 };

    /* umbra */
    pinge_ellipsem(cx + S(2), cy + S(3), SF(11), SF(9), 10, 15, 8, 80);

    /* corpus — grandior quam feles */
    pinge_ellipsem_umbratam(cx, cy + S(2), SF(11), SF(10),
                            corpus, lux, umbra_c);

    /* caput */
    pinge_ellipsem_umbratam(cx, cy - S(5), SF(9), SF(8),
                            corpus, lux, umbra_c);

    /* aures rotundae */
    pinge_ellipsem_umbratam(cx - S(7), cy - S(10), SF(3.5f), SF(3.5f),
                            corpus, lux, umbra_c);
    pinge_ellipsem(cx - S(7), cy - S(10), SF(2.0f), SF(2.0f), 180, 130, 100, 200);
    pinge_ellipsem_umbratam(cx + S(7), cy - S(10), SF(3.5f), SF(3.5f),
                            corpus, lux, umbra_c);
    pinge_ellipsem(cx + S(7), cy - S(10), SF(2.0f), SF(2.0f), 180, 130, 100, 200);

    /* rostrum (muzzle) — ovale lucidius */
    pinge_ellipsem(cx, cy - S(3), SF(5), SF(3.5f), 190, 155, 120, 255);

    /* oculi */
    pinge_ellipsem(cx - S(4), cy - S(7), SF(2.0f), SF(2.0f), 20, 15, 10, 255);
    pinge_ellipsem(cx + S(4), cy - S(7), SF(2.0f), SF(2.0f), 20, 15, 10, 255);
    /* lumen in oculis */
    pone_punctulum(cx - S(3), cy - S(8), 255, 255, 255, 200);
    pone_punctulum(cx + S(5), cy - S(8), 255, 255, 255, 200);

    /* nasus */
    pinge_ellipsem(cx, cy - S(3), SF(2.0f), SF(1.5f), 30, 20, 15, 255);
    pone_punctulum(cx - S(1), cy - S(4), 80, 70, 60, 160);
}

/* corvus — avis nigra cum rostro aurantiaco */
static void pinge_corvum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;
    color_t corpus  = { 40, 42, 55 };
    color_t lux     = { 80, 85, 105 };
    color_t umbra_c = { 15, 15, 25 };

    /* umbra */
    pinge_ellipsem(cx + S(2), cy + S(4), SF(9), SF(5), 10, 15, 8, 70);

    /* cauda */
    pinge_triangulum(cx + S(5), cy + S(2), cx + S(12), cy + S(6), cx + S(5), cy + S(6),
                     corpus.r, corpus.g, corpus.b, 255);

    /* ala (posterior) */
    pinge_ellipsem_umbratam(cx + S(1), cy + S(1), SF(10), SF(7),
                            (color_t){30, 32, 42}, (color_t){55, 58, 72}, (color_t){12, 12, 20});

    /* corpus */
    pinge_ellipsem_umbratam(cx - S(1), cy, SF(9), SF(7),
                            corpus, lux, umbra_c);

    /* caput */
    pinge_ellipsem_umbratam(cx - S(5), cy - S(5), SF(6), SF(5.5f),
                            corpus, lux, umbra_c);

    /* oculus */
    pinge_ellipsem(cx - S(7), cy - S(6), SF(1.8f), SF(1.8f), 255, 220, 50, 255);
    pinge_ellipsem(cx - S(7), cy - S(6), SF(0.9f), SF(0.9f), 10, 10, 10, 255);

    /* rostrum */
    pinge_triangulum(cx - S(10), cy - S(5), cx - S(14), cy - S(4),
                     cx - S(10), cy - S(3),
                     220, 160, 40, 255);
    /* rostrum superius lucidior */
    pinge_lineam(cx - S(10), cy - S(5), cx - S(14), cy - S(4), 250, 200, 80, 180);

    /* pedes */
    pinge_lineam(cx - S(2), cy + S(6), cx - S(3), cy + S(10), 180, 140, 40, 200);
    pinge_lineam(cx + S(2), cy + S(6), cx + S(1), cy + S(10), 180, 140, 40, 200);
    /* digiti */
    pinge_lineam(cx - S(3), cy + S(10), cx - S(5), cy + S(11), 180, 140, 40, 180);
    pinge_lineam(cx - S(3), cy + S(10), cx - S(1), cy + S(11), 180, 140, 40, 180);
    pinge_lineam(cx + S(1), cy + S(10), cx - S(1), cy + S(11), 180, 140, 40, 180);
    pinge_lineam(cx + S(1), cy + S(10), cx + S(3), cy + S(11), 180, 140, 40, 180);
}

/* rapum — rapum aurantiacum cum foliis viridibus */
static void pinge_rapum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;

    /* folia (virides trianguli supra) */
    pinge_triangulum(cx, cy - S(8), cx - S(4), cy - S(14), cx + S(1), cy - S(6),
                     40, 160, 40, 255);
    pinge_triangulum(cx, cy - S(8), cx + S(4), cy - S(14), cx - S(1), cy - S(6),
                     50, 180, 50, 255);
    pinge_triangulum(cx, cy - S(7), cx, cy - S(13), cx + S(2), cy - S(8),
                     60, 200, 60, 240);

    /* corpus rapi — conus inversus cum gradiente */
    for (int dy = -S(7); dy <= S(10); dy++) {
        float t = (float)(dy + S(7)) / (float)(S(17));
        float lat = SF(6) * (1.0f - t * 0.85f);
        if (lat < 0.5f) lat = 0.5f;
        for (int dx = -(int)lat; dx <= (int)lat; dx++) {
            float edge = fabsf((float)dx) / fmaxf(lat, 1.0f);
            float lum = 0.8f - edge * 0.4f + (1.0f - t) * 0.2f;
            /* lineis horizontalibus */
            float stria = sinf(t * 25.0f) * 0.05f;
            lum += stria;
            Uint8 r = (Uint8)(255 * lum > 255 ? 255 : 255 * lum);
            Uint8 g = (Uint8)(120 * lum);
            Uint8 b = (Uint8)(20 * lum);
            pone_punctulum(cx + dx, cy + dy, r, g, b, 255);
        }
    }

    /* apex (basis rapi apud folia) */
    pinge_ellipsem(cx, cy - S(7), SF(6), SF(2), 255, 140, 30, 255);
}

/* fungus — fungus cum pileo et stipite */
static void pinge_fungum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;

    /* stipes */
    color_t stip = { 230, 220, 200 };
    for (int dy = S(0); dy <= S(10); dy++) {
        float lat = SF(3) + (float)dy / SF(10) * SF(1.5f);
        for (int dx = -(int)lat; dx <= (int)lat; dx++) {
            float edge = fabsf((float)dx) / fmaxf(lat, 1.0f);
            float lum = 1.0f - edge * 0.3f;
            pone_punctulum(cx + dx, cy + dy,
                           (Uint8)(stip.r * lum), (Uint8)(stip.g * lum),
                           (Uint8)(stip.b * lum), 255);
        }
    }

    /* pileus (hemisphaera) */
    color_t pileus   = { 180, 50, 60 };
    color_t pil_lux  = { 230, 120, 130 };
    color_t pil_umbr = { 100, 20, 30 };
    float prx = SF(11), pry = SF(8);
    int pcy = cy - S(4);

    /* solum pars superior hemisphaerae */
    int irx = (int)(prx + 2), iry = (int)(pry + 2);
    for (int dy = -iry; dy <= 0; dy++) {
        for (int dx = -irx; dx <= irx; dx++) {
            float ex = (float)dx / prx;
            float ey = (float)dy / pry;
            float d = sqrtf(ex * ex + ey * ey);
            if (d > 1.1f) continue;
            Uint8 a = (d < 0.9f) ? 255 : (Uint8)(255.0f * fmaxf(0, 1.0f - (d - 0.9f) * 5.0f));
            if (a == 0) continue;
            float lum = 0.5f - 0.35f * ex - 0.45f * ey;
            if (lum < 0) lum = 0; if (lum > 1) lum = 1;
            Uint8 r, g, b;
            if (lum > 0.5f) {
                float t = (lum - 0.5f) * 2.0f;
                r = (Uint8)(pileus.r + t * (pil_lux.r - pileus.r));
                g = (Uint8)(pileus.g + t * (pil_lux.g - pileus.g));
                b = (Uint8)(pileus.b + t * (pil_lux.b - pileus.b));
            } else {
                float t = lum * 2.0f;
                r = (Uint8)(pil_umbr.r + t * (pileus.r - pil_umbr.r));
                g = (Uint8)(pil_umbr.g + t * (pileus.g - pil_umbr.g));
                b = (Uint8)(pil_umbr.b + t * (pileus.b - pil_umbr.b));
            }
            pone_punctulum(cx + dx, pcy + dy, r, g, b, a);
        }
    }
    /* margo inferior pilei */
    pinge_ellipsem(cx, pcy, prx, SF(1.5f), 160, 140, 130, 200);

    /* maculae albae in pileo */
    pinge_ellipsem(cx - S(4), pcy - S(3), SF(2.0f), SF(1.8f), 255, 255, 240, 200);
    pinge_ellipsem(cx + S(3), pcy - S(5), SF(1.5f), SF(1.3f), 255, 255, 240, 180);
    pinge_ellipsem(cx + S(1), pcy - S(2), SF(1.2f), SF(1.0f), 255, 255, 240, 160);
}

/* zodus — heros fulgens cum fulmine */
static void pinge_zodum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;

    /* fulgur circumfusus (glow) */
    float pulsus = 0.5f + 0.5f * sinf((float)numerus_picturae * 0.1f);
    for (int dy = -S(15); dy <= S(15); dy++) {
        for (int dx = -S(15); dx <= S(15); dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy)) / SF(14);
            if (d > 1.0f) continue;
            Uint8 a = (Uint8)((1.0f - d) * (25 + 20 * pulsus));
            pone_punctulum(cx + dx, cy + dy, 255, 240, 80, a);
        }
    }

    /* corpus aureus */
    color_t corpus  = { 255, 220, 50 };
    color_t lux     = { 255, 255, 200 };
    color_t umbra_c = { 200, 150, 10 };

    /* umbra */
    pinge_ellipsem(cx + S(1), cy + S(3), SF(8), SF(6), 80, 80, 20, 60);

    /* corpus */
    pinge_ellipsem_umbratam(cx, cy + S(2), SF(8), SF(8),
                            corpus, lux, umbra_c);

    /* caput */
    pinge_ellipsem_umbratam(cx, cy - S(5), SF(7), SF(6),
                            corpus, lux, umbra_c);

    /* oculi (determinati, heroici) */
    pinge_ellipsem(cx - S(3), cy - S(6), SF(2.2f), SF(1.5f), 255, 255, 255, 255);
    pinge_ellipsem(cx + S(3), cy - S(6), SF(2.2f), SF(1.5f), 255, 255, 255, 255);
    /* iris electrica */
    pinge_ellipsem(cx - S(3), cy - S(6), SF(1.3f), SF(1.3f), 50, 100, 255, 255);
    pinge_ellipsem(cx + S(3), cy - S(6), SF(1.3f), SF(1.3f), 50, 100, 255, 255);
    pinge_ellipsem(cx - S(3), cy - S(6), SF(0.6f), SF(0.6f), 10, 10, 30, 255);
    pinge_ellipsem(cx + S(3), cy - S(6), SF(0.6f), SF(0.6f), 10, 10, 30, 255);
    pone_punctulum(cx - S(2), cy - S(7), 255, 255, 255, 240);
    pone_punctulum(cx + S(4), cy - S(7), 255, 255, 255, 240);

    /* fulmen in corpore */
    pinge_lineam(cx - S(1), cy - S(1), cx + S(2), cy + S(2), 255, 255, 200, 255);
    pinge_lineam(cx + S(2), cy + S(2), cx - S(2), cy + S(3), 255, 255, 200, 255);
    pinge_lineam(cx - S(2), cy + S(3), cx + S(1), cy + S(6), 255, 255, 200, 255);

    /* aureola pulsans */
    float rad_aur = SF(10) + pulsus * SF(2);
    int ir = (int)(rad_aur + 3);
    for (int dy = -ir; dy <= ir; dy++) {
        for (int dx = -ir; dx <= ir; dx++) {
            float dist = sqrtf((float)(dx * dx + dy * dy));
            float diff = fabsf(dist - rad_aur);
            if (diff < 1.5f) {
                Uint8 a = (Uint8)((1.0f - diff / 1.5f) * (50 + 30 * pulsus));
                pone_punctulum(cx + dx, cy + dy, 255, 255, 150, a);
            }
        }
    }
}

/* oculus — oculus omnividens */
static void pinge_oculum(int px, int py)
{
    pinge_herbam(px, py);
    int cx = px + mag_cellulae / 2;
    int cy = py + mag_cellulae / 2;

    /* fulgur caeruleus */
    float pulsus = 0.5f + 0.5f * sinf((float)numerus_picturae * 0.07f);
    for (int dy = -S(14); dy <= S(14); dy++) {
        for (int dx = -S(14); dx <= S(14); dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy)) / SF(13);
            if (d > 1.0f) continue;
            Uint8 a = (Uint8)((1.0f - d) * (15 + 15 * pulsus));
            pone_punctulum(cx + dx, cy + dy, 80, 180, 255, a);
        }
    }

    /* sclera — forma oculi (ellipsis cum acuminibus) */
    for (int dy = -S(6); dy <= S(6); dy++) {
        for (int dx = -S(12); dx <= S(12); dx++) {
            float ex = (float)dx / SF(12);
            float ey = (float)dy / SF(6);
            /* forma mandorlae */
            float d = sqrtf(ex * ex + ey * ey * (1.0f + fabsf(ex) * 0.5f));
            if (d > 1.05f) continue;
            Uint8 a = (d < 0.9f) ? 255 : (Uint8)(255.0f * fmaxf(0, 1.0f - (d - 0.9f) * 6.67f));
            if (a == 0) continue;
            /* venae subtiles */
            float vn = strepitus(cx + dx, cy + dy, 33) * 0.15f;
            Uint8 r = (Uint8)(240 + vn * 15);
            Uint8 g = (Uint8)(235 - vn * 30);
            Uint8 b = (Uint8)(230 - vn * 20);
            pone_punctulum(cx + dx, cy + dy, r, g, b, a);
        }
    }

    /* iris */
    color_t iris = { 40, 140, 220 };
    float ir_rad = SF(5);
    int iir = (int)(ir_rad + 2);
    for (int dy = -iir; dy <= iir; dy++) {
        for (int dx = -iir; dx <= iir; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            if (d > ir_rad + 1.0f) continue;
            Uint8 a = (d < ir_rad - 0.5f) ? 255 :
                      (Uint8)(255.0f * fmaxf(0, 1.0f - (d - ir_rad + 0.5f)));
            if (a == 0) continue;
            /* radiationes in iride */
            float ang = atan2f((float)dy, (float)dx);
            float rad_var = sinf(ang * 8.0f) * 0.15f + 0.85f;
            float lum = rad_var * (1.0f - d / ir_rad * 0.3f);
            Uint8 r = (Uint8)(iris.r * lum);
            Uint8 g = (Uint8)(iris.g * lum);
            Uint8 b = (Uint8)(iris.b * lum);
            pone_punctulum(cx + dx, cy + dy, r, g, b, a);
        }
    }

    /* pupilla */
    pinge_ellipsem(cx, cy, SF(2.5f), SF(2.5f), 5, 5, 10, 255);

    /* punctum luminis */
    pinge_ellipsem(cx - S(2), cy - S(2), SF(1.5f), SF(1.5f), 255, 255, 255, 230);
    pone_punctulum(cx + S(2), cy + S(1), 255, 255, 255, 140);
}

/* --- pictura principalis --- */

static void pinge_cellulam(const tabula_t *tab, int x, int y)
{
    const cella_t *c = tabula_da_const(tab, x, y);
    genus_t g = c->genus;
    int px = x * mag_cellulae;
    int py = y * mag_cellulae;

    switch (g) {
    case VACUUM:  pinge_herbam(px, py);    break;
    case SAXUM:   pinge_saxum(px, py);     break;
    case MURUS:   pinge_murum(px, py);     break;
    case FELES:   pinge_felem(px, py);     break;
    case DALEKUS: pinge_dalekum(px, py);   break;
    case URSUS:   pinge_ursum(px, py);     break;
    case CORVUS:  pinge_corvum(px, py);    break;
    case RAPUM:   pinge_rapum(px, py);     break;
    case FUNGUS:  pinge_fungum(px, py);    break;
    case ZODUS:   pinge_zodum(px, py);     break;
    case OCULUS:  pinge_oculum(px, py);    break;
    default:      pinge_herbam(px, py);    break;
    }
}

/* --- initiatio --- */

int fenestra_initia(int latus_cellulae)
{
    if (latus_cellulae > 0)
        mag_cellulae = latus_cellulae;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init defecit: %s\n", SDL_GetError());
        return -1;
    }

    fenestra = SDL_CreateWindow(
        "munda",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 640,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!fenestra) {
        fprintf(stderr, "fenestra creari non potuit: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    pictor = SDL_CreateRenderer(fenestra, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pictor) {
        fprintf(stderr, "pictor creari non potuit: %s\n", SDL_GetError());
        SDL_DestroyWindow(fenestra);
        SDL_Quit();
        return -1;
    }

    clausa_est = 0;
    numerus_picturae = 0;
    return 0;
}

/* --- pictura tabulae --- */

void fenestra_pinge(const tabula_t *tab)
{
    if (!pictor || clausa_est)
        return;

    int latus = tab->latus;
    int lat_novum = latus * mag_cellulae;

    if (lat_novum != lat_img) {
        if (textura) SDL_DestroyTexture(textura);
        free(punctula);

        lat_img = lat_novum;
        punctula = malloc((size_t)lat_img * (size_t)lat_img * sizeof(Uint32));
        if (!punctula) return;

        textura = SDL_CreateTexture(pictor,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            lat_img, lat_img);
        if (!textura) return;

        SDL_SetWindowSize(fenestra, lat_img, lat_img);
    }

    memset(punctula, 0, (size_t)lat_img * (size_t)lat_img * sizeof(Uint32));

    for (int y = 0; y < latus; y++)
        for (int x = 0; x < latus; x++)
            pinge_cellulam(tab, x, y);

    SDL_UpdateTexture(textura, NULL, punctula, lat_img * (int)sizeof(Uint32));
    SDL_RenderCopy(pictor, textura, NULL, NULL);
    SDL_RenderPresent(pictor);

    numerus_picturae++;
}

/* --- eventus --- */

int fenestra_lege(void)
{
    SDL_Event eventus;
    while (SDL_PollEvent(&eventus)) {
        if (eventus.type == SDL_QUIT) {
            clausa_est = 1;
            return 'q';
        }
        if (eventus.type == SDL_KEYDOWN) {
            switch (eventus.key.keysym.sym) {
            case SDLK_q: case SDLK_ESCAPE: return 'q';
            case SDLK_UP:    return 'A';
            case SDLK_DOWN:  return 'B';
            case SDLK_RIGHT: return 'C';
            case SDLK_LEFT:  return 'D';
            case SDLK_t:     return 't';
            default: break;
            }
        }
    }
    return -1;
}

/* --- mundatio --- */

void fenestra_fini(void)
{
    free(punctula);
    punctula = NULL;
    if (textura)  { SDL_DestroyTexture(textura);   textura  = NULL; }
    if (pictor)   { SDL_DestroyRenderer(pictor);    pictor   = NULL; }
    if (fenestra) { SDL_DestroyWindow(fenestra);    fenestra = NULL; }
    SDL_Quit();
}

int fenestra_clausa(void)
{
    return clausa_est;
}
