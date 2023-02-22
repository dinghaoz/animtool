//
// Created by Dinghao Zeng on 2023/1/16.
//

#include "webprun.h"

#include "../imageio/image_enc.h"
#include "../imageio/imageio_util.h"
#include "webp/demux.h"

#include "defer.h"
#include "check.h"

int WebPDecRunWithData(WebPData* webp_data, void* ctx, AnimDecRunCallback callback) {
    auto dec = WebPAnimDecoderNew(webp_data, nullptr);
    checkf(dec, "Failed to create decoder via WebPAnimDecoderNew.");
    defer(WebPAnimDecoderDelete(dec));

    WebPAnimInfo webp_info;
    check(WebPAnimDecoderGetInfo(dec, &webp_info));

    int stop = 0;

    AnimInfo anim_info {
        .canvas_width = static_cast<int>(webp_info.canvas_width),
        .canvas_height = static_cast<int>(webp_info.canvas_height),
        .bgcolor = webp_info.bgcolor,
        .has_loop_count = 1,
        .loop_count = static_cast<int>(webp_info.loop_count)
    };

    check(callback.on_start(ctx, &anim_info, &stop));
    if (stop) return 1;

    int in_start_ts = 0;
    while (WebPAnimDecoderHasMoreFrames(dec)) {
        int in_end_ts;
        uint8_t* in_frame_rgba = nullptr;

        // NOTE that, the meaning of timestamp in WebPAnimDecoderGetNext is different from that in
        // WebPAnimEncoderAdd.
        // in WebPAnimDecoderGetNext, timestamp of the first frame = the frame's duration
        // in WebPAnimEncoderAdd, timestamp of the first frame  = 0
        // This design seems strange but it actually make sense, as you don't need to worry about
        // whether the timestamp will exceed the total duration while encoding.

        check(WebPAnimDecoderGetNext(dec, &in_frame_rgba, &in_end_ts));

        AnimFrame frame{};
        AnimFrameInitWithRGBA(&frame, in_frame_rgba, anim_info.canvas_width, anim_info.canvas_height);
        check(callback.on_frame(ctx, &frame, in_start_ts, in_end_ts, &stop));
        in_start_ts = in_end_ts;
        if (stop) break; // still call on_end as on_start has been called
    }

    check(callback.on_end(ctx, &anim_info));
    return 1;
}

int WebPDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    check(ImgIoUtilReadFile(file_path, &webp_data.bytes, &webp_data.size));

    return WebPDecRunWithData(&webp_data, ctx, callback);
}
