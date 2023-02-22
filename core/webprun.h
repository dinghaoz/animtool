//
// Created by Dinghao Zeng on 2023/1/16.
//

#ifndef ANIMTOOL_WEBPRUN_H
#define ANIMTOOL_WEBPRUN_H

#include "animrun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WebPData WebPData;

int WebPDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback);
int WebPDecRunWithData(WebPData* webp_data, void* ctx, AnimDecRunCallback callback);

#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_WEBPRUN_H
