#include "cli.h"
#include "dropframes_cmd.h"
#include "info_cmd.h"

int main(int argc, char *argv[]) {
    cli::App app {
            .name = "animtool",
            .desc = "A tool to process animated images."
    };

    cli::Cmd dropframes {};
    CmdDropFramesInit(&dropframes);
    app.AddCmd(&dropframes);

    cli::Cmd info {};
    CmdInfoInit(&info);
    app.AddCmd(&info);

    return app.Run(argc, argv);
}
