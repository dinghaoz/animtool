//
// Created by Dinghao Zeng on 2023/4/27.
//

#ifndef ANIMTOOL_OPACITY_H
#define ANIMTOOL_OPACITY_H


#ifdef __cplusplus
extern "C" {
#endif


int AnimToolGetOpacity(
        const char*const image_path,
        int sample_divider,
        float* out_opacity
);


#ifdef __cplusplus
}
#endif


#endif //ANIMTOOL_OPACITY_H
