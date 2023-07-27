//
// Created by Dinghao Zeng on 2023/3/3.
//

#include "picutils.h"

#include "blur.h"

#include "../imageio/imageio_util.h"
#include "../imageio/image_dec.h"
#include "webp/encode.h"
#include "webp/demux.h"

#include "cg.h"
#include "check.h"
#include "utils/defer.h"

#include <cmath>

void PicClear(WebPPicture* pic, cg::Color color) {

    for (int y=0; y<pic->height; ++y) {
        auto argb_line = pic->argb + pic->argb_stride * y;
        for (int x=0; x<pic->width; ++x) {
            argb_line[x] = color.ToARGB();
        }
    }
}

void PicTint(WebPPicture* pic, cg::Color color) {
    for (int y=0; y<pic->height; ++y) {
        auto argb_line = pic->argb + pic->argb_stride * y;
        for (int x=0; x<pic->width; ++x) {
            auto a = cg::Color::FromARGB(argb_line[x]).a;
            argb_line[x] = cg::Color(color.r, color.g, color.b, a).ToARGB();
        }
    }
}


int PicMerge(WebPPicture* dst, const WebPPicture* src, cg::Point point, cg::Color (*Merger)( const cg::Color&,  const cg::Color&)) {
    checkf(point.x + src->width <= dst->width, "Invalid geometry point.x=%d, src->width=%d, dst->width=%d", point.x, src->width, dst->width);
    checkf(point.y + src->height <= dst->height, "Invalid geometry point.y=%d, src->height=%d, dst->height=%d", point.y, src->height, dst->height);

    const size_t src_stride = src->argb_stride;
    const size_t dst_stride = dst->argb_stride;


    for (int y=0; y<dst->height; ++y) {
        for (int x=0; x<dst->width; ++x) {

            auto dst_pixel = dst->argb[y * dst_stride + x];
            uint32_t src_pixel = 0;
            if (point.y <= y && y<point.y + src->height && point.x <= x && x<point.x + src->width) {

                auto src_x = x - point.x;
                auto src_y = y - point.y;
                src_pixel = src->argb[src_y * src_stride + src_x];
            }

            dst->argb[y * dst_stride + x] = Merger(cg::Color::FromARGB(dst_pixel), cg::Color::FromARGB(src_pixel)).ToARGB();
        }
    }

    for (int y=0; y<src->height; ++y) {
        for (int x=0; x<src->width; ++x) {
            auto src_pixel = src->argb[y * src_stride + x];
            auto dst_x = x + point.x;
            auto dst_y = y + point.y;
            auto dst_pixel = dst->argb[dst_y * dst_stride + dst_x];

            dst->argb[dst_y * dst_stride + dst_x] = Merger(cg::Color::FromARGB(dst_pixel), cg::Color::FromARGB(src_pixel)).ToARGB();
        }
    }

    return 1;
}

int PicDraw(WebPPicture* dst, const WebPPicture* src, cg::Point point, int over) {
    if (over)
        return PicMerge(dst, src, point, cg::Blend);
    else
        return PicMerge(dst, src, point, cg::RevBlend);
}

int PicMask(WebPPicture* dst, const WebPPicture* mask, cg::Point point) {
    return PicMerge(dst, mask, point, cg::Mask);
}


int PicDrawOverFit(WebPPicture* dst, WebPPicture* src, int over) {

    auto fit_rect = cg::FitTo(
            cg::Size {.width = dst->width, .height = dst->height},
            cg::Size {.width = src->width, .height = src->height}
    );

    logger::d("fit_rect %d:%d:%d:%d", fit_rect.origin.x, fit_rect.origin.y, fit_rect.size.width, fit_rect.size.height);
    check(WebPPictureRescale(src, fit_rect.size.width, fit_rect.size.height));
    check(PicDraw(dst, src, fit_rect.origin, over));

    return 1;
}

int PicFill(WebPPicture* pic, cg::Size dst) {
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

int PicInitWithFile(WebPPicture* pic, const char* path) {

    WebPData data;
    WebPDataInit(&data);
    defer(WebPDataClear(&data));

    check(ImgIoUtilReadFile(path, &data.bytes, &data.size));

    auto reader = WebPGuessImageReader(data.bytes, data.size);

    pic->use_argb = 1;
    check(reader(data.bytes, data.size, pic, 1, NULL));

    return 1;
}




void PicToRGB(WebPPicture* pic, unsigned char* rgb) {
    for (int y=0; y<pic->height; ++y) {
        for (int x=0; x<pic->width; ++x) {
            auto color = cg::Color::FromARGB(pic->argb[y * pic->argb_stride + x]);
            auto pix = (y * pic->width + x) * 3;
            rgb[pix] = color.r;
            rgb[pix + 1] = color.g;
            rgb[pix + 2] = color.b;
        }
    }
}

void RGBToPic(unsigned char* rgb, WebPPicture* pic) {
    for (int y=0; y<pic->height; ++y) {
        for (int x=0; x<pic->width; ++x) {
            auto color = cg::Color::FromARGB(pic->argb[y * pic->argb_stride + x]);
            auto pix = (y * pic->width + x) * 3;
            pic->argb[y * pic->argb_stride + x] = cg::Color(rgb[pix], rgb[pix + 1], rgb[pix + 2], color.a).ToARGB();
        }
    }
}


int PicBlur(WebPPicture* pic, int radius) {
    auto rgb = reinterpret_cast<unsigned char*>(malloc(pic->width * pic->height * 3));
    check(rgb);
    defer(free(rgb));

    PicToRGB(pic, rgb);
    GaussianBlur(rgb, pic->width, pic->height, 3, radius);
    //superFastBlur(rgb, pic->width, pic->height, 5);
    RGBToPic(rgb, pic);

    return 1;
}
