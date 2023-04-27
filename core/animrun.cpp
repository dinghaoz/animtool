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

int AnimFrameGetOpacity(const AnimFrame* frame, float *out_opacity) {
    float opacity = 0;
    switch (frame->type) {
        case ANIME_FRAME_RGBA:
            for (int y=0; y<frame->rgba.height; ++y) {
                for (int x=0; x<frame->rgba.width; ++x) {
                    auto pixel = (frame->rgba.buf + (frame->rgba.width * sizeof(uint32_t)) * y + sizeof(uint32_t)*x);
                    auto alpha = *(pixel + 3);
                    opacity += static_cast<float >(alpha) / 255.0f;
                }
            }
            *out_opacity = opacity / static_cast<float>(frame->rgba.height * frame->rgba.width);
            break;

        case ANIME_FRAME_PIC:
            for (int y=0; y<frame->pic->height; ++y) {
                auto argb_line = frame->pic->argb + frame->pic->argb_stride * y;
                for (int x=0; x<frame->pic->width; ++x) {
                    auto pixel = argb_line[x];
                    auto alpha = pixel >> 24;
                    opacity += static_cast<float >(alpha) / 255.0f;
                }
            }
            *out_opacity = opacity / static_cast<float>(frame->pic->height * frame->pic->width);
            break;
        default:
            notreached("Unsupported AnimFrame type");
    }


    return 1;
}
