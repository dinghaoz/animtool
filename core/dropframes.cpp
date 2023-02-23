//
// Created by Dinghao Zeng on 2023/1/13.
//

#include "dropframes.h"

#include "animrun.h"
#include "webprun.h"
#include "gifrun.h"
#include "imgrun.h"
#include "filefmt.h"
#include "animenc.h"

#include "webp/encode.h" // WebPPicture
#include "webp/mux_types.h" // WebPData
#include "../imageio/imageio_util.h" // ImgIoUtilReadFile

#include "check.h"
#include "logger.h"
#include "utils/defer.h"

#include <cmath>
#include <cstring>


static int RelToAbs(FrameTransformRectRel* rel, int width, int height, FrameTransformRectAbs* abs) {
    if (rel->ratio_x == 0 || rel->ratio_y == 0) {
        abs->left = abs->top = abs->width = abs->height = 0;
    } else if (rel->ratio_x * height < rel->ratio_y * width) {
        switch (rel->gravities) {
            case FRG_CENTER:
                abs->width = static_cast<int>(round(static_cast<double>(height) * rel->ratio_x / rel->ratio_y));
                abs->height = height;
                abs->left = static_cast<int>(round((static_cast<double>(width) - abs->width) / 2));
                abs->top = 0;
                break;
            default:
                notreached("Unknown gravity %d", rel->gravities);
        }
    } else {
        switch (rel->gravities) {
            case FRG_CENTER:
                abs->width = width;
                abs->height = static_cast<int>(round(static_cast<double>(width) * rel->ratio_y / rel->ratio_x));
                abs->left = 0;
                abs->top = static_cast<int>(round((static_cast<double>(height) - abs->height) / 2));
                break;
            default:
                notreached("Unknown gravity %d", rel->gravities);
        }

    }

    return 1;
}


static int SrcToAbs(FrameTransformSrc* src, int width, int height, FrameTransformRectAbs* abs) {
    switch (src->type) {
        case FTRT_ABS:
            memcpy(abs, &src->abs, sizeof(*abs));
            break;

        case FTRT_REL:
            check(RelToAbs(&src->rel, width, height, abs));
            break;
    }

    return 1;
}


typedef struct NormalizedFrameTransform {
    FrameTransformRectAbs src;
    int n_dsts;
    FrameTransformDst dsts[MAX_N_TRANSFORM_DSTS];
} NormalizedFrameTransform;


typedef struct DropFramesOptions {
    const char* const input;
    const char* const output;
    const char* const output_dir;
    int target_frame_rate;
    int target_total_duration;
    int loop_count;

    // global: WebPAnimEncoderOptions
    int minimize_size;
    int verbose;

    // per-frame: WebPConfig
    int lossless;
    float quality;
    int method;
    int pass;

    int n_transforms;
    FrameTransform transforms[MAX_N_TRANSFORMS];
} DropFramesOptions;


struct EncodersGroup {
    int n_encoders;
    AnimEncoder* encoders[MAX_N_TRANSFORM_DSTS];
};

struct DropFramesContext {
    DropFramesOptions options;

    EncodersGroup encoders_groups[MAX_N_TRANSFORMS];

    int out_start_ts;

    int in_total_duration_so_far;
    int in_frame_count;
    int out_frame_count;

    NormalizedFrameTransform transforms[MAX_N_TRANSFORMS];

    int OnDecodeStart(const AnimInfo* info, int* stop) {
        auto in_canvas_width = info->canvas_width;
        auto in_canvas_height = info->canvas_height;

        logger::d("OnDecodeStart %d:%d", in_canvas_width, in_canvas_height);

        for (int i=0; i<options.n_transforms; ++i) {

            check(SrcToAbs(&options.transforms[i].src, in_canvas_width, in_canvas_height, &transforms[i].src));
            transforms[i].n_dsts = options.transforms[i].n_dsts;

            auto& src = transforms[i].src;

            logger::d("Normalized crop %d:%d:%d:%d", src.left, src.top, src.width, src.height);

            for (int j=0; j<transforms[i].n_dsts; ++j) {

                auto out_canvas_width = in_canvas_width;
                auto out_canvas_height = in_canvas_height;

                if (src.width > 0 && src.height > 0) {
                    checkf(src.left >= 0 && src.left <= in_canvas_width, "Invalid crop rectangle");
                    checkf(src.top >= 0 && src.top <= in_canvas_height, "Invalid crop rectangle");
                    checkf(src.left + src.width <= in_canvas_width, "Invalid crop rectangle");
                    checkf(src.top + src.height <= in_canvas_height, "Invalid crop rectangle");

                    out_canvas_width = src.width;
                    out_canvas_height = src.height;
                }

                auto& dst = options.transforms[i].dsts[j];

                if (dst.width > 0 && dst.height <= 0) {
                    transforms[i].dsts[j].width = dst.width;
                    transforms[i].dsts[j].height = static_cast<int>(round(static_cast<double>(out_canvas_height) * dst.width / out_canvas_width));
                } else if (dst.width <= 0 && dst.height > 0) {
                    transforms[i].dsts[j].width = static_cast<int>(round(static_cast<double>(out_canvas_width) * dst.height / out_canvas_height));
                    transforms[i].dsts[j].height = dst.height;
                } else {
                    transforms[i].dsts[j].width = dst.width;
                    transforms[i].dsts[j].height = dst.height;
                }

                if (transforms[i].dsts[j].width > 0 && transforms[i].dsts[j].height > 0) {
                    out_canvas_width = transforms[i].dsts[j].width;
                    out_canvas_height = transforms[i].dsts[j].height;
                }

                logger::d("AnimEncoderNew %d:%d", out_canvas_width, out_canvas_height);

                AnimEncoderOptions encoder_options {
                    .verbose = options.verbose,
                    .minimize_size = options.minimize_size,
                    .bgcolor = info->bgcolor
                };


                auto encoder = AnimEncoderNew(
                        dst.format,
                        out_canvas_width,
                        out_canvas_height,
                        &encoder_options
                        );
                checkf(encoder, "Failed to create AnimEncoderNew");

                encoders_groups[i].encoders[j] = encoder;
            }
        }

        return 1;
    }

    static int ShouldKeepTheFrame(int in_start_ts, int in_end_ts, int out_start_ts) {
        if (in_start_ts <= out_start_ts) {
            if (in_start_ts == in_end_ts) { // 0 duration frame is not legal but we allow it anyway
                return 1;
            } else {
                return out_start_ts < in_end_ts;
            }
        } else {
            return 0;
        }
    }

    int OnDecodeFrame(const AnimFrame* frame, int in_start_ts, int in_end_ts, int* stop) {
        logger::d("OnDecodeFrame ts = %d:%d", in_start_ts, in_end_ts);

        ++in_frame_count;

        require(in_start_ts >= 0);
        require(in_end_ts >= in_start_ts);
        if (in_end_ts == in_start_ts) {
            logger::d("0 duration detected. [%d:%d]", in_start_ts, in_end_ts);
        }

        require(in_total_duration_so_far == in_start_ts);

        int target_duration = 0;
        if (options.target_frame_rate > 0) {
            target_duration = static_cast<int>(round(1000.0 / options.target_frame_rate));
        }

        if (options.target_total_duration > 0 && in_end_ts > options.target_total_duration) {
            in_end_ts = options.target_total_duration;
            *stop = 1;
        }

        if (ShouldKeepTheFrame(in_start_ts, in_end_ts, out_start_ts)) { // never drop the first frame
            int out_end_ts = in_end_ts;

            if (target_duration > 0) {
                auto duration_left = in_end_ts - out_start_ts;
                auto n = duration_left / target_duration;
                if (target_duration * n < duration_left) {
                    n += 1; // ceiling`
                }

                out_end_ts = out_start_ts + target_duration * n;
            }

            require(out_end_ts >= 0);

            WebPPicture decoded_frame;
            check(WebPPictureInit(&decoded_frame));
            defer(WebPPictureFree(&decoded_frame));

            check(AnimFrameExportToPic(frame, &decoded_frame));

            logger::d("frame to be added %d", out_start_ts);

            AnimFrameOptions frame_options {
                    .lossless = options.lossless,
                    .quality = options.quality,
                    .method = options.method,
                    .pass = options.pass
            };

            check(TransformFrameForAllDsts(&decoded_frame, out_start_ts, out_end_ts, &frame_options));

            ++out_frame_count;

            out_start_ts = out_end_ts;
        } else {
            // drop the frame
            logger::d("frame dropped");
        }

        in_total_duration_so_far = in_end_ts;

        if (*stop) {
            logger::d("total_duration reached, bail.");
        }

        return 1;
    }

    class CoWPic {
    public:
        explicit CoWPic(WebPPicture* foreign): _local(), _foreign(foreign), _current(foreign) {}

        CoWPic(const CoWPic&) = delete;
        CoWPic& operator=(CoWPic&) = delete;

        int Crop(int left, int top, int width, int height) {
            check(CopyIfNeeded());
            check(WebPPictureCrop(&_local, left, top, width, height));
            return 1;
        }

        int Rescale(int width, int height) {
            check(CopyIfNeeded());
            check(WebPPictureRescale(&_local, width, height));
            return 1;
        }

        ~CoWPic() {
            if (_current == &_local) {
                WebPPictureFree(&_local);
            }
        }

        [[nodiscard]] WebPPicture* Get() const {
            return _current;
        }

    private:
        int CopyIfNeeded() {
            if (_current == &_local)
                return 1;

            int done = 0;
            check(WebPPictureInit(&_local));
            defer( if (!done) WebPPictureFree(&_local));

            check(WebPPictureCopy(_foreign, &_local));
            _current = &_local;
            done = 1;

            return 1;
        }

        WebPPicture _local;
        WebPPicture* _foreign;

        WebPPicture* _current;
    };

    int TransformFrameForAllDsts(WebPPicture* decoded_frame, int start_ts, int end_ts, const AnimFrameOptions* frame_options) {
        for (int i=0; i<options.n_transforms; ++i) {
            auto& transform = transforms[i];
            auto& src = transform.src;

            CoWPic cropped(decoded_frame);

            if (src.width > 0 && src.height > 0) {
                logger::d("Crop %d:%d:%d:%d", src.left, src.top, src.width, src.height);
                check(cropped.Crop(src.left, src.top, src.width, src.height));
            }


            for (int j=0; j < transform.n_dsts; ++j) {
                auto dst = transform.dsts[j];

                CoWPic rescaled(cropped.Get());

                if (dst.width > 0 && dst.height > 0) {
                    logger::d("Rescale %d:%d", dst.width, dst.height);
                    check(rescaled.Rescale(dst.width, dst.height));
                }

                // we always pass in out_start_ts instead of out_end_ts
                // out_start_ts never exceeds the total duration of the animated image.
                check(AnimEncoderAddFrame(encoders_groups[i].encoders[j], rescaled.Get(), start_ts, end_ts, frame_options));
            }
        }

        return 1;
    }

    void DeleteAllEncoders() {
        for (int i=0; i<options.n_transforms; ++i) {
            auto& transform = transforms[i];

            for (int j=0; j<transform.n_dsts; ++j) {
                AnimEncoderDelete(encoders_groups[i].encoders[j]);
            }
        }
    }

    static int min(int l, int r) {
        return (l > r) ? r : l;
    }

    struct PathBuilder {
        char buf[10240] = {};
        char* cur = buf;

        void AddStr(const char* start, const char* end) {
            int buf_len = sizeof(buf) - (cur - buf);
            int copy_len = min(buf_len- 1, end - start);

            memcpy(cur, start, copy_len);
            cur += copy_len;
            *cur = 0;
        }

        void AddStr(const char* start) {
            AddStr(start, start + strlen(start));
        }

        void AddSuffix(const char* sep, int t, int d) {
            char suffix[128] = {};
            snprintf(suffix, sizeof(suffix), "%st%dd%d", sep, t, d);
            AddStr(suffix);
        }

        void BuildWithPath(const char* src, int t, int d, const char* target_ext) {
            buf[0] = 0;
            cur = buf;

            const char *slash = strrchr(src, '/');
            const char *dot = strrchr(src, '.');

            const char *name_start = src;
            if (slash) {
                name_start = slash + 1;
            }

            const char *name_end = src + strlen(src);
            const char *ext_start = nullptr;
            if (dot && dot > name_start) {
                name_end = dot;
                ext_start = dot;
            }

            AddStr(src, name_start); // dir
            AddStr(name_start, name_end); // name

            const char *sep = (name_start == name_end) ? "" : "_";
            AddSuffix(sep, t, d);

            if (target_ext) {
                AddStr(target_ext);
            } else if (ext_start) {
                AddStr(ext_start);
            }
        }
    };


    int OnDecodeEnd(const AnimInfo* anim_info) {
        defer(DeleteAllEncoders());

        for (int i=0; i<options.n_transforms; ++i) {
            auto transform = options.transforms[i];

            for (int j=0; j<transform.n_dsts; ++j) {

                auto file_ext = AnimEncoderGetFileExt(encoders_groups[i].encoders[j]);

                PathBuilder pb;

                if (options.output_dir) {
                    pb.AddStr(options.output_dir);

                    if (options.output_dir[strlen(options.output_dir)-1] != '/') {
                        pb.AddStr("/");
                    }

                    const char* file_name = transform.dsts[j].file_name;
                    if (strlen(file_name) > 0) {
                        pb.AddStr(file_name);
                    } else {
                        pb.AddSuffix("", i, j);
                        pb.AddStr(file_ext);
                    }

                } else if (options.output) {
                    if (i == 0 && j == 0) {
                        pb.AddStr(options.output);
                    } else {
                        pb.BuildWithPath(options.output, i, j, nullptr);
                    }
                } else {
                    // fine, we fallback to input
                    pb.BuildWithPath(options.input, i, j, file_ext);
                }


                int loop_count = 0;

                if (options.loop_count >= 0) {
                    if (options.loop_count > 0) {
                        loop_count = options.loop_count;
                    }
                } else {
                    if (anim_info->has_loop_count && anim_info->loop_count > 0) {
                        loop_count = options.loop_count;
                    }
                }

                logger::d("AnimEncoderExport src_duration=%d out_start=%d, %s", in_total_duration_so_far, out_start_ts, pb.buf);
                // usually out_start_ts will not larger than in_total_duration_so_far, except if the duration sequence is like this
                // #0 100
                // #1 0
                // #2 0
                // #3 0
                check(AnimEncoderExport(encoders_groups[i].encoders[j], (out_start_ts > in_total_duration_so_far) ? out_start_ts : in_total_duration_so_far, loop_count, pb.buf));
            }
        }

        return 1;
    }
};


static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    *stop = 0;
    auto thiz = reinterpret_cast<DropFramesContext*>(ctx);
    return thiz->OnDecodeStart(info, stop);
}

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    *stop = 0;
    auto thiz = reinterpret_cast<DropFramesContext*>(ctx);
    return thiz->OnDecodeFrame(frame, start_ts, end_ts, stop);
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    auto thiz = reinterpret_cast<DropFramesContext*>(ctx);
    return thiz->OnDecodeEnd(anim_info);
}

static AnimDecRunCallback kDropFramesCallback {
    .on_start = OnStart,
    .on_frame = OnFrame,
    .on_end = OnEnd
};

static const char* GetGravityStr(int gravities) {
    switch (gravities) {
        case FRG_CENTER:
            return "center";
        default:
            return "unknown";
    }
}

int AnimToolDropFrames(
    const char* const input,
    const char* const output,
    const char* const output_dir,
    int target_frame_rate,
    int target_total_duration,
    int loop_count,

    // global: WebPAnimEncoderOptions
    int minimize_size,
    int verbose,

    // per-frame: WebPConfig
    int lossless,
    float quality,
    int method,
    int pass,

    int n_transforms,
    const FrameTransform transforms[MAX_N_TRANSFORMS]
) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    logger::i("Start AnimToolDropFrames");
    logger::i("    input: %s", input);
    logger::i("    output: %s", output);
    logger::i("    output_dir: %s", output_dir);
    logger::i("    frame_rate: %d", target_frame_rate);
    logger::i("    total_duration: %d", target_total_duration);

    logger::i("    minimize_size: %d", minimize_size);

    logger::i("    lossless: %d", lossless);
    logger::i("    quality: %f", quality);
    logger::i("    method: %d", method);
    logger::i("    pass: %d", pass);

    logger::i("    n_transforms: %d", n_transforms);
    for (int i=0; i<n_transforms; ++i) {
        auto& t = transforms[i];
        switch (t.src.type) {
            case FTRT_ABS:
                logger::i("      ABS %d:%d:%d:%d -> %d", t.src.abs.left, t.src.abs.top, t.src.abs.width, t.src.abs.height, t.n_dsts);
                break;

            case FTRT_REL:
                logger::i("      REL %s %d:%d -> %d", GetGravityStr(t.src.rel.gravities), t.src.rel.ratio_x, t.src.rel.ratio_y, t.n_dsts);
                break;
        }

        for (int j=0; j<t.n_dsts; ++j) {
            auto& dst = t.dsts[j];
            logger::i("        %d:%d @%s #%s", dst.width, dst.height, dst.file_name, dst.format);
        }
    }

    DropFramesOptions options {
        .input = input,
        .output = output,
        .output_dir = output_dir,
        .target_frame_rate = target_frame_rate,
        .target_total_duration = target_total_duration,
        .loop_count = loop_count,
        .minimize_size = minimize_size,
        .verbose = verbose,
        .lossless = lossless,
        .quality = quality,
        .method = method,
        .pass = pass,
        .n_transforms = n_transforms,
    };

    memcpy(options.transforms, transforms, sizeof(transforms[0]) * n_transforms);

    DropFramesContext ctx{
        .options = options
    };

    checkf(ImgIoUtilReadFile(input, &webp_data.bytes, &webp_data.size), "The input file cannot be open %s", input);

    if (IsWebP(&webp_data)) {
        check(WebPDecRunWithData(&webp_data, &ctx, kDropFramesCallback));
    } else if (IsGIF(&webp_data)) {
        check(GIFDecRun(input, &ctx, kDropFramesCallback));
    } else {
        if (!ImgDecRunWithData(&webp_data, &ctx, kDropFramesCallback)) {
            auto last_dot = strrchr(input, '.');
            notreached("Failed. Please check your file type: `%s`", last_dot ? (last_dot + 1) : input);
        }
    }

    logger::i("End AnimToolDropFrames: %d->%d", ctx.in_frame_count, ctx.out_frame_count);
    return 1;
}

int AnimToolDropFramesLite(
    const char* const input,
    const char* const output,
    int target_frame_rate,
    int target_total_duration,
    int loop_count,

    int src_left,
    int src_top,
    int src_width, // 0 means don't crop
    int src_height, // 0 means don't crop
    int dst_width,
    int dst_height
) {
    FrameTransform transform {
        .src = {
            .abs = {
                .left = src_left,
                .top = src_top,
                .width = src_width,
                .height = src_height
            },
            .type = FTRT_ABS
        },
        .n_dsts = 1,
        .dsts = { {
            .width = dst_width,
            .height = dst_height,
            .file_name = {0},
            .format = {0}
        }}
    };

    return AnimToolDropFrames(
            input,
            output,
            nullptr,
            target_frame_rate,
            target_total_duration,
            loop_count,

            0, // minimize_size
            0, // verbose

            0, // lossless
            75.0f, // quality
            0, // method
            1, // pass

            1,
            &transform
    );
}

