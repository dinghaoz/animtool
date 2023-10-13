#include "cli.h"
#include "dropframes_cmd.h"
#include "animate_cmd.h"
#include "info_cmd.h"
#include "opacity_cmd.h"
#include "overlay_cmd.h"
#include "underlay_cmd.h"
#include "mask_cmd.h"
#include "count_cmd.h"
#include "cluster_cmd.h"
#include "blur_cmd.h"

int main(int argc, char *argv[]) {
    cli::App app {
        .name = "animtool",
        .desc = "A tool to process animated images."
    };

    cli::Cmd dropframes {};
    CmdDropFramesInit(&dropframes);
    app.AddCmd(&dropframes);

    cli::Cmd animate {};
    CmdAnimateInit(&animate);
    app.AddCmd(&animate);

    cli::Cmd info {};
    CmdInfoInit(&info);
    app.AddCmd(&info);


    cli::Cmd opacity {};
    CmdOpacityInit(&opacity);
    app.AddCmd(&opacity);

    cli::Cmd overlay {};
    CmdOverlayInit(&overlay);
    app.AddCmd(&overlay);

    cli::Cmd underlay {};
    CmdUnderlayInit(&underlay);
    app.AddCmd(&underlay);

    cli::Cmd mask {};
    CmdMaskInit(&mask);
    app.AddCmd(&mask);

    cli::Cmd count {};
    CmdCountInit(&count);
    app.AddCmd(&count);

    cli::Cmd cluster {};
    CmdClusterInit(&cluster);
    app.AddCmd(&cluster);

    cli::Cmd blur {};
    CmdBlurInit(&blur);
    app.AddCmd(&blur);

    return app.Run(argc, argv);
}
