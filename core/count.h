//
// Created by Dinghao Zeng on 2023/8/7.
//

#ifndef ANIMTOOL_COUNT_H
#define ANIMTOOL_COUNT_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct CountPredicate {
    int larger;
    int larger_equal;
    int smaller;
    int smaller_equal;
    int equal;
} CountPredicate;

int AnimToolCount(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        CountPredicate red,
        CountPredicate green,
        CountPredicate blue,
        CountPredicate alpha,
        int* count
);
int AnimToolCountStr(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        const char* predicates,
        int* count
);

#ifdef __cplusplus
}
#endif

#endif //ANIMTOOL_COUNT_H
