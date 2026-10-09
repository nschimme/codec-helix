/* ***** BEGIN LICENSE BLOCK *****
 * Fixed-point HE-AAC v2 Parametric Stereo Decoder for Helix
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
