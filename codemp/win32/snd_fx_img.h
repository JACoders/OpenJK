#pragma once

typedef enum _DSP_IMAGE_image_FX_INDICES {
    GraphI3DL2_I3DL2Reverb = 0,
    GraphXTalk_XTalk = 1,
    GraphVoice_Voice_0 = 2,
    GraphVoice_Voice_1 = 3,
    GraphVoice_Voice_2 = 4,
    GraphVoice_Voice_3 = 5
} DSP_IMAGE_image_FX_INDICES;

#define DSI3DL2_ENVIRONMENT_GraphI3DL2_I3DL2Reverb -1000, -100, 0.000000, 1.490000, 0.830000, -2602, 0.007000, 200, 0.011000, 100.000000, 100.000000, 5000.000000

typedef struct _GraphI3DL2_FX0_I3DL2Reverb_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[2];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[35];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphI3DL2_FX0_I3DL2Reverb_STATE, *LPGraphI3DL2_FX0_I3DL2Reverb_STATE;

typedef const GraphI3DL2_FX0_I3DL2Reverb_STATE *LPCGraphI3DL2_FX0_I3DL2Reverb_STATE;

typedef struct _GraphXTalk_FX0_XTalk_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[4];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[4];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphXTalk_FX0_XTalk_STATE, *LPGraphXTalk_FX0_XTalk_STATE;

typedef const GraphXTalk_FX0_XTalk_STATE *LPCGraphXTalk_FX0_XTalk_STATE;

typedef struct _GraphVoice_FX0_Voice_0_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[1];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[1];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphVoice_FX0_Voice_0_STATE, *LPGraphVoice_FX0_Voice_0_STATE;

typedef const GraphVoice_FX0_Voice_0_STATE *LPCGraphVoice_FX0_Voice_0_STATE;

typedef struct _GraphVoice_FX1_Voice_1_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[1];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[1];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphVoice_FX1_Voice_1_STATE, *LPGraphVoice_FX1_Voice_1_STATE;

typedef const GraphVoice_FX1_Voice_1_STATE *LPCGraphVoice_FX1_Voice_1_STATE;

typedef struct _GraphVoice_FX2_Voice_2_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[1];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[1];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphVoice_FX2_Voice_2_STATE, *LPGraphVoice_FX2_Voice_2_STATE;

typedef const GraphVoice_FX2_Voice_2_STATE *LPCGraphVoice_FX2_Voice_2_STATE;

typedef struct _GraphVoice_FX3_Voice_3_STATE {
    DWORD dwScratchOffset;        // Offset in bytes, of scratch area for this FX
    DWORD dwScratchLength;        // Length in DWORDS, of scratch area for this FX
    DWORD dwYMemoryOffset;        // Offset in DSP WORDS, of Y memory area for this FX
    DWORD dwYMemoryLength;        // Length in DSP WORDS, of Y memory area for this FX
    DWORD dwFlags;                // FX bitfield for various flags. See xgpimage documentation
    DWORD dwInMixbinPtrs[1];      // XRAM offsets in DSP WORDS, of input mixbins
    DWORD dwOutMixbinPtrs[1];     // XRAM offsets in DSP WORDS, of output mixbins
} GraphVoice_FX3_Voice_3_STATE, *LPGraphVoice_FX3_Voice_3_STATE;

typedef const GraphVoice_FX3_Voice_3_STATE *LPCGraphVoice_FX3_Voice_3_STATE;
