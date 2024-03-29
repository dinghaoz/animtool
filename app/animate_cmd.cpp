//
// Created by Dinghao Zeng on 2023/2/22.
//

#include "animate_cmd.h"
#include "cli.h"
#include "output_flags.h"
#include "core/animate.h"

#include "core/logger.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    if (!AnimToolAnimate(
            cmd->args,
            cmd->n_args,
            cmd->GetStr("background"),
            cmd->GetInt("background_blur_radius"),
            cmd->GetInt("width"),
            cmd->GetInt("height"),
            cmd->GetInt("duration"),
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


void CmdAnimateInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd{
            .name = "animate",
            .desc = "Combine multiple image files into an animated image.",
            .usage = "[command options] IMAGE_FILE_PATH IMAGE_FILE_PATH ...",
            .examples = {
                    "animtool animate -o /path/to/output/file -d 67 /path/to/input/file1.png /path/to/input/file2.png /path/to/input/file3.png",
            },
            .n_args = 0,
            .context = nullptr,
            .action = CmdAction
    };


    cmd->AddFlag(cli::Flag{
            .name = "background",
            .short_aliases = {'b'},
            .desc = "background. format is type:content. e.g file:path/to/the/file, color:0xFF0000FF, frame:0",
            .type = cli::FLAG_STR,
            .required = 0,
            .multiple = 0,
            .default_value = { .str_value = "color:0x000000FF" }
    });

    cmd->AddFlag(cli::Flag{
            .name = "background_blur_radius",
            .short_aliases = {'c'},
            .desc = "background blur radius",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "width",
            .short_aliases = {'w'},
            .desc = "Canvas width",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "height",
            .short_aliases = {'h'},
            .desc = "Canvas height",
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

    cmd->AddFlag(cli::Flag{
            .name = "duration",
            .short_aliases = {'d'},
            .desc = "duration for each frame",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

//    cmd->AddFlag(cli::Flag{
//            .name = "loop_count",
//            .aliases = {'L'},
//            .desc = "if >= 0, force loop count the value, otherwise respect the one in input file.",
//            .type = cli::FLAG_INT,
//            .required = 0,
//            .multiple = 0,
//            .default_value = { .int_value = -1 }
//    });



    CmdAddOutputFlags(cmd);
}


