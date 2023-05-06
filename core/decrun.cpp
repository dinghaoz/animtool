//
// Created by Dinghao Zeng on 2023/5/6.
//

#include "decrun.h"
#include "animrun.h"
#include "webprun.h"
#include "gifrun.h"
#include "imgrun.h"
#include "filefmt.h"

#include "webp/encode.h" // WebPPicture
#include "webp/mux_types.h" // WebPData
#include "../imageio/imageio_util.h" // ImgIoUtilReadFile

#include "check.h"
#include "logger.h"
#include "utils/defer.h"

int DecRun(const char* input, void* ctx, AnimDecRunCallback callback) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    checkf(ImgIoUtilReadFile(input, &webp_data.bytes, &webp_data.size), "The input file cannot be open %s", input);

    if (IsWebP(&webp_data)) {
        check(WebPDecRunWithData(&webp_data, ctx, callback));
    } else if (IsGIF(&webp_data)) {
        check(GIFDecRun(input, ctx, callback));
    } else {
        if (!ImgDecRunWithData(&webp_data, ctx, callback)) {
            auto last_dot = strrchr(input, '.');
            notreached("Failed. Please check your file type: `%s`", last_dot ? (last_dot + 1) : input);
        }
    }

    return 1;
}
