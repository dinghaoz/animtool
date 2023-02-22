//
// Created by Dinghao Zeng on 2023/1/16.
//

#include "gifrun.h"
#include "rawgif.h"

#include "webp/encode.h"
#include "webp/mux.h"
#include "imageio/gifdec.h"
#include "check.h"
#include "logger.h"
#include "check_gif.h"

#include "utils/defer.h"

#ifdef HAVE_CONFIG_H
#include "webp/config.h"
#endif


#ifdef WEBP_HAVE_GIF
#include "gif_lib.h"

#include "gifcompat.h"

#define GIF_TRANSPARENT_MASK  0x01
#define GIF_DISPOSE_MASK      0x07
#define GIF_DISPOSE_SHIFT     2

static int GIFReadRawGraphicsExtension(const GifByteType* const buf, int* const duration_ten_ms,
                                    int* const dispose,
                                    int* const transparent_index) {
    const int flags = buf[1];
    const int dispose_raw = (flags >> GIF_DISPOSE_SHIFT) & GIF_DISPOSE_MASK;
    const int duration_raw = buf[2] | (buf[3] << 8);  // In 10 ms units.
    if (buf[0] != 4) return 0;
    *duration_ten_ms = duration_raw;  // Duration is in 1 ms units.
    *dispose = dispose_raw;

    *transparent_index =
            (flags & GIF_TRANSPARENT_MASK) ? buf[4] : GIF_INDEX_INVALID;
    return 1;
}

static GIFDisposeMethod GIFDisposeMethodFromRaw(int dispose_raw) {
    switch (dispose_raw) {
        case DISPOSE_PREVIOUS:
            return GIF_DISPOSE_RESTORE_PREVIOUS;
        case DISPOSE_BACKGROUND:
            return GIF_DISPOSE_BACKGROUND;
        case DISPOSE_DO_NOT:
        case DISPOSAL_UNSPECIFIED:
        default:
            return GIF_DISPOSE_NONE;
    }
}

int GIFDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback) {
    logger::d("GIF Decode via giflib(%d.%d.%d)", GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE);

    GifFileType* gif = NULL;
    int frame_duration_ten_ms = 0;
    int frame_timestamp_ms = 0;
    int orig_dispose = DISPOSAL_UNSPECIFIED;

    WebPPicture frame;                // Frame rectangle only (not disposed).
    WebPPicture curr_canvas;          // Not disposed.
    WebPPicture prev_canvas;          // Disposed.

    int frame_number = 0;     // Whether we are processing the first frame.
    int done;

    int keep_metadata = METADATA_XMP;  // ICC not output by default.
    WebPData icc_data;
    int stored_icc = 0;         // Whether we have already stored an ICC profile.
    WebPData xmp_data;
    int stored_xmp = 0;         // Whether we have already stored an XMP profile.
    int loop_count = 0;         // default: infinite
    int stored_loop_count = 0;  // Whether we have found an explicit loop count.
    int loop_compatibility = 0;

    int transparent_index = GIF_INDEX_INVALID;


    check(WebPPictureInit(&frame));
    defer(WebPPictureFree(&frame));

    check(WebPPictureInit(&curr_canvas));
    defer(WebPPictureFree(&curr_canvas));

    check(WebPPictureInit(&prev_canvas));
    defer(WebPPictureFree(&prev_canvas));

    WebPDataInit(&icc_data);
    defer(WebPDataClear(&icc_data));

    WebPDataInit(&xmp_data);
    defer(WebPDataClear(&xmp_data));

    loop_compatibility = 1;

    // Start the decoder object
    int gif_error = 0;
    gif = DGifOpenFileName(file_path, &gif_error);
    if (gif == 0) {
        log_gif_error("DGifOpenFileName", gif_error);
        return 0;
    }

    defer( 
        int gif_error = 0;
        if (!DGifCloseFile(gif, &gif_error)) {
            log_gif_error("DGifCloseFile", gif_error);
        }
    );


    AnimInfo anim_info {};
    RawGifGlobalInfo raw_gif_global {};

    // Loop over GIF images
    done = 0;
    do {
        GifRecordType type;
        check_gif(DGifGetRecordType(gif, &type), gif);

        switch (type) {
            case IMAGE_DESC_RECORD_TYPE: {
                GifImageDesc* const image_desc = &gif->Image;

                check_gif(DGifGetImageDesc(gif), gif);

                if (frame_number == 0) {
                    logger::d("Canvas screen: %d x %d", gif->SWidth, gif->SHeight);

                    // Fix some broken GIF global headers that report
                    // 0 x 0 screen dimension.
                    if (gif->SWidth == 0 || gif->SHeight == 0) {
                        image_desc->Left = 0;
                        image_desc->Top = 0;
                        gif->SWidth = image_desc->Width;
                        gif->SHeight = image_desc->Height;
                        check(gif->SWidth > 0 && gif->SHeight > 0);

                        logger::w("broken GIF. Fixed canvas screen dimension to: %d x %d",
                               gif->SWidth, gif->SHeight);
                    }
                    // Allocate current buffer.
                    frame.width = gif->SWidth;
                    frame.height = gif->SHeight;
                    frame.use_argb = 1;
                    check(WebPPictureAlloc(&frame));

                    GIFClearPic(&frame, NULL);
                    check(WebPPictureCopy(&frame, &curr_canvas));
                    check(WebPPictureCopy(&frame, &prev_canvas));

                    logger::d("GIFGetBackgroundColor tidx=%d", transparent_index);
                    uint32_t bgcolor = 0;
                    // Background color.
                    GIFGetBackgroundColor(gif->SColorMap, gif->SBackGroundColor,
                                          transparent_index,
                                          &bgcolor);

                    raw_gif_global.color_res = gif->SColorResolution;
                    raw_gif_global.bgcolor_index = gif->SBackGroundColor;
                    raw_gif_global.color_map = gif->SColorMap;

                    anim_info.canvas_width = gif->SWidth;
                    anim_info.canvas_height = gif->SHeight;
                    anim_info.bgcolor = bgcolor;
                    anim_info.raw_gif = &raw_gif_global;

                    // Initialize encoder.
                    int stop = 0;
                    check(callback.on_start(ctx, &anim_info, &stop));

                    if (stop) { return 1;}
                }

                // Some even more broken GIF can have sub-rect with zero width/height.
                if (image_desc->Width == 0 || image_desc->Height == 0) {
                    image_desc->Width = gif->SWidth;
                    image_desc->Height = gif->SHeight;
                }

                GIFFrameRect gif_rect{};
                logger::d("before GIFReadFrame trans=%d", transparent_index);
                check_gif(GIFReadFrame(gif, transparent_index, &gif_rect, &frame), gif);
                logger::d("after GIFReadFrame rect=[%d:%d:%d:%d]", gif_rect.x_offset, gif_rect.y_offset, gif_rect.width, gif_rect.height);

                // before blending, save the current canvas into prev_canvas
                GIFCopyPixels(&curr_canvas, &prev_canvas);

                // Blend frame rectangle with previous canvas to compose full canvas.
                // Note that 'curr_canvas' is same as 'prev_canvas' at this point.
                GIFBlendFrames(&frame, &gif_rect, &curr_canvas);

                // Update timestamp (for next frame).

                RawGifLocalInfo raw_gif_local {
                    .color_map = image_desc->ColorMap,
                    .interlace = image_desc->Interlace,
                    .transparent_index = transparent_index,
                    .left = gif_rect.x_offset,
                    .top = gif_rect.y_offset,
                    .width = gif_rect.width,
                    .height = gif_rect.height,
                    .dispose_method = orig_dispose
                };

                AnimFrame anim_frame{};
                AnimFrameInitWithPic(&anim_frame, &curr_canvas);
                anim_frame.raw_gif = &raw_gif_local;

                int stop = 0;
                check(callback.on_frame(ctx, &anim_frame, frame_timestamp_ms, frame_timestamp_ms + frame_duration_ten_ms * 10, &stop));
                ++frame_number;
                frame_timestamp_ms += frame_duration_ten_ms * 10;

                // Update canvases.
                GIFDisposeFrame(GIFDisposeMethodFromRaw(orig_dispose), &gif_rect, &prev_canvas, &curr_canvas);

                // In GIF, graphic control extensions are optional for a frame, so we
                // may not get one before reading the next frame. To handle this case,
                // we reset frame properties to reasonable defaults for the next frame.
                orig_dispose = DISPOSAL_UNSPECIFIED;
                frame_duration_ten_ms = 0;
                transparent_index = GIF_INDEX_INVALID;

                if (stop) {
                    done = 1;
                }
                break;
            }
            case EXTENSION_RECORD_TYPE: {
                int extension;
                GifByteType* data = NULL;
                check_gif(DGifGetExtension(gif, &extension, &data), gif);

                if (data == NULL) continue;

                switch (extension) {
                    case COMMENT_EXT_FUNC_CODE: {
                        break;  // Do nothing for now.
                    }
                    case GRAPHICS_EXT_FUNC_CODE: {
                        check_gif(GIFReadRawGraphicsExtension(data, &frame_duration_ten_ms, &orig_dispose,
                                                      &transparent_index), gif);
                        logger::d("GIFReadGraphicsExtension disp=%d tidx=%d dur=%d", orig_dispose, transparent_index, frame_duration_ten_ms);
                        break;
                    }
                    case PLAINTEXT_EXT_FUNC_CODE: {
                        break;
                    }
                    case APPLICATION_EXT_FUNC_CODE: {
                        if (data[0] != 11) break;    // Chunk is too short
                        if (!memcmp(data + 1, "NETSCAPE2.0", 11) ||
                            !memcmp(data + 1, "ANIMEXTS1.0", 11)) {
                            check(GIFReadLoopCount(gif, &data, &loop_count));

                            logger::d("Loop count: %d", loop_count);

                            stored_loop_count = loop_compatibility ? (loop_count != 0) : 1;
                        } else {  // An extension containing metadata.
                            // We only store the first encountered chunk of each type, and
                            // only if requested by the user.
                            const int is_xmp = (keep_metadata & METADATA_XMP) &&
                                               !stored_xmp &&
                                               !memcmp(data + 1, "XMP DataXMP", 11);
                            const int is_icc = (keep_metadata & METADATA_ICC) &&
                                               !stored_icc &&
                                               !memcmp(data + 1, "ICCRGBG1012", 11);
                            if (is_xmp || is_icc) {
                                check(GIFReadMetadata(gif, &data,
                                                     is_xmp ? &xmp_data : &icc_data));

                                if (is_icc) {
                                    stored_icc = 1;
                                } else if (is_xmp) {
                                    stored_xmp = 1;
                                }
                            }
                        }
                        break;
                    }
                    default: {
                        break;  // skip
                    }
                }
                while (data != NULL) {
                    check_gif(DGifGetExtensionNext(gif, &data), gif);
                }
                break;
            }
            case TERMINATE_RECORD_TYPE: {
                done = 1;
                break;
            }
            default: {
                logger::d("Skipping over unknown record type %d", type);

                break;
            }
        }
    } while (!done);

    if (!loop_compatibility) {
        if (!stored_loop_count) {
            // if no loop-count element is seen, the default is '1' (loop-once)
            // and we need to signal it explicitly in WebP. Note however that
            // in case there's a single frame, we still don't need to store it.
            if (frame_number > 1) {
                stored_loop_count = 1;
                loop_count = 1;
            }
        } else if (loop_count > 0 && loop_count < 65535) {
            // adapt GIF's semantic to WebP's (except in the infinite-loop case)
            loop_count += 1;
        }
    }

    anim_info.has_loop_count = stored_loop_count;
    anim_info.loop_count = loop_count;
    check(callback.on_end(ctx, &anim_info));

    return 1;
}

#else

int GIFDecRun(const char* file_path, void* ctx, AnimDecRunCallback callback) {
    notreached("GIF support not compiled. Please install the libgif-dev package before building.");
    return 0;
}

#endif // #ifdef WEBP_HAVE_GIF

