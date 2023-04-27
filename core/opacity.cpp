//
// Created by Dinghao Zeng on 2023/4/27.
//

#include "opacity.h"

#include "core/animrun.h"
#include "core/webprun.h"
#include "core/gifrun.h"
#include "core/imgrun.h"
#include "core/filefmt.h"
#include "core/rawgif.h"

#include "webp/mux_types.h" // WebPData
#include "../imageio/imageio_util.h" // ImgIoUtilReadFile


#ifdef WEBP_HAVE_GIF
#include "gif_lib.h"
#include "core/gifcompat.h"
#endif

#include "core/logger.h"
#include "core/check.h"
#include "utils/defer.h"

struct Context {
    int sample_divider;
    int frame_count;
    int n_samples;
    float acc_opacity;
};


static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    return 1;
}

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);


    if (thiz->sample_divider == 0 || thiz->frame_count % thiz->sample_divider == 0) {
        float opacity = 0;
        check(AnimFrameGetOpacity(frame, &opacity));
        thiz->acc_opacity += opacity;
        thiz->n_samples += 1;
    }

    ++thiz->frame_count;

    return 1;
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    return 1;
}



static AnimDecRunCallback kRunCallback {
        .on_start = OnStart,
        .on_frame = OnFrame,
        .on_end = OnEnd
};

int AnimToolGetOpacity(
        const char*const image_path,
        int sample_divider,
        float* out_opacity
) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    check(ImgIoUtilReadFile(image_path, &webp_data.bytes, &webp_data.size));

    Context ctx{
            .sample_divider = sample_divider
    };

    if (IsWebP(&webp_data)) {
        check(WebPDecRunWithData(&webp_data, &ctx, kRunCallback));
    } else if (IsGIF(&webp_data)) {
        check(GIFDecRun(image_path, &ctx, kRunCallback));
    } else {
        if (!ImgDecRunWithData(&webp_data, &ctx, kRunCallback)) {
            auto last_dot = strrchr(image_path, '.');
            notreached("Failed. Please check your file type: `%s`", last_dot ? (last_dot + 1) : image_path);
        }
    }

    if (ctx.n_samples == 0) {
        *out_opacity = 0;
    } else {
        *out_opacity = ctx.acc_opacity / static_cast<float>(ctx.n_samples);
    }

    return 1;
}
