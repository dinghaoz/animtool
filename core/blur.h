//
// Created by Dinghao Zeng on 2023/10/13.
//

#ifndef ANIMTOOL_BLUR_H
#define ANIMTOOL_BLUR_H

#ifdef __cplusplus
extern "C" {
#endif

int AnimToolBlur(
    const char* image_path,
    int index_of_frame,
    int blur_radius,
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



#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_BLUR_H
