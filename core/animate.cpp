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
        RGBA(uint32_t argb): r((argb >> 16) & 0x000000FF), g((argb >> 8) & 0x000000FF), b((argb >> 0)  & 0x000000FF), a((argb >> 24)  & 0x000000FF) {}
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
            return RGBA(0);
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

            dst->argb[dst_y * dst_stride + dst_x] = cg::Blend(cg::RGBA(dst_pixel), cg::RGBA(src_pixel)).ToARGB();
        }
    }

    return 1;
}


int AnimToolAnimate(
        const char* const image_paths[],
        int n_images,
        int duration,
        int width,
        int height,
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


    WebPPicture canvas;
    check(WebPPictureInit(&canvas));
    defer(WebPPictureFree(&canvas));

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

        WebPPicture* frame = &pic;

        if (width == 0 || height == 0) {
            width = pic.width;
            height = pic.height;
            logger::d("implied size %d:%d", width, height);
        } else if (width != pic.width || height != pic.width) {
            if (canvas.width == 0) {
                canvas.width = width;
                canvas.height = height;
                canvas.use_argb = 1;
                check(WebPPictureAlloc(&canvas));
            }
            SetColor(&canvas, 0xFF000000);

            auto fit_rect = cg::FitTo(
                    cg::Size {.width = canvas.width, .height = canvas.height},
                    cg::Size {.width = pic.width, .height = pic.height}
            );

            logger::d("fit_rect %d:%d:%d:%d", fit_rect.origin.x, fit_rect.origin.y, fit_rect.size.width, fit_rect.size.height);
            check(WebPPictureRescale(&pic, fit_rect.size.width, fit_rect.size.height));
            check(Draw(&canvas, &pic, fit_rect.origin));

            frame = &canvas;
        }

        if (encoder == 0) {
            encoder = AnimEncoderNew(format, width, height, &encoder_options);
            check(encoder);
        }

        auto end_ts = total_duration_so_far + duration;

        check(AnimEncoderAddFrame(encoder, frame, total_duration_so_far, end_ts, &frame_options));

        total_duration_so_far = end_ts;
    }

    check(AnimEncoderExport(encoder, total_duration_so_far, 0, output));

    return 1;
}



int AnimToolAnimateLite(
        const char*const image_paths[],
        int n_images,
        int duration,
        int width,
        int height,
        const char* output
) {
    return AnimToolAnimate(
            image_paths, n_images, duration, width, height, output, "webp",

            0, // minimize_size
            0, // verbose

            0, // lossless
            75.0f, // quality
            0, // method
            1 // pass
            );
}