//
// Created by Dinghao Zeng on 2023/3/3.
//

#ifndef ANIMTOOL_CG_H
#define ANIMTOOL_CG_H

#include <stdlib.h>
#include <cstdint>

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

        int Width() const {
            return size.width;
        }

        int Height() const {
            return size.height;
        }
    };


    static inline Rect FitTo(Size constraint, Size ratio) {
        if (constraint.width * ratio.height < constraint.height * ratio.width) {
            Size ret_size = {
                    .width = constraint.width,
                    .height = ratio.height * constraint.width / ratio.width
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
                    .width = ratio.width * constraint.height / ratio.height,
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

    struct Color {
        static Color FromRGBA(uint32_t rgba) {
            return Color((rgba >> 24) & 0x000000FF, (rgba >> 16) & 0x000000FF, (rgba >> 8) & 0x000000FF, (rgba >> 0) & 0x000000FF);
        }

        static Color FromARGB(uint32_t argb) {
            return Color((argb >> 16) & 0x000000FF, (argb >> 8) & 0x000000FF, (argb >> 0) & 0x000000FF, (argb >> 24) & 0x000000FF);
        }
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
        Color(const Color& other): r(other.r), g(other.g), b(other.b), a(other.a) {}
        Color& operator=(const Color& other) {
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

    static inline Color Blend(Color bottom, Color top) {
        auto alpha_bottom = bottom.a / 255.0f;
        auto alpha_top = top.a / 255.0f;

        auto alpha = alpha_bottom + alpha_top - alpha_top * alpha_bottom;
        if (alpha == 0) {
            return Color::FromARGB(0);
        }

        auto r = (top.r*alpha_top + bottom.r*alpha_bottom*(1 - alpha_top)) / alpha;
        auto g = (top.g*alpha_top + bottom.g*alpha_bottom*(1 - alpha_top)) / alpha;
        auto b = (top.b*alpha_top + bottom.b*alpha_bottom*(1 - alpha_top)) / alpha;

        return Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(alpha * 255));
    }
}

#endif //ANIMTOOL_CG_H
