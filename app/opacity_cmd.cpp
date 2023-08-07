//
// Created by Dinghao Zeng on 2023/4/27.
//

#include "opacity_cmd.h"
#include "cli.h"
#include "core/opacity.h"
#include "core/logger.h"
#include "core/check.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {

    auto sample_divider = cmd->GetInt("sample_divider");

    auto input = cmd->GetFirstArg();

    float opacity = 0;
    if (!AnimToolGetOpacity(input, sample_divider, &opacity)) {
        return cli::ACTION_FAILED;
    }

    fprintf(stdout, "%.2f\n", opacity);

    return cli::ACTION_OK;
}

void CmdOpacityInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "opacity",
            .desc = "Get the average opacity for the image file.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool opacity /path/to/input/file"
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
            .name = "sample_divider",
            .short_aliases = {'O'},
            .desc = "sample divider.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });
}