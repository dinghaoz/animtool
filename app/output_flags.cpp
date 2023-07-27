//
// Created by Dinghao Zeng on 2023/7/26.
//

#include "output_flags.h"
#include "cli.h"

void CmdAddOutputFlags(cli::Cmd* cmd) {

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
            .short_aliases = {'v'},
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