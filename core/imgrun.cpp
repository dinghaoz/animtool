
#include "imgrun.h"

#include "../imageio/image_enc.h"
#include "../imageio/imageio_util.h"
#include "../imageio/image_dec.h"
#include "webp/encode.h"
#include "webp/demux.h"

#include "defer.h"
#include "check.h"

int ImgDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    check(ImgIoUtilReadFile(file_path, &webp_data.bytes, &webp_data.size));

    return ImgDecRunWithData(&webp_data, ctx, callback);
}

int ImgDecRunWithData(WebPData* webp_data, void* ctx, AnimDecRunCallback callback) {
    auto reader = WebPGuessImageReader(webp_data->bytes, webp_data->size);

    WebPPicture pic;
    check(WebPPictureInit(&pic));
    defer(WebPPictureFree(&pic));
    pic.use_argb = 1;
    check(reader(webp_data->bytes, webp_data->size, &pic, 1, NULL));

    int stop = 0;
    
    AnimInfo anim_info {
        .canvas_width = pic.width,
        .canvas_height = pic.height
    };
    check(callback.on_start(ctx, &anim_info, &stop));
    if (stop) return 1;

    AnimFrame frame;
    AnimFrameInitWithPic(&frame, &pic);
    check(callback.on_frame(ctx, &frame, 0, 0, &stop));

    check(callback.on_end(ctx, &anim_info));

    return 1;
}

