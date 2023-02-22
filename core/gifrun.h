//
// Created by Dinghao Zeng on 2023/1/16.
//

#ifndef ANIMTOOL_GIFRUN_H
#define ANIMTOOL_GIFRUN_H

#include "animrun.h"

#ifdef __cplusplus
extern "C" {
#endif

int GIFDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback);

#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_GIFRUN_H
