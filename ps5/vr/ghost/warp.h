/*
 * PS5XR output warp: the eye picture Hmd2 is handed spends its pixels evenly
 * over angle instead of evenly over tangent, and Hmd2 undoes it with the per-eye
 * 1D lookup tables native PS VR2 titles hand it (libSceHmd2 "Fsr1dLut", the
 * layer's fsrEn; Call of the Mountain fills them every frame).
 *
 * What the tables mean (libSceHmd2 13.60 reprojection shader at 0x9cf00, FSR
 * branch): for each panel pixel Hmd2 works out where the eye's rectilinear
 * picture would be sampled, as 0..1 across the eye's field of view (u, v); it
 * reads horizontal table [u] and vertical table [v] (bilinear, entries at
 * (i + 0.5) / n) and samples the picture at those 0..1 positions instead. A
 * table therefore maps the rectilinear position to the stored one; identity
 * tables give the old picture.
 *
 * A rectilinear 2000-wide eye over PS VR2's 104.9 degrees has 12.5 px/deg in the
 * middle and 55 at the outer edge, where the panel shows far fewer. The same
 * pixels spread evenly over angle give 19.1 px/deg everywhere: 1.52x the detail
 * in the middle (1.44x vertically) for the same buffer, the same memory and the
 * same Hmd2 cost. "strength" blends between the two (0 = rectilinear).
 *
 * One axis: lo, hi are the tangents of the eye's two edges on it (left, right or
 * up, down; both positive). Position 0 is the lo edge.
 */
#ifndef PS5XR_WARP_H
#define PS5XR_WARP_H
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct warp_axis {
    float lo, hi, strength;
    /* Or WiVRn's foveation (a remote headset, remote/backend/gv_foveation.h): nband (odd, up to
     * WARP_BANDS) stored pixel counts, the middle band 1:1 with the rectilinear picture and each
     * band out from it one more rectilinear pixel per stored one. strength is then 1. */
    uint16_t band[7];
    uint8_t nband;
} warp_axis;
#define WARP_BANDS 7

/* Bands: stored position w (0..1) -> rectilinear u (0..1), and du/dw there. */
static inline double warp_band_u(const warp_axis *a, double w, double *du_dw)
{
    const int n = (a->nband - 1) / 2;
    double stored = 0.0, src = 0.0;
    for (int i = 0; i < a->nband; i++) {
        stored += a->band[i];
        src += (double)(abs(n - i) + 1) * a->band[i];
    }
    const double x = w * stored;
    double acc_out = 0.0, acc_src = 0.0;
    for (int i = 0; i < a->nband; i++) {
        const double r = (double)(abs(n - i) + 1);
        if (x <= acc_out + a->band[i] || i + 1 == a->nband) {
            if (du_dw)
                *du_dw = r * stored / src;
            return (acc_src + (x - acc_out) * r) / src;
        }
        acc_out += a->band[i];
        acc_src += r * a->band[i];
    }
    return w;
}

/* Stored position w (0..1) -> rectilinear position u (0..1). The compositor
 * uses this for every output pixel; the shader has the same formula. */
static inline double warp_u_of_w(const warp_axis *a, double w)
{
    if (a->nband)
        return warp_band_u(a, w, NULL);
    const double t0 = atan((double)a->lo), t1 = atan((double)a->hi);
    const double even = (tan(-t0 + w * (t0 + t1)) + a->lo) / ((double)a->lo + a->hi);
    return (1.0 - a->strength) * w + a->strength * even;
}

/* du/dw at w: how many rectilinear positions one stored one spans there. */
static inline double warp_du_dw(const warp_axis *a, double w)
{
    if (a->nband) {
        double d;
        warp_band_u(a, w, &d);
        return d;
    }
    const double t0 = atan((double)a->lo), t1 = atan((double)a->hi), t = tan(-t0 + w * (t0 + t1));
    return (1.0 - a->strength) + a->strength * (t0 + t1) * (1.0 + t * t) / ((double)a->lo + a->hi);
}

/* Rectilinear position u -> stored position w, by bisection (u rises with w). */
static inline double warp_w_of_u(const warp_axis *a, double u)
{
    double lo = 0.0, hi = 1.0;
    for (int k = 0; k < 40; k++) {
        const double mid = 0.5 * (lo + hi);
        if (warp_u_of_w(a, mid) < u)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

/* The table Hmd2 reads: entry i = the stored position of rectilinear position (i + 0.5) / n. */
static inline void warp_table(const warp_axis *a, float *out, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
        out[i] = (float)warp_w_of_u(a, (i + 0.5) / n);
}

/* How much more detail the middle of the stored picture holds than a
 * rectilinear one of the same size: dw/du at the eye's straight-ahead point. */
static inline double warp_centre_gain(const warp_axis *a)
{
    if (a->nband)
        return 1.0 / warp_du_dw(a, warp_w_of_u(a, (double)a->lo / ((double)a->lo + a->hi)));
    const double t0 = atan((double)a->lo), t1 = atan((double)a->hi);
    /* du/dw of the even part at the straight-ahead point (tan = 0): (t0 + t1) / (lo + hi) */
    const double du_dw = (1.0 - a->strength) + a->strength * (t0 + t1) / ((double)a->lo + a->hi);
    return 1.0 / du_dw;
}

/* An eye's two axes from the headset's tangents (psvr2_info.fov: out, in, top,
 * bottom for the left eye, mirrored for the right). */
static inline void warp_eye(const float fov[4], int eye, float strength, warp_axis out[2])
{
    out[0].lo = eye ? fov[1] : fov[0];
    out[0].hi = eye ? fov[0] : fov[1];
    out[1].lo = fov[2];
    out[1].hi = fov[3];
    out[0].strength = out[1].strength = strength;
}
#endif
