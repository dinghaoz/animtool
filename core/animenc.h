//
// Created by Dinghao Zeng on 2023/2/3.
//

#ifndef MERCURY_ANIMENC_H
#define MERCURY_ANIMENC_H

#include <stdint.h>

struct AnimEncoder;

struct AnimEncoderOptions {
    int verbose;
    int minimize_size;
    uint32_t bgcolor;
};

struct AnimFrameOptions {
    int lossless;
    float quality;
    int method;
    int pass;
};


typedef struct WebPPicture WebPPicture;


AnimEncoder* AnimEncoderNew(const char* format, int canvas_width, int canvas_height, const AnimEncoderOptions* options);
int AnimEncoderAddFrame(AnimEncoder* encoder, WebPPicture* pic, int start_ts, int end_ts, const AnimFrameOptions* options);
int AnimEncoderExport(AnimEncoder* encoder, int final_ts, int loop_count, const char* output_path);
const char* AnimEncoderGetFileExt(const AnimEncoder* encoder);

void AnimEncoderDelete(AnimEncoder* encoder);

#endif //MERCURY_ANIMENC_H
