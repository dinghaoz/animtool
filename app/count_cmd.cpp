//
// Created by Dinghao Zeng on 2023/8/7.
//

#include "count_cmd.h"
#include "cli.h"

#include "core/count.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto input = cmd->GetFirstArg();

    int count = 0;
    if (!AnimToolCountStr(
            input,
            cmd->GetInt("frame"),
            cmd->GetInt("origin_x"),
            cmd->GetInt("origin_y"),
            cmd->GetInt("width"),
            cmd->GetInt("height"),
            cmd->GetStr("predicate"),
            &count)) {
        return cli::ACTION_FAILED;
    }

    fprintf(stdout, "%d\n", count);

    return cli::ACTION_OK;
}

void CmdCountInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "count",
            .desc = "Count the pixel according to the given predicate.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool count /path/to/input/file -f index_of_frame -p 15<red<=100:alpha=0"
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
            .short_aliases = {'f'},
            .desc = "Index of the frame to count.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_x",
            .short_aliases = {'x'},
            .desc = "Rectangle origin x.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "origin_y",
            .short_aliases = {'y'},
            .desc = "Rectangle origin y.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "width",
            .short_aliases = {'w'},
            .desc = "Rectangle width.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = -1 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "height",
            .short_aliases = {'h'},
            .desc = "Rectangle height.",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = -1 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "predicate",
            .short_aliases = {'p'},
            .desc = "A predicate to specify when a pixel is counted.",
            .type = cli::FLAG_STR,
            .required = 0,
            .multiple = 1,
            .default_value = { .str_value = "15<red<=100:alpha=0" }
    });
}