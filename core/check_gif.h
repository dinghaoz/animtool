//
// Created by Dinghao Zeng on 2023/2/3.
//

#ifndef MERCURY_CHECK_GIF_H
#define MERCURY_CHECK_GIF_H

#include "logger.h"

#ifdef HAVE_CONFIG_H
#include "webp/config.h"
#endif

#ifdef WEBP_HAVE_GIF
#include "gif_lib.h"
#include "imageio/gifdec.h"

enum {
    METADATA_ICC  = (1 << 0),
    METADATA_XMP  = (1 << 1),
    METADATA_ALL  = METADATA_ICC | METADATA_XMP
};

static inline void GIFGetErrorString(char* buf, int buf_len, int gif_error) {
    // libgif 4.2.0 has retired PrintGifError() and added GifErrorString().
#if LOCAL_GIF_PREREQ(4,2)
#if LOCAL_GIF_PREREQ(5,0)
    // Static string actually, hence the const char* cast.
    const char* error_str = (const char*)GifErrorString(gif_error);
#else
    const char* error_str = (const char*)GifErrorString();
#endif
    if (error_str == NULL) error_str = "Unknown error";
    snprintf(buf, buf_len, "GIFLib Error %d: %s", gif_error, error_str);
#else
  snprintf(buf, buf_len, "GIFLib Error %d: ", gif_error);
  PrintGifError();
#endif
}

#define log_gif_error(code_str, gif_error) \
do { \
    char buf[256];         \
    GIFGetErrorString(buf, sizeof(buf), gif_error); \
    logger::e("ERROR: Check `%s` failed. %s. @%s:%d", code_str, buf, __FILE__, __LINE__); \
} while (0)

#define check_gif(code, file) \
    do {            \
        if (!(code)) {             \
            log_gif_error(#code, file->Error); \
            return 0;        \
        }                \
    } while(0)

#endif

#endif //MERCURY_CHECK_GIF_H
