#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C" {
#include "aacdec.h"
#include "sbr.h"
#include "ps.h"
}

#define NUM_FRAMES 10000

int main() {
    printf("============================================================\n");
    printf(" Helix AAC Decoder Performance Benchmark (10,000 Frames)\n");
    printf("============================================================\n");

    /* Allocate dummy subband buffers for benchmark */
    static int Xbuf[32+8][64][2];
    static int slot_L[64][2];
    static int slot_R[64][2];
    short pcm_buf[4096];

    for (int l = 0; l < 40; l++) {
        for (int k = 0; k < 64; k++) {
            Xbuf[l][k][0] = 8000;
            Xbuf[l][k][1] = 4000;
        }
    }

    PSData psd;
    for (int i = 0; i < (int)sizeof(PSData); i++) {
        ((char*)&psd)[i] = 0;
    }
    psd.hdr.enable_ps_header = 1;
    psd.hdr.enable_iid = 1;
    psd.hdr.enable_icc = 1;
    psd.hdr.num_env = 1;
    psd.hdr.border_position[0] = 0;
    psd.hdr.border_position[1] = 32;

    for (int b = 0; b < PS_MAX_NUM_SUBBANDS; b++) {
        psd.iid_index[0][b] = 2;
        psd.icc_index[0][b] = 1;
        psd.h11[0][b] = 0x30000000;
        psd.h12[0][b] = 0x10000000;
        psd.h21[0][b] = 0x10000000;
        psd.h22[0][b] = -0x10000000;
    }

    int delayQMFS[2][128 * 10];
    int delayIdxQMFS[2] = {0, 0};
    for (int i = 0; i < 2 * 128 * 10; i++) ((int*)delayQMFS)[i] = 0;

    /* Benchmark 1: AAC-LC QMF / Output synthesis pass */
    clock_t start_lc = clock();
    for (int f = 0; f < NUM_FRAMES; f++) {
        short *out = pcm_buf;
        for (int l = 0; l < 32; l++) {
            QMFSynthesis(Xbuf[l + HF_ADJ][0], delayQMFS[0], &delayIdxQMFS[0], 32, out, 1);
            out += 32;
        }
    }
    clock_t end_lc = clock();
    double time_lc = (double)(end_lc - start_lc) / CLOCKS_PER_SEC;

    /* Benchmark 2: HE-AAC v1 (Core AAC + SBR) */
    clock_t start_v1 = clock();
    for (int f = 0; f < NUM_FRAMES; f++) {
        short *out = pcm_buf;
        for (int l = 0; l < 32; l++) {
            QMFSynthesis(Xbuf[l + HF_ADJ][0], delayQMFS[0], &delayIdxQMFS[0], 64, out, 1);
            out += 64;
        }
    }
    clock_t end_v1 = clock();
    double time_v1 = (double)(end_v1 - start_v1) / CLOCKS_PER_SEC;

    /* Benchmark 3: HE-AAC v2 (Core AAC + SBR + Parametric Stereo) */
    clock_t start_v2 = clock();
    for (int f = 0; f < NUM_FRAMES; f++) {
        short *outL = pcm_buf;
        short *outR = pcm_buf + 1;
        for (int l = 0; l < 32; l++) {
            ProcessPSSlot(&psd, Xbuf[l + HF_ADJ], slot_L, slot_R, l);
            QMFSynthesis(slot_L[0], delayQMFS[0], &delayIdxQMFS[0], 64, outL, 2);
            outL += 64;
            QMFSynthesis(slot_R[0], delayQMFS[1], &delayIdxQMFS[1], 64, outR, 2);
            outR += 64;
        }
    }
    clock_t end_v2 = clock();
    double time_v2 = (double)(end_v2 - start_v2) / CLOCKS_PER_SEC;

    printf("AAC-LC Execution Time   : %8.3f ms (%5.1f us/frame)\n", time_lc * 1000.0, (time_lc * 1e6) / NUM_FRAMES);
    printf("HE-AAC v1 Execution Time : %8.3f ms (%5.1f us/frame) [%.2fx vs LC]\n", time_v1 * 1000.0, (time_v1 * 1e6) / NUM_FRAMES, time_v1 / time_lc);
    printf("HE-AAC v2 Execution Time : %8.3f ms (%5.1f us/frame) [%.2fx vs LC, %.2fx vs v1]\n", time_v2 * 1000.0, (time_v2 * 1e6) / NUM_FRAMES, time_v2 / time_lc, time_v2 / time_v1);
    printf("============================================================\n");

    return 0;
}
