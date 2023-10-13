//
// Created by Dinghao Zeng on 2023/10/13.
//

#include "blur_cmd.h"
#include "cli.h"
#include "output_flags.h"
#include "core/blur.h"

#include "core/logger.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    if (!AnimToolBlur(
            cmd->GetFirstArg(),
            cmd->GetInt("frame"),
            cmd->GetInt("blur_radius"),
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

    return cli::ACTION_OK;;
}



void CmdBlurInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "blur",
            .desc = "Make an image blur.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool blur -o /path/to/output/file -c 67 -i 0 /path/to/input/file"
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
            .name = "frame",
            .short_aliases = {'i'},
            .desc = "Index of the frame to count. -1 for all frames",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = -1 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "blur_radius",
            .short_aliases = {'c'},
            .desc = "blur radius",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "output",
            .short_aliases = {'o'},
            .desc = "output image path",
            .type = cli::FLAG_STR,
            .required = 0,
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

    CmdAddOutputFlags(cmd);
}
