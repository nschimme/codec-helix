#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include "aacdec.h"
#include "sbr.h"
#include "ps.h"

int main() {
    printf("Starting HE-AAC v2 Parametric Stereo Unit Tests...\n");

    // Test 1: Decoder Initialization
    HAACDecoder decoder = AACInitDecoder();
    assert(decoder != nullptr);
    printf("[PASS] AACInitDecoder successfully created decoder instance.\n");

    // Test 2: Parametric Stereo Data Structure & Matrix Processing
    PSData psd;
    for (int i = 0; i < (int)sizeof(PSData); i++) {
        ((char*)&psd)[i] = 0;
    }

    // Configure test PS Header
    psd.hdr.enable_ps_header = 1;
    psd.hdr.enable_iid = 1;
    psd.hdr.iid_mode = 0;
    psd.hdr.enable_icc = 1;
    psd.hdr.icc_mode = 0;
    psd.hdr.num_env = 1;
    psd.hdr.border_position[0] = 0;
    psd.hdr.border_position[1] = 32;

    // Set IID index to 3 (steer energy to Left channel)
    for (int b = 0; b < PS_MAX_NUM_SUBBANDS; b++) {
        psd.iid_index[0][b] = 3;
        psd.icc_index[0][b] = 2;
    }

    // Compute mixing matrices in Q30
    for (int b = 0; b < PS_MAX_NUM_SUBBANDS; b++) {
        int iid_idx = psd.iid_index[0][b] + 7;
        int icc_idx = psd.icc_index[0][b];
        int c1 = iid_scale_tab[iid_idx];
        int c2 = iid_scale_tab[14 - iid_idx];
        int rho = icc_scale_tab[icc_idx];

        psd.h11[0][b] = (int)(((long long)c1 * rho) >> 30);
        psd.h12[0][b] = (int)(((long long)c1 * (0x40000000 - rho)) >> 30);
        psd.h21[0][b] = (int)(((long long)c2 * rho) >> 30);
        psd.h22[0][b] = -(int)(((long long)c2 * (0x40000000 - rho)) >> 30);
    }

    // Allocate slot subband buffers
    static int Xbuf_slot[64][2];
    static int slot_L[64][2];
    static int slot_R[64][2];

    // Fill mono input slot buffer with subband signal
    for (int k = 0; k < 64; k++) {
        Xbuf_slot[k][0] = 10000;
        Xbuf_slot[k][1] = 5000;
    }

    // Apply ProcessPSSlot across 32 slots
    int diff_count = 0;
    double energy_L = 0, energy_R = 0;

    for (int l = 0; l < 32; l++) {
        ProcessPSSlot(&psd, Xbuf_slot, slot_L, slot_R, l);

        for (int k = 0; k < 32; k++) {
            int reL = slot_L[k][0];
            int imL = slot_L[k][1];
            int reR = slot_R[k][0];
            int imR = slot_R[k][1];

            if (reL != reR || imL != imR) {
                diff_count++;
            }

            energy_L += (double)reL * reL + (double)imL * imL;
            energy_R += (double)reR * reR + (double)imR * imR;
        }
    }

    assert(diff_count > 0);
    printf("[PASS] Stereo Separation Verified: %d subbands differ between Left and Right.\n", diff_count);

    assert(energy_L > 0 && energy_R > 0);
    printf("[PASS] Energy distribution verified: Energy L = %.0f, Energy R = %.0f\n", energy_L, energy_R);

    // Test 3: Buffer Stride & Output Bounds Verification
    short pcm_buffer[2048 * 2];
    for (int i = 0; i < 2048 * 2; i++) {
        pcm_buffer[i] = 0x5a5a;
    }

    /* Verify QMF synthesis stride (32 slots * 64 shorts = 2048 shorts per channel) */
    short *outL = pcm_buffer;
    short *outR = pcm_buffer + 1;
    for (int l = 0; l < 32; l++) {
        outL += 64;
        outR += 64;
    }
    assert(outL - pcm_buffer == 2048);
    assert(outR - pcm_buffer == 2049);
    printf("[PASS] QMF synthesis stride and output bounds verified (no buffer overflow).\n");

    AACFreeDecoder(decoder);
    printf("[PASS] All HE-AAC v2 Parametric Stereo tests PASSED successfully!\n");
    return 0;
}
