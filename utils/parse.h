//
// Created by Dinghao Zeng on 2023/8/7.
//

#ifndef ANIMTOOL_PARSE_H
#define ANIMTOOL_PARSE_H

#include <cstring>

static const char* StrSearch(const char* pstart, const char* pend, const char* target) {
    const char* p = pstart;
    int target_len = strlen(target);
    while (p < pend) {
        if (!strncmp(target, p, target_len))
            break;
        ++p;
    }
    return p;
}

template<typename T>
static int ParseList(const char* pstart, const char* pend, const char* sep, T* values, int limit_n, int* pcount, int(*ElementParser)(const char* pstart, const char* pend, T* value)) {
    int i = 0;
    const char* itr = pstart;
    int lsep = strlen(sep);
    while (i < limit_n && itr <= pend) {
        const char* psep = StrSearch(itr, pend, sep);

        if (ElementParser(itr, psep, values+i)) {
            return -1;
        }
        ++i;

        itr = psep + lsep;
    }

    if (pcount) *pcount = i;

    return 0;
}



static int ParseInt(const char* pstart, const char* pend, int* value) {
    const char* p = pstart;
    int ret = 0;
    while (p < pend) {
        if ('0' <= *p && *p <= '9') {
            ret = ret * 10 + (*p-'0');
        } else {
            return -1;
        }

        ++p;
    }

    *value = ret;

    return 0;
}


#endif //ANIMTOOL_PARSE_H
