//
// Created by Dinghao Zeng on 2023/2/22.
//

#ifndef MERCURY_ANIMATE_H
#define MERCURY_ANIMATE_H


#ifdef __cplusplus
extern "C" {
#endif


int AnimToolAnimate(
        const char*const image_paths[],
        int n_images,
        const char* background,
        int bg_blur_radius,
        int width,
        int height,
        int duration,
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


int AnimToolAnimateLite(
        const char*const image_paths[],
        int n_images,
        const char* background,
        int bg_blur_radius,
        int width,
        int height,
        int duration,
        const char* output
);


#ifdef __cplusplus
}
#endif

#endif //MERCURY_ANIMATE_H
