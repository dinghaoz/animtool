//
// Created by Dinghao Zeng on 2023/10/13.
//

#include "blur.h"

#include "picutils.h"
#include "clrparse.h"
#include "decrun.h"
#include "webp/encode.h"
#include "animenc.h"
#include "utils/defer.h"

struct Context {
    int frame_count;
    int index_of_frame;
    int blur_radius;

    AnimEncoder* encoder;

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

static int BlurOneFrame(Context* thiz, const AnimFrame* frame, int start_ts, int end_ts) {
    WebPPicture decoded_frame;
    check(WebPPictureInit(&decoded_frame));
    defer(WebPPictureFree(&decoded_frame));

    check(AnimFrameExportToPic(frame, &decoded_frame));

    check(PicBlur(&decoded_frame, thiz->blur_radius));

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

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);


    if (thiz->index_of_frame >= 0) {
        if (thiz->frame_count == thiz->index_of_frame) {
            check(BlurOneFrame(thiz, frame, start_ts, end_ts));
            *stop = 1;
        }
    } else {
        check(BlurOneFrame(thiz, frame, start_ts, end_ts));
    }

    ++thiz->frame_count;

    return 1;




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
) {
    Context ctx{
        .index_of_frame = index_of_frame,
        .blur_radius = blur_radius,
        .output = output,
        .format = format,
        .minimize_size = minimize_size,
        .verbose = verbose,
        .lossless = lossless,
        .quality = quality,
        .method = method,
        .pass = pass,
    };

    check(DecRun(image_path, &ctx, kRunCallback));

    return 1;
}
