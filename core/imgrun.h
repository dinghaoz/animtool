#ifndef ANIMTOOL_IMGRUN_H
#define ANIMTOOL_IMGRUN_H

#include "animrun.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WebPData WebPData;

int ImgDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback);
int ImgDecRunWithData(WebPData* webp_data, void* ctx, AnimDecRunCallback callback);

#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_IMGRUN_H