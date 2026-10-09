/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ps.h,v 1.0 2023/10/01 00:00:00 $
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 *
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _PS_H
#define _PS_H

#include "aaccommon.h"
#include "bitstream.h"
#include "../utils/helix_pgm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PS_ENVELOPES     4
#define PS_NUM_SUBBANDS_20   20
#define PS_NUM_SUBBANDS_34   34
#define PS_MAX_NUM_SUBBANDS  34

#define EXT_PS               2

#define MAX_HYBRID_BANDS     32
#define PS_SUBBANDS_DECORR   32

/* Fixed point Q30 constants */
#define Q30(x)               ((int)((x) * 1073741824.0 + ((x) >= 0 ? 0.5 : -0.5)))
#define Q15(x)               ((short)((x) * 32768.0 + ((x) >= 0 ? 0.5 : -0.5)))

/* Compact PS Header & Grid parameters (8-bit fields to save ROM and RAM) */
typedef struct _PSHeader {
    unsigned char enable_ps_header;
    unsigned char enable_iid;
    unsigned char iid_mode;
    unsigned char enable_icc;
    unsigned char icc_mode;
    unsigned char enable_ext;
    unsigned char num_env;
    unsigned char border_position[MAX_PS_ENVELOPES + 1];
    unsigned char iid_dt[MAX_PS_ENVELOPES];
    unsigned char icc_dt[MAX_PS_ENVELOPES];
} PSHeader;

/* PS Channel & Frame state */
typedef struct _PSData {
    unsigned char header_read;
    PSHeader hdr;

    /* Quantized and dequantized parameters */
    signed char iid_index[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];
    signed char icc_index[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];

    signed char iid_index_prev[PS_MAX_NUM_SUBBANDS];
    signed char icc_index_prev[PS_MAX_NUM_SUBBANDS];

    /* Hybrid filter delay buffers (complex QMF subbands 0..2) */
    int hybrid_delay[3][12][2]; /* [qmf_band][delay_samples][real/imag] */

    /* Decorrelator delay buffers: 32 subbands, up to 14 delay samples */
    int decorr_delay[32][14][2]; /* [subband][delay][re/im] */

    /* Allpass decorrelator state */
    int allpass_delay[32][3][2]; /* [subband][filter_stage][re/im] */

    /* Interpolated mixing matrices (Q30 format) */
    int h11[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];
    int h12[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];
    int h21[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];
    int h22[MAX_PS_ENVELOPES][PS_MAX_NUM_SUBBANDS];

} PSData;

/* External static tables in PROGMEM */
extern const int iid_scale_tab[15] PROGMEM;
extern const int icc_scale_tab[8] PROGMEM;
extern const int alpha_tab[8] PROGMEM;

/* Function prototypes */
int DecodePSHeader(BitStreamInfo *bsi, PSHeader *hdr);
int DecodePSDataPayload(BitStreamInfo *bsi, PSData *psd);
int DecodePSHuffman(BitStreamInfo *bsi, int type);
void ProcessPSSlot(PSData *psd, int Xbuf_slot[64][2], int slot_L[64][2], int slot_R[64][2], int l);

#ifdef __cplusplus
}
#endif

#endif /* _PS_H */
