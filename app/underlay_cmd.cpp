//
// Created by Dinghao Zeng on 2023/5/6.
//

#include "underlay_cmd.h"

#include "cli.h"
#include "output_flags.h"
#include "core/addlayer.h"
#include "core/logger.h"
#include "core/check.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    if (!AnimToolAddLayer(
            cmd->GetFirstArg(),
            cmd->GetStr("layer"),
            0,
            cmd->GetBool("center"),
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

void CmdUnderlayInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "underlay",
            .desc = "Draw an underlay to the input image file.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool overlay /path/to/input/file"
            },
            .n_args = 1,
            .args_desc = "Path of the image file. Supported file formats: WebP"
                         ", GIF"
                         ", JPEG"
                         ", PNG"
                         ", PNM (PGM, PPM, PAM)"
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
            .short_aliases = {'o'},
            .desc = "output image path",
            .type = cli::FLAG_STR,
            .required = 1,
            .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
            .name = "format",
            .short_aliases = {'f'},
            .desc = "output image format: webp or gif",
            .type = cli::FLAG_STR,
            .required = 0,
            .multiple = 0,
            .default_value = { .str_value = "webp" }
    });


    cmd->AddFlag(cli::Flag{
            .name = "layer",
            .aliases = {"underlay"},
            .desc = "underlay image path",
            .type = cli::FLAG_STR,
            .required = 1,
            .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
            .name = "center",
            .desc = "center the overlay",
            .type = cli::FLAG_BOOL,
            .required = 0,
            .multiple = 0,
            .default_value = { .bool_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_x",
            .short_aliases = {'x'},
            .desc = "Overlay origin x. Ignored if --center is set",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_y",
            .short_aliases = {'y'},
            .desc = "Overlay origin y. Ignored if --center is set",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    CmdAddOutputFlags(cmd);
}