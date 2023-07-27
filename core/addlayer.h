//
// Created by Dinghao Zeng on 2023/5/6.
//

#ifndef ANIMTOOL_ADDLAYER_H
#define ANIMTOOL_ADDLAYER_H

int AnimToolAddLayer(
        const char* input,
        const char* layer_path,
        int overlay, // or else underlay
        int center,
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

#endif //ANIMTOOL_ADDLAYER_H
