#include "filefmt.h"

#include "webp/mux_types.h" // WebPData
#include "webp/decode.h" // WebPGetInfo


#ifdef WEBP_HAVE_GIF
#include "gif_lib.h"
#else
#define GIF_STAMP "GIFVER"          /* First chars in file - GIF stamp.  */
#define GIF_STAMP_LEN sizeof(GIF_STAMP) - 1
#define GIF_VERSION_POS 3           /* Version first character in stamp. */
#define GIF87_STAMP "GIF87a"        /* First chars in file - GIF stamp.  */
#define GIF89_STAMP "GIF89a"        /* First chars in file - GIF stamp.  */
#endif



int IsWebP(const WebPData* const webp_data) {
    return (WebPGetInfo(webp_data->bytes, webp_data->size, nullptr, nullptr) != 0);
}


int IsGIF(const WebPData* const data) {
    return data->size > GIF_STAMP_LEN &&
           (!memcmp(GIF_STAMP, data->bytes, GIF_STAMP_LEN) ||
            !memcmp(GIF87_STAMP, data->bytes, GIF_STAMP_LEN) ||
            !memcmp(GIF89_STAMP, data->bytes, GIF_STAMP_LEN));
}
