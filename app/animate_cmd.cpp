//
// Created by Dinghao Zeng on 2023/2/22.
//

#include "animate_cmd.h"
#include "cli.h"
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
            cmd->GetInt("duration"),
            cmd->GetInt("width"),
            cmd->GetInt("height"),
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
            .name = "width",
            .aliases = {"w"},
            .desc = "Canvas width",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "height",
            .aliases = {"h"},
            .desc = "Canvas height",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
            .name = "output",
            .aliases = {"o"},
            .desc = "output image path",
            .type = cli::FLAG_STR,
            .required = 0,
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
            .name = "duration",
            .aliases = {"d"},
            .desc = "duration for each frame",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = 0 }
    });

//    cmd->AddFlag(cli::Flag{
//            .name = "loop_count",
//            .aliases = {"L"},
//            .desc = "if >= 0, force loop count the value, otherwise respect the one in input file.",
//            .type = cli::FLAG_INT,
//            .required = 0,
//            .multiple = 0,
//            .default_value = { .int_value = -1 }
//    });

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


