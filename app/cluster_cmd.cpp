//
// Created by Dinghao Zeng on 2023/10/10.
//

#include "cluster_cmd.h"
#include "cli.h"

#include "core/cluster.h"
#include "core/cg.h"
#include "utils/defer.h"


static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    auto input = cmd->GetFirstArg();

    auto k = cmd->GetInt("number_of_clusters");
    auto argbs = new uint32_t[k];
    defer(delete[] argbs);
    auto counts = new uint32_t[k];
    defer(delete[] counts);

    if (!AnimToolCluster(
            input,
            cmd->GetInt("frame"),
            cmd->GetInt("origin_x"),
            cmd->GetInt("origin_y"),
            cmd->GetInt("width"),
            cmd->GetInt("height"),
            k,
            argbs,
            counts)) {
        return cli::ACTION_FAILED;
    }

    for (int i=0; i<k; ++i) {
        auto argb = argbs[i];
        auto clr = cg::Color::FromARGB(argb);
        fprintf(stdout, "#%02X%02X%02X%02X, %d\n", clr.r, clr.g, clr.b, clr.a, counts[i]);
    }

    return cli::ACTION_OK;
}

void CmdClusterInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd {
            .name = "cluster",
            .desc = "Cluster the image's color.",
            .usage = "[command options] IMAGE_FILE_PATH",
            .examples = {
                    "animtool cluster /path/to/input/file -f index_of_frame -k 2"
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
            .desc = "Index of the frame to count. -1 for all frames",
            .type = cli::FLAG_INT,
            .required = 0,
            .multiple = 0,
            .default_value = { .int_value = -1 }
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
            .name = "number_of_clusters",
            .short_aliases = {'k'},
            .desc = "k of k-means.",
            .type = cli::FLAG_INT,
            .required = 1,
            .multiple = 0,
    });
}
