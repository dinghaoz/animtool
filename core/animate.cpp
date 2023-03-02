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

static void SetColor(WebPPicture* pic, uint32_t color) {

    for (int y=0; y<pic->height; ++y) {
        auto argb_line = pic->argb + pic->argb_stride * y;
        for (int x=0; x<pic->width; ++x) {
            argb_line[x] = color;
        }
    }
}

namespace cg {
    struct Point {
        int x;
        int y;
    };

    struct Size {
        int width;
        int height;
    };

    struct Rect {
        Point origin;
        Size size;

        int Left() const {
            return origin.x;
        }

        int Top() const {
            return origin.y;
        }

        int Right() const {
            return origin.x + size.width;
        }

        int Bottom() const {
            return origin.y + size.height;
        }
    };


    Rect FitTo(Size constraint, Size size) {
        if (constraint.width * size.height < constraint.height * size.width) {
            Size ret_size = {
                    .width = constraint.width,
                    .height = size.height * constraint.width / size.width
            };
            return Rect {
                    .origin = {
                            .x = 0,
                            .y = (constraint.height - ret_size.height) / 2
                    },
                    .size = ret_size
            };
        } else {
            Size ret_size = {
                    .width = size.width * constraint.height /size.height,
                    .height = constraint.height
            };
            return Rect {
                    .origin = {
                            .x = (constraint.width - ret_size.width) / 2,
                            .y = 0
                    },
                    .size = ret_size
            };
        }
    }

    struct RGBA {
        static RGBA FromRGBA(uint32_t rgba) {
            return RGBA((rgba >> 24) & 0x000000FF, (rgba >> 16) & 0x000000FF, (rgba >> 8) & 0x000000FF, (rgba >> 0) & 0x000000FF);
        }

        static RGBA FromARGB(uint32_t argb) {
            return RGBA((argb >> 16) & 0x000000FF, (argb >> 8) & 0x000000FF, (argb >> 0) & 0x000000FF, (argb >> 24) & 0x000000FF);
        }
        RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
        RGBA(const RGBA& other): r(other.r), g(other.g), b(other.b), a(other.a) {}
        RGBA& operator=(const RGBA& other) {
            r = other.r;
            g = other.g;
            b = other.b;
            a = other.a;
            return *this;
        }
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;

        uint32_t ToARGB() const {
            return (static_cast<uint32_t>(a) << 24) |
                    (static_cast<uint32_t>(r) << 16) |
                    (static_cast<uint32_t>(g) << 8) |
                    (static_cast<uint32_t>(b) << 0);
        }
    };

    static RGBA Blend(RGBA bottom, RGBA top) {
        auto alpha_bottom = bottom.a / 255.0f;
        auto alpha_top = top.a / 255.0f;

        auto alpha = alpha_bottom + alpha_top - alpha_top * alpha_bottom;
        if (alpha == 0) {
            return RGBA::FromARGB(0);
        }

        auto r = (top.r*alpha_top + bottom.r*alpha_bottom*(1 - alpha_top)) / alpha;
        auto g = (top.g*alpha_top + bottom.g*alpha_bottom*(1 - alpha_top)) / alpha;
        auto b = (top.b*alpha_top + bottom.b*alpha_bottom*(1 - alpha_top)) / alpha;

        return RGBA(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(alpha * 255));
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


int AnimToolAnimate(
        const char* const image_paths[],
        int n_images,
        const char* background,
        const char* bg_color_str,
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


    auto bg_color = cg::RGBA::FromRGBA(RGBAFrom(bg_color_str));

    logger::d("bg_color ARGB %x", bg_color.ToARGB());
    if (width == 0 || height == 0) {
        if (background) {
            WebPPicture bg;
            check(WebPPictureInit(&bg));
            defer(WebPPictureFree(&bg));
            bg.use_argb = 1;

            check(WebPPictureInitWithFile(&bg, background));
            width = bg.width;
            height = bg.height;
            logger::d("implied size from bg %d:%d", width, height);
        } else {

            WebPPicture first_image;
            check(WebPPictureInit(&first_image));
            defer(WebPPictureFree(&first_image));
            first_image.use_argb = 1;

            check(WebPPictureInitWithFile(&first_image, image_paths[0]));
            width = first_image.width;
            height = first_image.height;
            logger::d("implied size from first frame %d:%d", width, height);
        }
    }

    WebPPicture model;
    check(WebPPictureInit(&model));
    defer(WebPPictureFree(&model));
    model.use_argb = 1;
    model.width = width;
    model.height = height;
    check(WebPPictureAlloc(&model));

    if (bg_color.a > 0) {
        SetColor(&model, bg_color.ToARGB());
    }

    if (background) {
        WebPPicture bg;
        check(WebPPictureInit(&bg));
        defer(WebPPictureFree(&bg));
        bg.use_argb = 1;

        check(WebPPictureInitWithFile(&bg, background));

        check(DrawOverFit(&model, &bg));
    }


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

        check(WebPPictureCopy(&model, &canvas));
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
        const char* bg_color_str,
        int width,
        int height,
        int duration,
        const char* output
) {
    return AnimToolAnimate(
            image_paths, n_images, background, bg_color_str, width, height, duration, output, "webp",

            0, // minimize_size
            0, // verbose

            0, // lossless
            75.0f, // quality
            0, // method
            1 // pass
            );
}