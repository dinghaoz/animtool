//
// Created by Dinghao Zeng on 2023/1/14.
//

#ifndef ANIMTOOL_WEBPCHECK_H
#define ANIMTOOL_WEBPCHECK_H

#include "logger.h"

#define check(code) \
    do {            \
        if (!(code)) {\
            logger::e("ERROR: Check `%s` failed. @%s:%d", #code, __FILE__, __LINE__); \
            return 0;        \
        }                \
    } while(0)

#define checkf(code, format, ...) \
    do {            \
        if (!(code)) {\
            char buf[1024] = {}; \
            snprintf(buf, sizeof(buf), format, ##__VA_ARGS__); \
            logger::e("ERROR: Check `%s` failed. %s. @%s:%d", #code, buf, __FILE__, __LINE__); \
            return 0;        \
        }                \
    } while(0)

#define notreached(format, ...) \
    do {            \
        char buf[1024] = {}; \
        snprintf(buf, sizeof(buf), format, ##__VA_ARGS__); \
        logger::e("ERROR: %s @%s:%d", buf, __FILE__, __LINE__); \
        return 0;        \
    } while(0)

#endif //ANIMTOOL_WEBPCHECK_H

#define require(code) checkf(code, "Inconsistent internal state.")
#define imply(cond, code) do { if ((cond)) require(code); } while (0)
#define concat0(str1, str2) (str1 " " str2)
#define requiref(code, format, ...) checkf(code, concat0("Inconsistent internal state. ", format), ##__VA_ARGS__)
#define implyf(cond, code, format, ...) do { if ((cond)) requiref(code, format, ##__VA_ARGS__); } while (0)