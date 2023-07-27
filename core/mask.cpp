//
// Created by Dinghao Zeng on 2023/5/24.
//

#include "mask.h"

#include "picutils.h"
#include "clrparse.h"
#include "decrun.h"
#include "webp/encode.h"
#include "animenc.h"
#include "utils/defer.h"



struct Context {
    WebPPicture* mask;
    AnimEncoder* encoder;
    int fit;
    cg::Point point;

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

    if (thiz->fit) {
        auto fit_rect = cg::FitTo(cg::Size {.width = info->canvas_width, .height = info->canvas_height}, cg::Size{.width = thiz->mask->width, .height = thiz->mask->height});
        thiz->point = fit_rect.origin;
        check(WebPPictureRescale(thiz->mask, fit_rect.size.width, fit_rect.size.height));
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

    check(PicMask(&decoded_frame, thiz->mask, thiz->point));

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



int AnimToolMask(
        const char* input,
        const char* mask_path,
        int fit,
        int x, int y, // ignored if center == 1

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

    WebPPicture mask;
    check(WebPPictureInit(&mask));
    defer(WebPPictureFree(&mask));
    check(PicInitWithFile(&mask, mask_path));


    Context ctx{
            .mask = &mask,
            .fit = fit,
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