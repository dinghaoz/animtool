//
// Created by Dinghao Zeng on 2023/5/6.
//

#ifndef ANIMTOOL_CLRPARSE_H
#define ANIMTOOL_CLRPARSE_H

#include <stdlib.h>
#include <cstdint>
#include <cstring>

#include "check.h"

static int ParseColor(const char* pstart, const char* pend, uint8_t base, uint32_t* value) {
    require(base == 16 || base == 10 || base == 8);
    const char* p = pstart;
    int ret = 0;
    while (p < pend) {
        if ('a' <= *p && *p <= 'f') {
            require(base == 16);
            ret = ret * base + (*p-'a') + 10;
        } else if ('A' <= *p && *p <= 'F') {
            require(base == 16);
            ret = ret * base + (*p-'A') + 10;
        } else if ('0' <= *p && *p <= '9') {
            ret = ret * base + (*p-'0');
        } else {
            notreached("invalid color char %c", *p);
        }

        ++p;
    }

    *value = ret;

    return 1;
}

static uint32_t RGBAFrom(const char* color_str) {
    if (color_str[0] == '0' && color_str[1] == 'x') {
        uint32_t color = 0;

        auto content = color_str + 2;
        char buf[8] = {};
        for (int i=0; i<sizeof(buf); ++i) {
            if (i < strlen(content)) {
                buf[i] = content[i];
            } else {
                buf[i] = (i >= 7) ? 'F' : '0';
            }
        }
        check(ParseColor(buf, buf + sizeof(buf), 16, &color));
        return color;
    } else {
        uint32_t color = 0;
        check(ParseColor(color_str, color_str + strlen(color_str), 10, &color));
        return color;
    }
}



#endif //ANIMTOOL_CLRPARSE_H
