//
// Created by Dinghao Zeng on 2023/2/9.
//

#ifndef ANIMTOOL_RAWGIF_H
#define ANIMTOOL_RAWGIF_H

struct ColorMapObject;

struct RawGifGlobalInfo {
    int color_res;
    int bgcolor_index;
    ColorMapObject *color_map;
};


struct RawGifLocalInfo {
    ColorMapObject *color_map;
    int interlace;
    int transparent_index;
    int left, top, width, height;
    int dispose_method;
};

#endif //ANIMTOOL_RAWGIF_H
