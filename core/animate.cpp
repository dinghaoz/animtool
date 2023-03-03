//
// Created by Dinghao Zeng on 2023/2/22.
//

#include "animate.h"
#include "animenc.h"

#include "../imageio/imageio_util.h"
#include "../imageio/image_dec.h"
#include "webp/encode.h"
#include "webp/demux.h"

#include "cg.h"
#include "blur.h"
#include "check.h"
#include "utils/defer.h"

#include <algorithm>
#include <cmath>

static void SetColor(WebPPicture* pic, uint32_t color) {

    for (int y=0; y<pic->height; ++y) {
        auto argb_line = pic->argb + pic->argb_stride * y;
        for (int x=0; x<pic->width; ++x) {
            argb_line[x] = color;
        }
    }
}

static int Draw(WebPPicture* dst, const WebPPicture* src, cg::Point point) {
    require(point.x + src->width <= dst->width);
    require(point.y + src->height <= dst->height);

    const size_t src_stride = src->argb_stride;
    const size_t dst_stride = dst->argb_stride;

    for (int y=0; y<src->height; ++y) {
        for (int x=0; x<src->width; ++x) {
            auto src_pixel = src->argb[y * src_stride + x];
            auto dst_x = x + point.x;
            auto dst_y = y + point.y;
            auto dst_pixel = dst->argb[dst_y * dst_stride + dst_x];

            dst->argb[dst_y * dst_stride + dst_x] = cg::Blend(cg::RGBA::FromARGB(dst_pixel), cg::RGBA::FromARGB(src_pixel)).ToARGB();
        }
    }

    return 1;
}


static int DrawOverFit(WebPPicture* dst, WebPPicture* src) {

    auto fit_rect = cg::FitTo(
            cg::Size {.width = dst->width, .height = dst->height},
            cg::Size {.width = src->width, .height = src->height}
    );

    logger::d("fit_rect %d:%d:%d:%d", fit_rect.origin.x, fit_rect.origin.y, fit_rect.size.width, fit_rect.size.height);
    check(WebPPictureRescale(src, fit_rect.size.width, fit_rect.size.height));
    check(Draw(dst, src, fit_rect.origin));

    return 1;
}

static int PicFill(WebPPicture* pic, cg::Size dst) {
    if (pic->width == dst.width && pic->height == dst.height) {
        return 1;
    }

    auto rv_fit_rect = cg::FitTo(
            cg::Size {.width = pic->width, .height = pic->height},
            cg::Size {.width = dst.width, .height = dst.height}
    );

    auto scale_factor = dst.width / static_cast<float>(rv_fit_rect.size.width);

    check(WebPPictureRescale(pic, static_cast<int>(round(pic->width * scale_factor)), static_cast<int>(round(pic->height * scale_factor))));

    auto crop_rect = cg::FitTo(
            cg::Size {.width = pic->width, .height = pic->height },
            dst
    );

    check(WebPPictureCrop(pic, crop_rect.Left(), crop_rect.Top(), dst.width, dst.height));

    return 1;
}

static int WebPPictureInitWithFile(WebPPicture* pic, const char* path) {

    WebPData data;
    WebPDataInit(&data);
    defer(WebPDataClear(&data));

    check(ImgIoUtilReadFile(path, &data.bytes, &data.size));

    auto reader = WebPGuessImageReader(data.bytes, data.size);

    check(reader(data.bytes, data.size, pic, 1, NULL));

    return 1;
}

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


cg::RGBA PicGetColor(void* ctx, int i_pix) {
    auto pic = reinterpret_cast<WebPPicture*>(ctx);

    int y = i_pix /pic->height;
    int x = i_pix % pic->width;

    int pos = y * pic->argb_stride + x;
    return cg::RGBA::FromARGB(pic->argb[pos]);
}

void PicSetColor(void* ctx, int i_pix, cg::RGBA color) {
    auto pic = reinterpret_cast<WebPPicture*>(ctx);

    int y = i_pix /pic->height;
    int x = i_pix % pic->width;

    int pos = y * pic->argb_stride + x;

    pic->argb[pos] = color.ToARGB();
}

void PicToRGB(WebPPicture* pic, unsigned char* rgb) {
    for (int y=0; y<pic->height; ++y) {
        for (int x=0; x<pic->width; ++x) {
            auto color = cg::RGBA::FromARGB(pic->argb[y * pic->argb_stride + x]);
            auto pix = y * pic->width + x;
            rgb[pix] = color.r;
            rgb[pix + 1] = color.g;
            rgb[pix + 2] = color.b;
        }
    }
}

void RGBToPic(unsigned char* rgb, WebPPicture* pic) {
    for (int y=0; y<pic->height; ++y) {
        for (int x=0; x<pic->width; ++x) {
            auto color = cg::RGBA::FromARGB(pic->argb[y * pic->argb_stride + x]);
            auto pix = y * pic->width + x;
            pic->argb[y * pic->argb_stride + x] = cg::RGBA(rgb[pix], rgb[pix+1], rgb[pix+2], color.a).ToARGB();
        }
    }
}


static int PicBlur(WebPPicture* pic, int radius) {
    auto rgb = reinterpret_cast<unsigned char*>(malloc(pic->width * pic->height * 3));
    check(rgb);

    defer(free(rgb));
    PicToRGB(pic, rgb);
    GaussianBlur(rgb, pic->width, pic->height, 3, radius);
    RGBToPic(rgb, pic);

    return 1;
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
            check(WebPPictureInitWithFile(pic, bg_content));
            logger::d("implied size from bg %d:%d", pic->width, pic->height);
        } else {
            check(WebPPictureInitWithFile(pic, image_paths[0]));
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
            check(WebPPictureInitWithFile(pic, bg_content));
        }
        check(PicFill(pic, canvas_size));
        if (bg_blur_radius > 0) {
            check(PicBlur(pic, bg_blur_radius));
        }
    } else if (!strcmp(bg_type, "frame")) {
        auto frame_idx = atoi(bg_content);
        require(frame_idx < n_images);
        WebPPictureFree(pic);
        check(WebPPictureInitWithFile(pic, image_paths[frame_idx]));

        check(PicFill(pic, canvas_size));

        if (bg_blur_radius > 0) {
            check(PicBlur(pic, bg_blur_radius));
        }
    } else if (!strcmp(bg_type, "color")) {
        pic->width = width;
        pic->height = height;

        check(WebPPictureAlloc(pic));
        SetColor(pic, cg::RGBA::FromRGBA(RGBAFrom(bg_content)).ToARGB());
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
        check(WebPPictureInitWithFile(&pic, image_paths[i]));

        check(WebPPictureCopy(&bg, &canvas));
        check(DrawOverFit(&canvas, &pic));

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