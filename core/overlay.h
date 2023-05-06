//
// Created by Dinghao Zeng on 2023/5/6.
//

#ifndef ANIMTOOL_OVERLAY_H
#define ANIMTOOL_OVERLAY_H

int AnimToolOverlay(
        const char* input,
        const char* overlay_path,
        int x, int y,
        const char* rgba_str,

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

#endif //ANIMTOOL_OVERLAY_H
