#include "cli.h"

#include "webp/encode.h"
#include "gif_lib.h"


int main(int argc, char *argv[]) {
    cli::App app {
        .name = "animtool",
        .desc = "A tool to process animated images."
    };

    auto a = WebPEncodeRGB;
    fprintf(stderr, "%p", a);

//    cli::Cmd dropframes {};
//    CmdDropFramesInit(&dropframes);
//    app.AddCmd(&dropframes);
//
//    cli::Cmd info {};
//    CmdInfoInit(&info);
//    app.AddCmd(&info);

    return app.Run(argc, argv);
}
