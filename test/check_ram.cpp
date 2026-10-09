#include <cstdio>
#include <cstdlib>

extern "C" {
#include "aacdec.h"
#include "sbr.h"
#include "ps.h"
}

int main() {
    printf("============================================================\n");
    printf(" HE-AAC v1 SBR (Mono vs Stereo) RAM Footprint Analysis\n");
    printf("============================================================\n");

#if defined(HELIX_FEATURE_AUDIO_CODEC_AAC_SBR_DOWNSAMPLED) && HELIX_FEATURE_AUDIO_CODEC_AAC_SBR_DOWNSAMPLED
    printf(" Mode: Downsampled SBR (Single-rate 1x Fs)\n");
#else
    printf(" Mode: Standard SBR (Dual-rate 2x Fs)\n");
#endif

    printf("  sizeof(PSInfoSBR) [Dual-channel CPE support] : %zu bytes\n", sizeof(PSInfoSBR));

    /* Calculate size if AAC_MAX_NCHANS was 1 (Mono-only SBR for internet radio) */
    size_t qmfa_size = 1 * DELAY_SAMPS_QMFA * sizeof(int);
    size_t qmfs_size = 1 * DELAY_SAMPS_QMFS * sizeof(int);
    size_t xbuf_size = 1 * HF_GEN * 64 * 2 * sizeof(int);

    printf("  Single-channel Mono SBR delay buffers total  : %zu bytes\n", qmfa_size + qmfs_size + xbuf_size);
    printf("============================================================\n");

    return 0;
}
