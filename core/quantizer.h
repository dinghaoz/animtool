// =============================================================
// Quantizer objects and functions
//
// Design and implementation by:
// - Herv� Drolon <drolon@infonie.fr>
// - Carsten Klein (cklein05@users.sourceforge.net)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// =============================================================

#ifndef FREEIMAGE_QUANTIZER_H
#define FREEIMAGE_QUANTIZER_H

#ifndef _MSC_VER
// define portable types for 32-bit / 64-bit OS
#include <inttypes.h>
typedef int32_t BOOL;
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int64_t INT64;
typedef uint64_t UINT64;
#else
// MS is not C99 ISO compliant
typedef long BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef signed __int64 INT64;
typedef unsigned __int64 UINT64;
#endif // _MSC_VER
/**
  Xiaolin Wu color quantization algorithm
*/
class WuQuantizer
{
public:

    typedef struct tagBox {
        int r0;			 // min value, exclusive
        int r1;			 // max value, inclusive
        int g0;
        int g1;
        int b0;
        int b1;
        int vol;
    } Box;

protected:
    float *gm2;
    LONG *wt, *mr, *mg, *mb;
    WORD *Qadd;

    // DIB data
    unsigned width, height;

protected:
    void Hist3DAddPixel(LONG *vwt, LONG *vmr, LONG *vmg, LONG *vmb, float *m2, int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void M3D(LONG *vwt, LONG *vmr, LONG *vmg, LONG *vmb, float *m2);
    LONG Vol(Box *cube, LONG *mmt);
    LONG Bottom(Box *cube, BYTE dir, LONG *mmt);
    LONG Top(Box *cube, BYTE dir, int pos, LONG *mmt);
    float Var(Box *cube);
    float Maximize(Box *cube, BYTE dir, int first, int last , int *cut,
                   LONG whole_r, LONG whole_g, LONG whole_b, LONG whole_w);
    bool Cut(Box *set1, Box *set2);
    void Mark(Box *cube, int label, BYTE *tag);

public:
    // Constructor - Input parameter: DIB 24-bit to be quantized
    int Init(int w, int h);

    // Destructor
    ~WuQuantizer();

    void AddPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    int Build(int PaletteSize, void* ctx, void(*Visitor)(void* ctx, int i, uint8_t r, uint8_t g, uint8_t b));
};


#endif // FREEIMAGE_QUANTIZER_H
