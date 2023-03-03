//
// Created by Dinghao Zeng on 2023/3/3.
//

#include "blur.h"
#include <stdlib.h>
#include <algorithm>

using namespace std;


void GaussianBlur(unsigned char* img,  int width, int height, int channels, int radius)
{
    radius = min(max(1, radius), 248);
    unsigned int kernelSize = 1 + radius * 2;
    unsigned int* kernel = (unsigned int*)malloc(kernelSize* sizeof(unsigned int));
    memset(kernel, 0, kernelSize* sizeof(unsigned int));
    int(*mult)[256] = (int(*)[256])malloc(kernelSize * 256 * sizeof(int));
    memset(mult, 0, kernelSize * 256 * sizeof(int));

    int	xStart = 0;
    int	yStart = 0;
    width = xStart + width - max(0, (xStart + width) - width);
    height = yStart + height - max(0, (yStart + height) - height);
    int imageSize = width*height;
    int widthstep = width*channels;
    if (channels == 3 || channels == 4)
    {
        unsigned char *    CacheImg = nullptr;
        CacheImg = (unsigned char *)malloc(sizeof(unsigned char) * imageSize * 6);
        if (CacheImg == nullptr) return;
        unsigned char *    rCache = CacheImg;
        unsigned char *    gCache = CacheImg + imageSize;
        unsigned char *    bCache = CacheImg + imageSize * 2;
        unsigned char *    r2Cache = CacheImg + imageSize * 3;
        unsigned char *    g2Cache = CacheImg + imageSize * 4;
        unsigned char *    b2Cache = CacheImg + imageSize * 5;
        int sum = 0;
        for (int K = 1; K < radius; K++){
            unsigned int szi = radius - K;
            kernel[radius + K] = kernel[szi] = szi*szi;
            sum += kernel[szi] + kernel[szi];
            for (int j = 0; j < 256; j++){
                mult[radius + K][j] = mult[szi][j] = kernel[szi] * j;
            }
        }
        kernel[radius] = radius*radius;
        sum += kernel[radius];
        for (int j = 0; j < 256; j++){
            mult[radius][j] = kernel[radius] * j;
        }
        for (int Y = 0; Y < height; ++Y) {
            unsigned char*     LinePS = img + Y*widthstep;
            unsigned char*     LinePR = rCache + Y*width;
            unsigned char*     LinePG = gCache + Y*width;
            unsigned char*     LinePB = bCache + Y*width;
            for (int X = 0; X < width; ++X) {
                int     p2 = X*channels;
                LinePR[X] = LinePS[p2];
                LinePG[X] = LinePS[p2 + 1];
                LinePB[X] = LinePS[p2 + 2];
            }
        }
        int kernelsum = 0;
        for (int K = 0; K < kernelSize; K++){
            kernelsum += kernel[K];
        }
        float fkernelsum = 1.0f / kernelsum;
        for (int Y = yStart; Y < height; Y++){
            int heightStep = Y * width;
            unsigned char*     LinePR = rCache + heightStep;
            unsigned char*     LinePG = gCache + heightStep;
            unsigned char*     LinePB = bCache + heightStep;
            for (int X = xStart; X < width; X++){
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++){
                    unsigned    int     readPos = ((X - radius + K + width) % width);
                    int * pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                    cg += pmult[LinePG[readPos]];
                    cb += pmult[LinePB[readPos]];
                }
                unsigned int p = heightStep + X;
                r2Cache[p] = cr* fkernelsum;
                g2Cache[p] = cg* fkernelsum;
                b2Cache[p] = cb* fkernelsum;
            }
        }
        for (int X = xStart; X < width; X++){
            int WidthComp = X*channels;
            int WidthStep = width*channels;
            unsigned char*     LinePS = img + X*channels;
            unsigned char*     LinePR = r2Cache + X;
            unsigned char*     LinePG = g2Cache + X;
            unsigned char*     LinePB = b2Cache + X;
            for (int Y = yStart; Y < height; Y++){
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++){
                    unsigned int   readPos = ((Y - radius + K + height) % height) * width;
                    int * pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                    cg += pmult[LinePG[readPos]];
                    cb += pmult[LinePB[readPos]];
                }
                int    p = Y*WidthStep;
                LinePS[p] = (unsigned char)(cr * fkernelsum);
                LinePS[p + 1] = (unsigned char)(cg * fkernelsum);
                LinePS[p + 2] = (unsigned char)(cb* fkernelsum);


            }
        }
        free(CacheImg);
    }
    else if (channels == 1)
    {
        unsigned char *    CacheImg = nullptr;
        CacheImg = (unsigned char *)malloc(sizeof(unsigned char) * imageSize * 2);
        if (CacheImg == nullptr) return;
        unsigned char *    rCache = CacheImg;
        unsigned char *    r2Cache = CacheImg + imageSize;

        int sum = 0;
        for (int K = 1; K < radius; K++){
            unsigned int szi = radius - K;
            kernel[radius + K] = kernel[szi] = szi*szi;
            sum += kernel[szi] + kernel[szi];
            for (int j = 0; j < 256; j++){
                mult[radius + K][j] = mult[szi][j] = kernel[szi] * j;
            }
        }
        kernel[radius] = radius*radius;
        sum += kernel[radius];
        for (int j = 0; j < 256; j++){
            mult[radius][j] = kernel[radius] * j;
        }
        for (int Y = 0; Y < height; ++Y) {
            unsigned char*     LinePS = img + Y*widthstep;
            unsigned char*     LinePR = rCache + Y*width;
            for (int X = 0; X < width; ++X) {
                LinePR[X] = LinePS[X];
            }
        }
        int kernelsum = 0;
        for (int K = 0; K < kernelSize; K++){
            kernelsum += kernel[K];
        }
        float fkernelsum = 1.0f / kernelsum;
        for (int Y = yStart; Y < height; Y++){
            int heightStep = Y * width;
            unsigned char*     LinePR = rCache + heightStep;
            for (int X = xStart; X < width; X++){
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++){
                    unsigned    int     readPos = ( (X - radius + K+width)%width);
                    int * pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                }
                unsigned int p = heightStep + X;
                r2Cache[p] = cr * fkernelsum;
            }
        }
        for (int X = xStart; X < width; X++){
            int WidthComp = X*channels;
            int WidthStep = width*channels;
            unsigned char*     LinePS = img + X*channels;
            unsigned char*     LinePR = r2Cache + X;
            for (int Y = yStart; Y < height; Y++){
                int cb = 0;
                int cg = 0;
                int cr = 0;
                for (int K = 0; K < kernelSize; K++){
                    unsigned int   readPos = ((Y - radius + K+height)%height) * width;
                    int * pmult = mult[K];
                    cr += pmult[LinePR[readPos]];
                }
                int    p = Y*WidthStep;
                LinePS[p] = (unsigned char)(cr* fkernelsum);
            }
        }
        free(CacheImg);
    }
    free(kernel);
    free(mult);
}