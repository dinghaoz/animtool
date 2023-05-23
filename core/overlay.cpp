//
// Created by Dinghao Zeng on 2023/5/6.
//

#include "overlay.h"

#include "picutils.h"
#include "clrparse.h"
#include "decrun.h"
#include "webp/encode.h"
#include "animenc.h"
#include "utils/defer.h"


struct Context {
    const WebPPicture* overlay;
    AnimEncoder* encoder;
    cg::Point point;
    int center;

    int total_duration_so_far;

    const char* output;
    const char* format;
    // global: WebPAnimEncoderOptions
    int minimize_size;
    int verbose;

    // per-frame: WebPConfig
    int lossless;
    float quality;
    int method;
    int pass;
};


static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    if (thiz->center) {
        auto x = (info->canvas_width - thiz->overlay->width)/2;
        auto y = (info->canvas_height - thiz->overlay->height)/2;
        thiz->point = cg::Point {
            .x = x,
            .y = y
        };
    }

    AnimEncoderOptions encoder_options {
            .verbose = thiz->verbose,
            .minimize_size = thiz->minimize_size,
            .bgcolor = info->bgcolor
    };
    auto encoder = AnimEncoderNew(thiz->format, info->canvas_width, info->canvas_height, &encoder_options);
    check(encoder);
    thiz->encoder = encoder;

    return 1;
}

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);


    WebPPicture decoded_frame;
    check(WebPPictureInit(&decoded_frame));
    defer(WebPPictureFree(&decoded_frame));

    check(AnimFrameExportToPic(frame, &decoded_frame));

    check(PicDraw(&decoded_frame, thiz->overlay, thiz->point));

    AnimFrameOptions frame_options {
            .lossless = thiz->lossless,
            .quality = thiz->quality,
            .method = thiz->method,
            .pass = thiz->pass
    };

    thiz->total_duration_so_far = end_ts;

    check(AnimEncoderAddFrame(thiz->encoder, &decoded_frame, start_ts, end_ts, &frame_options));

    return 1;
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    auto thiz = reinterpret_cast<Context*>(ctx);
    defer(AnimEncoderDelete(thiz->encoder));

    check(AnimEncoderExport(thiz->encoder, thiz->total_duration_so_far, anim_info->loop_count, thiz->output));

    return 1;
}



static AnimDecRunCallback kRunCallback {
        .on_start = OnStart,
        .on_frame = OnFrame,
        .on_end = OnEnd
};



int AnimToolOverlay(
        const char* input,
        const char* overlay_path,
        int center,
        int x, int y, // ignored if center == 1
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
        ) {
    WebPPicture overlay;
    check(PicInitWithFile(&overlay, overlay_path));
    auto tint_color = cg::Color::FromRGBA(RGBAFrom(rgba_str));
    if (tint_color.a > 0) {
        PicTint(&overlay, tint_color);
    }


    Context ctx{
        .overlay = &overlay,
        .center = center,
        .point = cg::Point {
            .x = x,
            .y = y
        },
        .output = output,
        .format = format,
        .minimize_size = minimize_size,
        .verbose = verbose,
        .lossless = lossless,
        .quality = quality,
        .method = method,
        .pass = pass,
    };

    check(DecRun(input, &ctx, kRunCallback));

    return 1;
}


