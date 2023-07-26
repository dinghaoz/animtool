#include "cli.h"
#include "output_flags.h"
#include "core/dropframes.h"
#include "core/logger.h"


static const char* StrSearch(const char* pstart, const char* pend, const char* target) {
    const char* p = pstart;
    int target_len = strlen(target);
    while (p < pend) {
        if (!strncmp(target, p, target_len))
            break;
        ++p;
    }
    return p;
}

template<typename T>
static int ParseList(const char* pstart, const char* pend, const char* sep, T* values, int limit_n, int* pcount, int(*ElementParser)(const char* pstart, const char* pend, T* value)) {
    int i = 0;
    const char* itr = pstart;
    int lsep = strlen(sep);
    while (i < limit_n && itr <= pend) {
        const char* psep = StrSearch(itr, pend, sep);

        if (ElementParser(itr, psep, values+i)) {
            return -1;
        }
        ++i;

        itr = psep + lsep;
    }

    if (pcount) *pcount = i;

    return 0;
}

static int ParseInt(const char* pstart, const char* pend, int* value) {
    const char* p = pstart;
    int ret = 0;
    while (p < pend) {
        if ('0' <= *p && *p <= '9') {
            ret = ret * 10 + (*p-'0');
        } else {
            return -1;
        }

        ++p;
    }

    *value = ret;

    return 0;
}

static int ParseColonInts(const char* pstart, const char* pend, int* values, int limit_n, int* pcount) {
    return ParseList(pstart, pend, ":", values, limit_n, pcount, ParseInt);
}


static int ParseGravity(const char* pstart, const char* pend, int* value) {
    if (!strncmp("center", pstart, pend - pstart)) {
        *value = FRG_CENTER;
    } else {
        return -1;
    }

    return 0;
}

static int ParseBarGravities(const char* pstart, const char* pend, int* values, int limit_n, int* pcount) {
    return ParseList(pstart, pend, "|", values, limit_n, pcount, ParseGravity);
}

static int ParseDst(const char* pstart, const char* pend, FrameTransformDst* value) {
    auto at = StrSearch(pstart, pend, "@");
    auto pund = StrSearch(pstart, pend, "#");
    auto fn_start = at + 1;
    if (fn_start < pund) {
        auto fn_len = pund - fn_start;
        strncpy(value->file_name, fn_start, MAX_FILE_NAME_LENGTH > fn_len ? fn_len : MAX_FILE_NAME_LENGTH);
    } else {
        value->file_name[0] = 0;
    }

    auto fmt_start = pund + 1;
    if (fmt_start < pend) {
        auto fmt_len = pend - fmt_start;
        strncpy(value->format, fmt_start, MAX_FORMAT_LENGTH > fmt_len ? fmt_len : MAX_FORMAT_LENGTH);
    } else {
        value->format[0] = 0;
    }

    int resize[2] = {};
    if (ParseColonInts(pstart, (at < pund) ? at : pund, resize, sizeof(resize)/sizeof (resize[0]), nullptr)) {
        return -1;
    }

    value->width = resize[0];
    value->height = resize[1];

    return 0;
}

static int ParseSemiColonDsts(const char* pstart, const char* pend, FrameTransformDst* values, int limit_n, int* pcount) {
    return ParseList(pstart, pend, ";", values, limit_n, pcount, ParseDst);
}

static int ParseTransform(const char* pstart, FrameTransform* transform) {
    auto pend = pstart + strlen(pstart);

    auto arrow = StrSearch(pstart, pend, "->");
    if (arrow >= pend) return -1;

    auto comma = StrSearch(pstart, arrow, ",");
    if (comma < arrow) {
        transform->src.type = FTRT_REL;
        int gravities[2] = {};
        if (ParseBarGravities(pstart, comma, gravities, sizeof(gravities)/sizeof (gravities[0]),
                              nullptr)) {
            return -1;
        }
        transform->src.rel.gravities = gravities[0] | gravities[1];

        int ratio[2] = {};
        if (ParseColonInts(comma+1, arrow, ratio, sizeof(ratio)/sizeof (ratio[0]), nullptr)) {
            return -1;
        }
        transform->src.rel.ratio_x = ratio[0];
        transform->src.rel.ratio_y = ratio[1];

    } else {
        transform->src.type = FTRT_ABS;
        int crops[4] = {};
        if (ParseColonInts(pstart, arrow, crops, sizeof(crops)/sizeof (crops[0]), nullptr)) {
            return -1;
        }

        transform->src.abs.left = crops[0];
        transform->src.abs.top = crops[1];
        transform->src.abs.width = crops[2];
        transform->src.abs.height = crops[3];
    }

    int n_dsts = 0;
    if (arrow + 2 == pend) { // special case. at least one dst
        n_dsts = 1;
        transform->dsts[0].width = 0;
        transform->dsts[0].height = 0;
        transform->dsts[0].file_name[0] = 0;
    } else {
        if (ParseSemiColonDsts(arrow+2, pend, transform->dsts, MAX_N_TRANSFORM_DSTS, &n_dsts)) {
            return -1;
        }
    }

    transform->n_dsts = n_dsts;

    return 0;
}

static cli::ActionError CmdAction(void* context, const cli::CmdResult* cmd, cli::StrBuilder& error) {
    const char* transform_strs[MAX_N_FLAG_VALUES] = {};
    int n_transforms = 0;
    if (cmd->GetStrList("transform", transform_strs, &n_transforms)) {
        return cli::ACTION_WRONG_ARGS;
    }

    FrameTransform transforms[MAX_N_FLAG_VALUES] = {};
    for (int i=0; i<n_transforms; ++i) {
        if (ParseTransform(transform_strs[i], &transforms[i])) {
            error.AddText("Failed to parse transform `%s`", transform_strs[i]);
            return cli::ACTION_WRONG_ARGS;
        }
    }

    auto verbose = cmd->GetBool("verbose");
    if (verbose) {
        logger::level = logger::LOG_DEBUG;
    }

    if (!AnimToolDropFrames(
            cmd->GetFirstArg(),
            cmd->GetStr("output"),
            cmd->GetStr("output_dir"),
            cmd->GetInt("frame_rate"),
            cmd->GetInt("total_duration"),
            cmd->GetInt("loop_count"),

            cmd->GetBool("minimize_size"),
            verbose,

            cmd->GetBool("lossless"),
            cmd->GetFloat("quality"),
            cmd->GetInt("method"),
            cmd->GetInt("pass"),

            n_transforms,
            transforms
    )) {
        return cli::ACTION_FAILED;
    }

    return cli::ACTION_OK;;
}


void CmdDropFramesInit(cli::Cmd* cmd) {
    *cmd = cli::Cmd{
        .name = "dropframes",
        .desc = "Drop frame rate for an animated image along with crop and resize support.",
        .usage = "[command options] IMAGE_FILE_PATH",
        .examples = {
            "animtool dropframes -o /path/to/output/file -R 15 /path/to/input/file",
            "animtool dropframes -o /path/to/output/file -R 15 -D 10000 /path/to/input/file",
            "animtool dropframes -o /path/to/output/file -R 15 -D 10000 -T '->250:250' /path/to/input/file",
            "animtool dropframes -o /path/to/output/file -R 15 -D 10000 -T '0:100:500:500->' /path/to/input/file",
            "animtool dropframes -o /path/to/output/file -R 15 -D 10000 -T '0:100:500:500->250:250' /path/to/input/file",
            "animtool dropframes -d /path/to/output/dir -R 15 -D 10000 -T '0:100:500:500->250:250@my_file_name.webp' /path/to/input/file",
            "animtool dropframes -d /path/to/output/dir -R 15 -T '->300:400@file_300x400.gif#gif' /path/to/input/file",
            "animtool dropframes -d /path/to/output/dir -R 15 -T '->300:400@file_300x400.webp;600:800@file_600x800.webp;900:1200' -T '0:100:500:500->250:250' /path/to/input/file"
        },
        .n_args = 1,
        .args_desc = "Path of the image file to be processed. Supported file formats: WebP"
#ifdef WEBP_HAVE_GIF
        ", GIF"
#endif
#ifdef WEBP_HAVE_JPEG
         ", JPEG"
#endif
#ifdef WEBP_HAVE_PNG
         ", PNG"
#endif 
         ", PNM (PGM, PPM, PAM)"
#ifdef WEBP_HAVE_TIFF
         ", TIFF"
#endif
        ". A static image is treated as one frame animated image.",
        .context = nullptr,
        .action = CmdAction
    };

    cmd->AddFlag(cli::Flag{
        .name = "output",
        .aliases = {"o"},
        .desc = "output image path",
        .type = cli::FLAG_STR,
        .required = 0,
        .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
        .name = "output_dir",
        .aliases = {"d"},
        .desc = "output image dir path",
        .type = cli::FLAG_STR,
        .required = 0,
        .multiple = 0
    });

    cmd->AddFlag(cli::Flag{
        .name = "frame_rate",
        .aliases = {"R"},
        .desc = "output image max frame rate. 0 means no limits.",
        .type = cli::FLAG_INT,
        .required = 0,
        .multiple = 0,
        .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
        .name = "total_duration",
        .aliases = {"D"},
        .desc = "output image max total duration, in milliseconds. 0 means no limits.",
        .type = cli::FLAG_INT,
        .required = 0,
        .multiple = 0,
        .default_value = { .int_value = 0 }
    });

    cmd->AddFlag(cli::Flag{
        .name = "loop_count",
        .aliases = {"L"},
        .desc = "if >= 0, force loop count the value, otherwise respect the one in input file.",
        .type = cli::FLAG_INT,
        .required = 0,
        .multiple = 0,
        .default_value = { .int_value = -1 }
    });


    CmdAddOutputFlags(cmd);

    cmd->AddFlag(cli::Flag{
        .name = "transform",
        .aliases = {"T"},
        .desc = "A transform. Can have more than one. Format: "
                "`left:top:width:height->width:height@file_name;width:height@file_name#format`. "
                "`->` separates the source rectangle and the destinations. "
                "Source rectangle specifies how we crop the source image. "
                "After the `->` mark, there are multiple destinations separated by `;`. "
                "Each destination specifies the output image size and the name of the file. "
                "Following #, it is the output format, it can either be webp or gif. "
                "All of the components of a transform string are optional.",
        .type = cli::FLAG_STR,
        .required = 0,
        .multiple = 1,
        .default_value = { .str_value = "->0:0" }
    });
}

