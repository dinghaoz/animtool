//
// Created by Dinghao Zeng on 2023/1/16.
//

#ifndef ANIMTOOL_ANIMRUN_H
#define ANIMTOOL_ANIMRUN_H


#include <stdint.h>

struct RawGifGlobalInfo;

typedef struct AnimInfo {
    int canvas_width;
    int canvas_height;
    uint32_t bgcolor;
    int has_loop_count;
    int loop_count;
    RawGifGlobalInfo* raw_gif;
} AnimInfo;

typedef struct WebPPicture WebPPicture;

typedef enum AnimFrameType {
    ANIME_FRAME_PIC,
    ANIME_FRAME_RGBA
} AnimFrameType;

struct RawGifLocalInfo;

typedef struct AnimFrame {
    union {
        WebPPicture* pic;
        struct {
            uint8_t* buf;
            int width;
            int height;
        } rgba;
    };
    AnimFrameType type;
    RawGifLocalInfo* raw_gif;
} AnimFrame;

#ifdef __cplusplus
extern "C" {
#endif

void AnimFrameInitWithRGBA(AnimFrame* frame, uint8_t *rgba, int width, int height);
void AnimFrameInitWithPic(AnimFrame* frame, WebPPicture *pic);
int AnimFrameExportToPic(const AnimFrame* frame, WebPPicture *pic);

#ifdef __cplusplus
}
#endif


typedef struct AnimDecRunCallback {
    int (*on_start)(void* ctx, const AnimInfo* anim_info, int* stop);
    int (*on_frame)(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop);
    int (*on_end)(void* ctx, const AnimInfo* anim_info);
} AnimDecRunCallback;


#endif //ANIMTOOL_ANIMRUN_H
