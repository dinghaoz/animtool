#include "cli.h"

#include "core/animrun.h"
#include "core/webprun.h"
#include "core/gifrun.h"
#include "core/imgrun.h"
#include "core/filefmt.h"
#include "core/rawgif.h"

#ifdef HAVE_CONFIG_H
#include "webp/config.h"
#endif

#include "webp/mux_types.h" // WebPData
#include "../imageio/imageio_util.h" // ImgIoUtilReadFile


#ifdef WEBP_HAVE_GIF
#include "gif_lib.h"
#include "core/gifcompat.h"
#endif

#include "core/logger.h"
#include "core/check.h"
#include "utils/defer.h"

struct Context {
    int detail;
    int frame_count;
    int total_duration;
};

#define imginfo(fmt, ...) \
    fprintf(stdout, fmt"\n", ##__VA_ARGS__)

static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);
    *stop = !thiz->detail;

    imginfo("Canvas size %dx%d", info->canvas_width, info->canvas_height);
    imginfo("Background color %x", info->bgcolor);
    if (info->has_loop_count) {
        imginfo("Loop count %d", info->loop_count);
    }

#ifdef WEBP_HAVE_GIF
    if (info->raw_gif) {
        imginfo("[GIF]");
        imginfo("    res %d", info->raw_gif->color_res);
        imginfo("    bg index %d", info->raw_gif->bgcolor_index);
        if (info->raw_gif->color_map) {
            imginfo("    cmap(%d)", info->raw_gif->color_map->ColorCount);
        }
    }
#endif

    return 1;
}

#ifdef WEBP_HAVE_GIF
static const char* GetDisposalMethodName(int dispose_method) {
    switch (dispose_method) {
        case DISPOSAL_UNSPECIFIED: return "?(0)";
        case DISPOSE_DO_NOT: return "DONOT(1)";
        case DISPOSE_BACKGROUND: return "BG(2)";
        case DISPOSE_PREVIOUS: return "PREV(3)";
        default: return nullptr;
    }
}
#endif


static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    require(thiz->detail);
    *stop = !thiz->detail;

    cli::StrBuilder sb {};
#ifdef WEBP_HAVE_GIF
    if (frame->raw_gif) {
        sb.AddText(" [GIF]");
        if (frame->raw_gif->interlace) {
            sb.AddText(" interlaced");
        }

        if (frame->raw_gif->transparent_index >= 0) {
            sb.AddText(" trans=%d", frame->raw_gif->transparent_index);
        }

        if (frame->raw_gif->color_map) {
            sb.AddText(" cmap(%d)", frame->raw_gif->color_map->ColorCount);
        }

        sb.AddText(" [%d:%d:%d:%d]", frame->raw_gif->left, frame->raw_gif->top, frame->raw_gif->width, frame->raw_gif->height);
        sb.AddText(" disp=%s", GetDisposalMethodName(frame->raw_gif->dispose_method));
    }
#endif

    imginfo("image#%d duration=%d%s", thiz->frame_count, end_ts - start_ts, sb.buf);

    ++thiz->frame_count;
    thiz->total_duration = end_ts;

    return 1;
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    auto thiz = reinterpret_cast<Context*>(ctx);
    require(thiz->detail);

    imginfo("Total duration %d", thiz->total_duration);
    if (anim_info->has_loop_count) {
        imginfo("Loop count %d", anim_info->loop_count);
    }

    return 1;
}

static AnimDecRunCallback kDropFramesCallback {
    .on_start = OnStart,
    .on_frame = OnFrame,
    .on_end = OnEnd
};

static int PrintInfo(const char* input, int detail) {
    WebPData webp_data;
    WebPDataInit(&webp_data);
    defer(WebPDataClear(&webp_data));

    check(ImgIoUtilReadFile(input, &webp_data.bytes, &webp_data.size));

    Context ctx{
        .detail = detail
    };

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
    return 1;
}

static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    auto input = cmd->GetFirstArg();
    PrintInfo(input, cmd->GetBool("detail"));

    return cli::ACTION_OK;
}

void CmdInfoInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
        .name = "info",
        .desc = "Get meta info for an input image file.",
        .usage = "[command options] IMAGE_FILE_PATH",
        .examples = {
            "animtool info /path/to/input/file"
        },
        .n_args = 1,
        .args_desc = "Path of the image file. Supported file formats: WebP"
#ifdef WEBP_HAVE_GIF
        ", GIF"
#endif
#ifdef WEBP_HAVE_JPEG
         ", JPEG"
#endif
#ifdef WEBP_HAVE_PNG
         ", PNG"
#endif 
         ", PNM (PGM, PPM, PAM)"
#ifdef WEBP_HAVE_TIFF
         ", TIFF"
#endif
        ". A static image is treated as one frame animated image.",
        .context = nullptr,
        .action = CmdAction
    };

    cmd->AddFlag(cli::Flag{
        .name = "detail",
        .aliases = {"d"},
        .desc = "Print image detail for every frame.",
        .type = cli::FLAG_BOOL,
        .required = 0,
        .multiple = 0,
        .default_value = { .bool_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "verbose",
            .aliases = {"v"},
            .desc = "print detail log",
            .type = cli::FLAG_BOOL,
            .required = 0,
            .multiple = 0,
            .default_value = { .bool_value = 0 }
    });
}