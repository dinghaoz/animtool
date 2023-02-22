//
// Created by Dinghao Zeng on 2023/1/16.
//

#include "animrun.h"
#include "webp/encode.h"

#include "check.h"

void AnimFrameInitWithRGBA(AnimFrame* frame, uint8_t *rgba, int width, int height) {
    frame->rgba.buf = rgba;
    frame->rgba.width = width;
    frame->rgba.height = height;
    frame->type = ANIME_FRAME_RGBA;
}


void AnimFrameInitWithPic(AnimFrame* frame, WebPPicture *pic) {
    frame->pic = pic;
    frame->type = ANIME_FRAME_PIC;
}


int AnimFrameExportToPic(const AnimFrame* frame, WebPPicture *pic) {
    pic->use_argb = 1;

    switch (frame->type) {
        case ANIME_FRAME_RGBA:
            pic->width = frame->rgba.width;
            pic->height = frame->rgba.height;
            check(WebPPictureImportRGBA(pic, frame->rgba.buf, frame->rgba.width * sizeof(uint32_t)));
            break;

        case ANIME_FRAME_PIC:
            pic->width = frame->pic->width;
            pic->height = frame->pic->height;
            check(WebPPictureCopy(frame->pic, pic));
            break;
        default:
            notreached("Unsupported AnimFrame type");
    }

    return 1;
}
