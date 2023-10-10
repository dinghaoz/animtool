//
// Created by Dinghao Zeng on 2023/10/10.
//

#include "cluster.h"
#include "core/animrun.h"
#include "core/rawgif.h"
#include "decrun.h"
#include "cg.h"

#include "check.h"
#include "utils/defer.h"

#include "dkm.hpp"


struct Context {
    int frame_count;

    int index_of_frame;
    int x, y, width, height;
    int k;

    std::vector<std::array<float , 4>>* points;
    uint32_t* argbs;
};


static void CollectPoints(void* ctx, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    thiz->points->push_back({r/255.0f, g/255.0f, b/255.0f, a/255.0f});
}


static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    return 1;
}

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    if (thiz->index_of_frame >= 0) {
        if (thiz->frame_count == thiz->index_of_frame) {
            check(AnimFrameEnumerate(frame, thiz->x, thiz->y, thiz->width, thiz->height, ctx, CollectPoints));
            *stop = 1;
        }
    } else {
        check(AnimFrameEnumerate(frame, thiz->x, thiz->y, thiz->width, thiz->height, ctx, CollectPoints));
    }

    ++thiz->frame_count;

    return 1;
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    auto thiz = reinterpret_cast<Context*>(ctx);
    auto cluster_data = dkm::kmeans_lloyd(*thiz->points, thiz->k);

    auto p = thiz->argbs;
    for (const auto& mean : std::get<0>(cluster_data)) {
        *p = cg::Color(mean[0] * 255, mean[1]*255, mean[2]*255, mean[3]*255).ToARGB();
        ++p;
    }


    return 1;
}



static AnimDecRunCallback kRunCallback {
        .on_start = OnStart,
        .on_frame = OnFrame,
        .on_end = OnEnd
};


int AnimToolCluster(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        int k,
        uint32_t* argbs
) {

    Context ctx{
            .index_of_frame = index_of_frame,
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .k = k,
            .points = new std::vector<std::array<float , 4>>(),
            .argbs = argbs
    };

    defer(delete ctx.points);

    check(DecRun(image_path, &ctx, kRunCallback));

    return 1;
}