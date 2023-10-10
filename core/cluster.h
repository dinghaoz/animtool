//
// Created by Dinghao Zeng on 2023/10/10.
//

#ifndef ANIMTOOL_CLUSTER_H
#define ANIMTOOL_CLUSTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int AnimToolCluster(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        int k,
        uint32_t* argbs,
        uint32_t* counts
);


#ifdef __cplusplus
}
#endif


#endif //ANIMTOOL_CLUSTER_H
