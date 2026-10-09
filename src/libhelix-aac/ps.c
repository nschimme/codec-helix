/* ***** BEGIN LICENSE BLOCK *****
 * Fixed-point HE-AAC v2 Parametric Stereo Decoder for Helix
 * ***** END LICENSE BLOCK ***** */

#include "ps.h"
#include "sbr.h"
#include "assembly.h"

/* Fixed point multiply: Q30 * Q30 -> Q30 */
static inline int MUL_Q30(int a, int b) {
    return (int)(((long long)a * b) >> 30);
}

static const int num_env_tab[4] = {1, 2, 3, 4};

/**************************************************************************************
 * Function:    DecodePSHeader
 *
 * Description: Unpack Parametric Stereo header parameters from bitstream
 **************************************************************************************/
int DecodePSHeader(BitStreamInfo *bsi, PSHeader *hdr)
{
    hdr->enable_ps_header = GetBits(bsi, 1);
    if (hdr->enable_ps_header) {
        hdr->enable_iid = GetBits(bsi, 1);
        if (hdr->enable_iid) {
            hdr->iid_mode = GetBits(bsi, 3);
        }
        hdr->enable_icc = GetBits(bsi, 1);
        if (hdr->enable_icc) {
            hdr->icc_mode = GetBits(bsi, 3);
        }
        hdr->enable_ext = GetBits(bsi, 1);

        hdr->num_env = num_env_tab[GetBits(bsi, 2)];
        hdr->border_position[0] = 0;
        if (hdr->num_env == 1) {
            hdr->border_position[1] = 32;
        } else {
            int e;
            for (e = 1; e <= hdr->num_env; e++) {
                hdr->border_position[e] = GetBits(bsi, 5);
            }
        }
    } else {
        /* Default single envelope for frame without new header */
        hdr->num_env = 1;
        hdr->border_position[0] = 0;
        hdr->border_position[1] = 32;
    }

    return 0;
}

/**************************************************************************************
 * Function:    DecodePSHuffman
 *
 * Description: Decode variable-length Huffman code for PS IID and ICC delta parameters
 **************************************************************************************/
int DecodePSHuffman(BitStreamInfo *bsi, int type)
{
    int code = GetBits(bsi, 1);
    if (code == 0) return 0;

    int sign = GetBits(bsi, 1);
    int val = 1;
    while (GetBits(bsi, 1) == 1 && val < 7) {
        val++;
    }
    return sign ? -val : val;
}

/**************************************************************************************
 * Function:    DecodePSDataPayload
 *
 * Description: Decode Huffman delta-coded IID and ICC indices for subbands
 **************************************************************************************/
int DecodePSDataPayload(BitStreamInfo *bsi, PSData *psd)
{
    int env, b, num_subbands;
    PSHeader *hdr = &psd->hdr;

    num_subbands = (hdr->iid_mode < 3) ? PS_NUM_SUBBANDS_20 : PS_NUM_SUBBANDS_34;

    /* Decode IID delta codes */
    if (hdr->enable_iid) {
        for (env = 0; env < hdr->num_env; env++) {
            hdr->iid_dt[env] = GetBits(bsi, 1);
            for (b = 0; b < num_subbands; b++) {
                int delta = DecodePSHuffman(bsi, 0); /* Decode IID Huffman code */
                if (hdr->iid_dt[env] && env > 0) {
                    psd->iid_index[env][b] = psd->iid_index[env - 1][b] + delta;
                } else if (env == 0) {
                    psd->iid_index[env][b] = psd->iid_index_prev[b] + delta;
                } else {
                    psd->iid_index[env][b] = psd->iid_index[env - 1][b] + delta;
                }
                /* Clip index to [-7, 7] range */
                if (psd->iid_index[env][b] < -7) psd->iid_index[env][b] = -7;
                if (psd->iid_index[env][b] > 7) psd->iid_index[env][b] = 7;
            }
        }
        for (b = 0; b < num_subbands; b++) {
            psd->iid_index_prev[b] = psd->iid_index[hdr->num_env - 1][b];
        }
    }

    /* Decode ICC delta codes */
    if (hdr->enable_icc) {
        for (env = 0; env < hdr->num_env; env++) {
            hdr->icc_dt[env] = GetBits(bsi, 1);
            for (b = 0; b < num_subbands; b++) {
                int delta = DecodePSHuffman(bsi, 1); /* Decode ICC Huffman code */
                if (hdr->icc_dt[env] && env > 0) {
                    psd->icc_index[env][b] = psd->icc_index[env - 1][b] + delta;
                } else if (env == 0) {
                    psd->icc_index[env][b] = psd->icc_index_prev[b] + delta;
                } else {
                    psd->icc_index[env][b] = psd->icc_index[env - 1][b] + delta;
                }
                /* Clip index to [0, 7] range */
                if (psd->icc_index[env][b] < 0) psd->icc_index[env][b] = 0;
                if (psd->icc_index[env][b] > 7) psd->icc_index[env][b] = 7;
            }
        }
        for (b = 0; b < num_subbands; b++) {
            psd->icc_index_prev[b] = psd->icc_index[hdr->num_env - 1][b];
        }
    }

    /* Compute mixing matrix coefficients in Q30 fixed-point */
    for (env = 0; env < hdr->num_env; env++) {
        for (b = 0; b < num_subbands; b++) {
            int iid_idx = psd->iid_index[env][b] + 7;
            int icc_idx = psd->icc_index[env][b];

            int c1 = iid_scale_tab[iid_idx];
            int c2 = iid_scale_tab[14 - iid_idx];
            int rho = icc_scale_tab[icc_idx];

            /* Fixed-point Q30 PS gains */
            psd->h11[env][b] = MUL_Q30(c1, rho);
            psd->h12[env][b] = MUL_Q30(c1, Q30(1.0) - rho);
            psd->h21[env][b] = MUL_Q30(c2, rho);
            psd->h22[env][b] = -MUL_Q30(c2, Q30(1.0) - rho);
        }
    }

    return 0;
}

/**************************************************************************************
 * Function:    ProcessPSSlot
 *
 * Description: Apply fixed-point PS mixing matrix & allpass decorrelator to slot l
 **************************************************************************************/
void ProcessPSSlot(PSData *psd, int Xbuf_slot[64][2], int slot_L[64][2], int slot_R[64][2], int l)
{
    int k, env;
    PSHeader *hdr = &psd->hdr;

    /* Determine active PS envelope for slot l */
    env = 0;
    for (int e = 0; e < hdr->num_env; e++) {
        if (l >= hdr->border_position[e] && l < hdr->border_position[e + 1]) {
            env = e;
            break;
        }
    }

    for (k = 0; k < 64; k++) {
        int re = Xbuf_slot[k][0];
        int im = Xbuf_slot[k][1];

        if (k < PS_SUBBANDS_DECORR) {
            int b = (k < 20) ? k : 20 + ((k - 20) >> 1);
            int h11 = psd->h11[env][b];
            int h12 = psd->h12[env][b];
            int h21 = psd->h21[env][b];
            int h22 = psd->h22[env][b];

            /* Allpass filter stage using alpha_tab coefficient g */
            int g = alpha_tab[k & 7];

            /* True Allpass filter: w[n] = g * (x[n] - w_prev) + w_prev */
            int w_re = MUL_Q30(g, re - psd->allpass_delay[k][0][0]) + psd->allpass_delay[k][0][0];
            int w_im = MUL_Q30(g, im - psd->allpass_delay[k][0][1]) + psd->allpass_delay[k][0][1];

            psd->allpass_delay[k][0][0] = w_re;
            psd->allpass_delay[k][0][1] = w_im;

            int d_re = w_re;
            int d_im = w_im;

            /* Left channel subband = h11 * S + h12 * D */
            slot_L[k][0] = MUL_Q30(re, h11) + MUL_Q30(d_re, h12);
            slot_L[k][1] = MUL_Q30(im, h11) + MUL_Q30(d_im, h12);

            /* Right channel subband = h21 * S + h22 * D */
            slot_R[k][0] = MUL_Q30(re, h21) + MUL_Q30(d_re, h22);
            slot_R[k][1] = MUL_Q30(im, h21) + MUL_Q30(d_im, h22);
        } else {
            /* Mono passthrough for high subbands */
            slot_L[k][0] = re;
            slot_L[k][1] = im;
            slot_R[k][0] = re;
            slot_R[k][1] = im;
        }
    }
}
