//
// Created by Dinghao Zeng on 2023/3/3.
//

#ifndef ANIMTOOL_PICUTILS_H
#define ANIMTOOL_PICUTILS_H

#include "cg.h"

struct WebPPicture;

void PicClear(WebPPicture* pic, cg::Color color);

int PicDraw(WebPPicture* dst, const WebPPicture* src, cg::Point point);
int PicDrawOverFit(WebPPicture* dst, WebPPicture* src);

int PicFill(WebPPicture* pic, cg::Size dst);

int PicInitWithFile(WebPPicture* pic, const char* path);

int PicBlur(WebPPicture* pic, int radius);


#endif //ANIMTOOL_PICUTILS_H
