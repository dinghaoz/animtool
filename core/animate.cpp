//
// Created by Dinghao Zeng on 2023/2/22.
//

#include "animate.h"
#include "animenc.h"

#include "../imageio/image_enc.h"
#include "../imageio/imageio_util.h"
#include "../imageio/image_dec.h"
#include "webp/encode.h"
#include "webp/demux.h"


#include "check.h"
#include "utils/defer.h"

int AnimToolAnimate(
        const char* const image_paths[],
        int n_images,
        int duration,
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
    AnimEncoder* encoder = 0;
    defer(if (encoder) AnimEncoderDelete(encoder); );

    AnimEncoderOptions encoder_options {
        .verbose = verbose,
        .minimize_size = minimize_size,
    };

    AnimFrameOptions frame_options {
        .lossless = lossless,
        .quality = quality,
        .method = method,
        .pass = pass
    };


    int width = 0;
    int height = 0;

    int total_duration_so_far = 0;
    for (int i=0; i<n_images; ++i) {
        WebPData data;
        WebPDataInit(&data);
        defer(WebPDataClear(&data));

        check(ImgIoUtilReadFile(image_paths[i], &data.bytes, &data.size));

        auto reader = WebPGuessImageReader(data.bytes, data.size);

        WebPPicture pic;
        check(WebPPictureInit(&pic));
        defer(WebPPictureFree(&pic));
        pic.use_argb = 1;
        check(reader(data.bytes, data.size, &pic, 1, NULL));

        if (width == 0 || height == 0) {
            width = pic.width;
            height = pic.height;
        } else {
            checkf(width == pic.width && height == pic.height, "We only support images with same width and height for now");
        }

        if (encoder == 0) {
            encoder = AnimEncoderNew(format, width, height, &encoder_options);
            check(encoder);
        }

        auto end_ts = total_duration_so_far + duration;
        check(AnimEncoderAddFrame(encoder, &pic, total_duration_so_far, end_ts, &frame_options));

        total_duration_so_far = end_ts;
    }

    check(AnimEncoderExport(encoder, total_duration_so_far, 0, output));

    return 1;
}



int AnimToolAnimateLite(
        const char*const image_paths[],
        int n_images,
        int duration,
        const char* output
) {
    return AnimToolAnimate(
            image_paths, n_images, duration, output, "webp",

            0, // minimize_size
            0, // verbose

            0, // lossless
            75.0f, // quality
            0, // method
            1 // pass
            );
}