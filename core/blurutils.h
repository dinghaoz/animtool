//
// Created by Dinghao Zeng on 2023/3/3.
//

#ifndef ANIMTOOL_BLURUTILS_H
#define ANIMTOOL_BLURUTILS_H

void superFastBlur(unsigned char *pix, int w, int h, int radius);
void GaussianBlur(unsigned char* img,  int width, int height, int channels, int radius);

#endif //ANIMTOOL_BLURUTILS_H
