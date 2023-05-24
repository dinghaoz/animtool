//
// Created by Dinghao Zeng on 2023/5/24.
//

#ifndef ANIMTOOL_MASK_H
#define ANIMTOOL_MASK_H


int AnimToolMask(
        const char* input,
        const char* mask_path,
        int fit,
        int x, int y, // ignored if center == 1

        const char* output,
        const char* format,
        // global: WebPAnimEncoderOptions
        int minimize_size,
        int verbose,

        // per-frame: WebPConfig
        int lossless,
        float quality,
        int method,
        int pass
);

#endif //ANIMTOOL_MASK_H
