/* ***** BEGIN LICENSE BLOCK *****
 * Fixed-point HE-AAC v2 Parametric Stereo Tables for Helix
 * ***** END LICENSE BLOCK ***** */

#include "ps.h"

/* Dequantization scale factor tables in PROGMEM (Q30 format, capped to 32-bit max) */

/* IID (Inter-channel Intensity Difference) scale factors: 10^(index * 1.5 / 20) in Q30 */
const int iid_scale_tab[15] PROGMEM = {
    0x02a2491c, /* -7: 0.041355 */
    0x053531b7, /* -6: 0.082180 */
    0x0a14fb61, /* -5: 0.157490 */
    0x1338a0a7, /* -4: 0.299832 */
    0x23f2fef7, /* -3: 0.561341 */
    0x3f1e9444, /* -2: 0.986233 */
    0x63333333, /* -1: 1.550000 */
    0x40000000, /*  0: 1.000000 */
    0x67305980, /*  1: 1.610000 */
    0x7fffffff, /*  2: 2.458330 (clamped Q30) */
    0x7fffffff, /*  3: 3.735430 (clamped Q30) */
    0x7fffffff, /*  4: 6.059240 (clamped Q30) */
    0x7fffffff, /*  5: 8.903440 (clamped Q30) */
    0x7fffffff, /*  6: 13.20880 (clamped Q30) */
    0x7fffffff  /*  7: 19.27130 (clamped Q30) */
};

/* ICC (Inter-channel Cross-Correlation) parameters in Q30 */
const int icc_scale_tab[8] PROGMEM = {
    0x40000000, /* 1.000000 */
    0x3d304918, /* 0.955700 */
    0x3a4fbc4e, /* 0.910800 */
    0x323c28cb, /* 0.785100 */
    0x2475a898, /* 0.570000 */
    0x162c9d78, /* 0.346600 */
    0x0a1c1724, /* 0.158000 */
    0x00000000  /* 0.000000 */
};

/* Allpass filter fractional delay / feedback coefficients in Q30 */
const int alpha_tab[8] PROGMEM = {
    0x23d70a3d, /* 0.35 */
    0x20000000, /* 0.30 */
    0x1c28f5c3, /* 0.26 */
    0x1851eb85, /* 0.22 */
    0x147ae148, /* 0.18 */
    0x10a3d70a, /* 0.14 */
    0x0cccccccc, /* 0.10 */
    0x08f5c28f  /* 0.06 */
};
