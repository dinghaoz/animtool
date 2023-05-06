//
// Created by Dinghao Zeng on 2023/5/6.
//

#ifndef ANIMTOOL_DECRUN_H
#define ANIMTOOL_DECRUN_H


#include "animrun.h"

#ifdef __cplusplus
extern "C" {
#endif

int DecRun(const char* input, void* ctx, AnimDecRunCallback callback);

#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_DECRUN_H
