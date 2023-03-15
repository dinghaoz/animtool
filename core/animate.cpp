//
// Created by Dinghao Zeng on 2023/2/22.
//

#include "animate.h"
#include "animenc.h"
#include "picutils.h"

#include "../imageio/imageio_util.h"
#include "../imageio/image_dec.h"
#include "webp/encode.h"
#include "webp/demux.h"

#include "cg.h"
#include "blur.h"
#include "check.h"
#include "utils/defer.h"

#include <algorithm>



static int ParseColor(const char* pstart, const char* pend, uint8_t base, uint32_t* value) {
    require(base == 16 || base == 10 || base == 8);
    const char* p = pstart;
    int ret = 0;
    while (p < pend) {
        if ('a' <= *p && *p <= 'f') {
            require(base == 16);
            ret = ret * base + (*p-'a') + 10;
        } else if ('A' <= *p && *p <= 'F') {
            require(base == 16);
            ret = ret * base + (*p-'A') + 10;
        } else if ('0' <= *p && *p <= '9') {
            ret = ret * base + (*p-'0');
        } else {
            notreached("invalid color char %c", *p);
        }

        ++p;
    }

    *value = ret;

    return 1;
}

static uint32_t RGBAFrom(const char* color_str) {
    if (color_str[0] == '0' && color_str[1] == 'x') {
        uint32_t color = 0;
        check(ParseColor(color_str + 2, color_str + strlen(color_str), 16, &color));
        return color;
    } else {
        uint32_t color = 0;
        check(ParseColor(color_str, color_str + strlen(color_str), 10, &color));
        return color;
    }
}



static int BackgroundInit(WebPPicture* pic,
    const char* const image_paths[],
    int n_images,
    const char* background,
    int bg_blur_radius,
    int width,
    int height) {

    require(n_images > 0);
    auto colon = strchr(background, ':');
    require(colon);
    char bg_type[256] = {};
    strncpy(bg_type, background, std::min(static_cast<int>(sizeof(bg_type)), static_cast<int>(colon - background)));

    logger::d("bg_type %s", bg_type);
    pic->use_argb = 1;

    auto bg_content = colon + 1;

    cg::Size canvas_size {};
    if (width == 0 || height == 0) {
        if (!strcmp(bg_type, "file")) {
            check(PicInitWithFile(pic, bg_content));
            logger::d("implied size from bg %d:%d", pic->width, pic->height);
        } else {
            check(PicInitWithFile(pic, image_paths[0]));
            logger::d("implied size from first frame %d:%d", pic->width, pic->height);
        }

        canvas_size.width = pic->width;
        canvas_size.height = pic->height;
    } else {
        canvas_size.width = width;
        canvas_size.height = height;
    }


    if (!strcmp(bg_type, "file")) {
        if (pic->width == 0) {
            check(PicInitWithFile(pic, bg_content));
        }
        check(PicFill(pic, canvas_size));
        if (bg_blur_radius > 0) {
            check(PicBlur(pic, bg_blur_radius));
        }
    } else if (!strcmp(bg_type, "frame")) {
        auto frame_idx = atoi(bg_content);
        require(frame_idx < n_images);
        WebPPictureFree(pic);
        check(PicInitWithFile(pic, image_paths[frame_idx]));

        check(PicFill(pic, canvas_size));

        if (bg_blur_radius > 0) {
            check(PicBlur(pic, bg_blur_radius));
        }
    } else if (!strcmp(bg_type, "color")) {
        pic->width = width;
        pic->height = height;

        check(WebPPictureAlloc(pic));
        PicClear(pic, cg::Color::FromRGBA(RGBAFrom(bg_content)));
    } else {
        notreached("Unrecognized type: %s", bg_type);
    }

    return 1;
}


int AnimToolAnimate(
        const char* const image_paths[],
        int n_images,
        const char* background,
        int bg_blur_radius,
        int width,
        int height,
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

    require(n_images > 0);

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

    logger::d("bg_blur_radius %d", bg_blur_radius);


    WebPPicture bg;
    check(WebPPictureInit(&bg));
    defer(WebPPictureFree(&bg));
    check(BackgroundInit(&bg, image_paths, n_images, background, bg_blur_radius, width, height));


    auto encoder = AnimEncoderNew(format, width, height, &encoder_options);
    check(encoder);
    defer(if (encoder) AnimEncoderDelete(encoder); );


    WebPPicture canvas;
    check(WebPPictureInit(&canvas));
    defer(WebPPictureFree(&canvas));
    canvas.use_argb = 1;


    int total_duration_so_far = 0;
    for (int i=0; i<n_images; ++i) {

        WebPPicture pic;
        check(WebPPictureInit(&pic));
        defer(WebPPictureFree(&pic));
        pic.use_argb = 1;
        check(PicInitWithFile(&pic, image_paths[i]));

        check(WebPPictureCopy(&bg, &canvas));
        check(PicDrawOverFit(&canvas, &pic));

        auto end_ts = total_duration_so_far + duration;
        check(AnimEncoderAddFrame(encoder, &canvas, total_duration_so_far, end_ts, &frame_options));
        total_duration_so_far = end_ts;
    }

    check(AnimEncoderExport(encoder, total_duration_so_far, 0, output));

    return 1;
}



int AnimToolAnimateLite(
        const char*const image_paths[],
        int n_images,
        const char* background,
        int bg_blur_radius,
        int width,
        int height,
        int duration,
        const char* output
) {
    return AnimToolAnimate(
            image_paths, n_images, background, bg_blur_radius, width, height, duration, output, "webp",

            0, // minimize_size
            0, // verbose

            0, // lossless
            75.0f, // quality
            0, // method
            1 // pass
            );
}