#include "filefmt.h"

#include "webp/mux_types.h" // WebPData
#include "webp/decode.h" // WebPGetInfo


#include "gif_lib.h"


int IsWebP(const WebPData* const webp_data) {
    return (WebPGetInfo(webp_data->bytes, webp_data->size, nullptr, nullptr) != 0);
}


int IsGIF(const WebPData* const data) {
    return data->size > GIF_STAMP_LEN &&
           (!memcmp(GIF_STAMP, data->bytes, GIF_STAMP_LEN) ||
            !memcmp(GIF87_STAMP, data->bytes, GIF_STAMP_LEN) ||
            !memcmp(GIF89_STAMP, data->bytes, GIF_STAMP_LEN));
}
