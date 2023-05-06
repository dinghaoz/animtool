//
// Created by Dinghao Zeng on 2023/5/6.
//

#include "overlay_cmd.h"

#include "cli.h"
#include "core/overlay.h"
#include "core/logger.h"
#include "core/check.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    if (!AnimToolOverlay(
            cmd->GetFirstArg(),
            cmd->GetStr("overlay"),
            cmd->GetInt("origin_x"),
            cmd->GetInt("origin_y"),
            cmd->GetStr("tint"),

            cmd->GetStr("output"),
            cmd->GetStr("format"),

            cmd->GetBool("minimize_size"),
            verbose,

            cmd->GetBool("lossless"),
            cmd->GetFloat("quality"),
            cmd->GetInt("method"),
            cmd->GetInt("pass")
    )) {
        return cli::ACTION_FAILED;
    }

    return cli::ACTION_OK;
}

void CmdOverlayInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "overlay",
            .desc = "Draw an overlay to the input image file.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool overlay /path/to/input/file"
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
            .name = "tint",
            .desc = "tint color in RGB like 0xFFF034.",
            .type = cli::FLAG_STR,
            .required = 0,
            .multiple = 0,
            .default_value = { .str_value = "0x00000000" }
    });


    cmd->AddFlag(cli::Flag{
            .name = "output",
            .aliases = {"o"},
            .desc = "output image path",
            .type = cli::FLAG_STR,
            .required = 1,
            .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
            .name = "format",
            .aliases = {"f"},
            .desc = "output image format: webp or gif",
            .type = cli::FLAG_STR,
            .required = 0,
            .multiple = 0,
            .default_value = { .str_value = "webp" }
    });


    cmd->AddFlag(cli::Flag{
            .name = "overlay",
            .desc = "overlay image path",
            .type = cli::FLAG_STR,
            .required = 1,
            .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_x",
            .aliases = {"x"},
            .desc = "Overlay origin x",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_y",
            .aliases = {"y"},
            .desc = "Overlay origin y",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "minimize_size",
            .desc = "If true, minimize the output size (slow). Implicitly disables key-frame insertion.",
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

    cmd->AddFlag(cli::Flag{
            .name = "lossless",
            .desc = "Lossless encoding.",
            .type = cli::FLAG_BOOL,
            .required = 0,
            .multiple = 0,
            .default_value = { .bool_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "quality",
            .desc = "between 0 and 100.",
            .type = cli::FLAG_FLOAT,
            .required = 0,
            .multiple = 0,
            .default_value = { .float_value = 75.0f }
    });

    cmd->AddFlag(cli::Flag{
            .name = "method",
            .desc = "quality/speed trade-off (0=fast, 6=slower-better).",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "pass",
            .desc = "number of entropy-analysis passes (in [1..10]).",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 1 }
    });
}