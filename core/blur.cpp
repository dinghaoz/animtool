//
// Created by Dinghao Zeng on 2023/3/3.
//

#include "blur.h"
#include <stdlib.h>
#include <algorithm>
#include <cstring>

using namespace std;
void superFastBlur(unsigned char *pix, int w, int h, int radius)
{
    int div;
    int wm, hm, wh;
    int *vMIN, *vMAX;
    unsigned char *r, *g, *b, *dv;
    int rsum, gsum, bsum, x, y, i, p, p1, p2, yp, yi, yw;

    if (radius < 1) return;

    wm = w - 1;
    hm = h - 1;
    wh = w * h;
    div = radius + radius + 1;
    vMIN = (int *)malloc(sizeof(int) * max(w, h));
    vMAX = (int *)malloc(sizeof(int) * max(w, h));
    r = (unsigned char *)malloc(sizeof(unsigned char) * wh);
    g = (unsigned char *)malloc(sizeof(unsigned char) * wh);
    b = (unsigned char *)malloc(sizeof(unsigned char) * wh);
    dv = (unsigned char *)malloc(sizeof(unsigned char) * 256 * div);

    for (i = 0; i < 256 * div; i++)
        dv[i] = (i / div);

    yw = yi = 0;

    for (y = 0; y < h; y++)
    {
        rsum = gsum = bsum = 0;
        for (i = -radius; i <= radius; i++)
        {
            p = (yi + min(wm, max(i, 0))) * 3;
            bsum += pix[p];
            gsum += pix[p + 1];
            rsum += pix[p + 2];
        }
        for (x = 0; x < w; x++)
        {
            r[yi] = dv[rsum];
            g[yi] = dv[gsum];
            b[yi] = dv[bsum];

            if (y == 0)
            {
                vMIN[x] = min(x + radius + 1, wm);
                vMAX[x] = max(x - radius, 0);
            }
            p1 = (yw + vMIN[x]) * 3;
            p2 = (yw + vMAX[x]) * 3;

            bsum += pix[p1] - pix[p2];
            gsum += pix[p1 + 1] - pix[p2 + 1];
            rsum += pix[p1 + 2] - pix[p2 + 2];

            yi++;
        }
        yw += w;
    }

    for (x = 0; x < w; x++)
    {
        rsum = gsum = bsum = 0;
        yp = -radius * w;
        for (i = -radius; i <= radius; i++)
        {
            yi = max(0, yp) + x;
            rsum += r[yi];
            gsum += g[yi];
            bsum += b[yi];
            yp += w;
        }
        yi = x;
        for (y = 0; y < h; y++)
        {
            pix[yi * 3] = dv[bsum];
            pix[yi * 3 + 1] = dv[gsum];
            pix[yi * 3 + 2] = dv[rsum];

            if (x == 0)
            {
                vMIN[y] = min(y + radius + 1, hm) * w;
                vMAX[y] = max(y - radius, 0) * w;
            }
            p1 = x + vMIN[y];
            p2 = x + vMAX[y];

            rsum += r[p1] - r[p2];
            gsum += g[p1] - g[p2];
            bsum += b[p1] - b[p2];

            yi += w;
        }
    }

    free(r);
    free(g);
    free(b);

    free(vMIN);
    free(vMAX);
    free(dv);
}

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