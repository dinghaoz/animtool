//
// Created by Dinghao Zeng on 2023/1/13.
//

#ifndef MERCURY_DROPFRAMES_H
#define MERCURY_DROPFRAMES_H

#define MAX_N_TRANSFORMS 10
#define MAX_N_TRANSFORM_DSTS 10
#define MAX_FILE_NAME_LENGTH 256
#define MAX_FORMAT_LENGTH 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FrameTransformRectAbs {
    int left;
    int top;
    int width;
    int height;
} FrameTransformRectAbs;

typedef enum FrameRectGravity {
    FRG_CENTER = (1u << 0)
} FrameRectGravity;

typedef struct FrameTransformRectRel {
    int ratio_x;
    int ratio_y;
    int gravities;
} FrameTransformRectRel;

typedef enum FrameTransformRectType {
    FTRT_ABS,
    FTRT_REL
} FrameTransformRectType;

typedef struct FrameTransformSrc {
    union {
        FrameTransformRectAbs abs;
        FrameTransformRectRel rel;
    };
    FrameTransformRectType type;
} FrameTransformSrc;

typedef struct FrameTransformDst {
    int width;
    int height;
    char file_name[MAX_FILE_NAME_LENGTH];
    char format[MAX_FORMAT_LENGTH];
} FrameTransformDst;

typedef struct FrameTransform {
    FrameTransformSrc src;
    int n_dsts;
    FrameTransformDst dsts[MAX_N_TRANSFORM_DSTS];
} FrameTransform;


int AnimToolDropFrames(
    const char* const input,
    const char* const output,
    const char* const output_dir,
    int target_frame_rate,
    int target_total_duration,
    int loop_count,

    // global: WebPAnimEncoderOptions
    int minimize_size,
    int verbose,

    // per-frame: WebPConfig
    int lossless,
    float quality,
    int method,
    int pass,

    int n_transforms,
    const FrameTransform transforms[MAX_N_TRANSFORMS]
);

int AnimToolDropFramesLite(
    const char* const input,
    const char* const output,
    int target_frame_rate, // 0 means don't drop frames
    int target_total_duration, // 0 means don't shorten the duration
    int loop_count, // -1 means respecting the input file, 0 means infinite
    int src_left,
    int src_top,
    int src_width, // 0 means don't crop
    int src_height, // 0 means don't crop
    int dst_width, // 0 means don't resize
    int dst_height // 0 means don't resize
);

#ifdef __cplusplus
}
#endif


#endif //MERCURY_DROPFRAMES_H
