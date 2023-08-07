//
// Created by Dinghao Zeng on 2023/8/7.
//

#include "count.h"

#include "core/animrun.h"
#include "core/rawgif.h"
#include "decrun.h"
#include "utils/parse.h"

#include "check.h"

struct Context {
    int frame_count;

    int index_of_frame;
    int x, y, width, height;
    CountPredicate red;
    CountPredicate green;
    CountPredicate blue;
    CountPredicate alpha;

    int result;
};

static int PredicateMatched(uint8_t c, CountPredicate predicate) {
    return (predicate.equal < 0 || c == predicate.equal) &&
           (predicate.larger < 0 || c > predicate.larger) &&
           (predicate.larger_equal < 0 || c >= predicate.larger) &&
           (predicate.smaller < 0 || c < predicate.smaller) &&
           (predicate.smaller_equal < 0 || c <= predicate.smaller_equal);
}

static void Counter(void* ctx, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    if (PredicateMatched(r, thiz->red) && PredicateMatched(g, thiz->green) && PredicateMatched(b, thiz->green) && PredicateMatched(a, thiz->alpha))
        ++thiz->result;
}


static int OnStart(void* ctx, const AnimInfo* info, int* stop) {
    return 1;
}

static int OnFrame(void* ctx, const AnimFrame* frame, int start_ts, int end_ts, int* stop) {
    auto thiz = reinterpret_cast<Context*>(ctx);

    if (thiz->frame_count == thiz->index_of_frame) {
        check(AnimFrameEnumerate(frame, thiz->x, thiz->y, thiz->width, thiz->height, ctx, Counter));
        *stop = 1;
    }

    ++thiz->frame_count;

    return 1;
}

static int OnEnd(void* ctx, const AnimInfo* anim_info) {
    return 1;
}



static AnimDecRunCallback kRunCallback {
        .on_start = OnStart,
        .on_frame = OnFrame,
        .on_end = OnEnd
};


int AnimToolCount(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        CountPredicate red,
        CountPredicate green,
        CountPredicate blue,
        CountPredicate alpha,
        int* count
) {
    Context ctx{
        .index_of_frame = index_of_frame,
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .red = red,
        .green = green,
        .blue = blue,
        .alpha = alpha,
        .result = 0
    };

    check(DecRun(image_path, &ctx, kRunCallback));

    *count = ctx.result;

    return 1;
}

const CountPredicate CountPredicateDefault = {
        .larger = -1,
        .larger_equal = -1,
        .smaller = -1,
        .smaller_equal = -1,
        .equal = -1
};

struct NamedPredicate {
    const char* name_start;
    const char* name_end;
    CountPredicate predicate;
};


int ParsePredicate(const char* pstart, const char* pend, NamedPredicate* value) {
    value->predicate = CountPredicateDefault;
    auto name_start = pstart;
    while (name_start < pend) {
        if ('a' <= *name_start && *name_start <= 'z')
            break;
        ++name_start;
    }

    if (name_start == pend)
        return 0;

    auto name_end = name_start;
    while (name_end < pend) {
        if ('a' <= *name_end && *name_end <= 'z') {
            ++name_end;
        } else {
            break;
        }
    }

    value->name_start = name_start;
    value->name_end = name_end;

    auto ple = StrSearch(pstart, name_start, "<=");
    if (ple < name_start) {
        if (ple > pstart) {
            if (ParseInt(pstart, ple, &value->predicate.larger_equal)) {
                return -1;
            }
        }
    }
    auto pl = StrSearch(pstart, name_start, "<");
    if (pl < name_start) {
        if (pl > pstart) {
            if (ParseInt(pstart, pl, &value->predicate.larger)) {
                return -1;
            }
        }
    }


    auto pse = StrSearch(name_end, pend, "<=");
    if (pse < pend) {
        pse += strlen("<=");
        if (pse > name_end) {
            if (ParseInt(pse, pend, &value->predicate.smaller_equal)) {
                return -1;
            }
        }
    }
    auto ps = StrSearch(name_end, pend, "<");
    if (ps < pend) {
        ps += strlen("<");
        if (ps > name_end) {
            if (ParseInt(ps, pend, &value->predicate.smaller)) {
                return -1;
            }
        }
    }

    auto pe = StrSearch(name_end, pend, "=");
    if (pe < pend) {
        pe += strlen("=");
        if (pe > name_end) {
            if (ParseInt(pe, pend, &value->predicate.equal)) {
                return -1;
            }
        }
    }


    return 0;
}


int ParsePredicates(const char* predicates,
                    CountPredicate *o_red,
                    CountPredicate *o_green,
                    CountPredicate *o_blue,
                    CountPredicate *o_alpha) {

    NamedPredicate named_predicates[4];

    int count = 0;
    if (ParseList(predicates, predicates + strlen(predicates), ":", named_predicates, sizeof(named_predicates)/sizeof(NamedPredicate), &count,
                    ParsePredicate)) {
        return -1;
    }

    for (int i=0; i<count; ++i) {
        const auto& p = named_predicates[i];
        if (strncmp("alpha", p.name_start, p.name_end - p.name_start) == 0) {
            *o_alpha = p.predicate;
        } else if (strncmp("red", p.name_start, p.name_end - p.name_start) == 0) {
            *o_red = p.predicate;
        } else if (strncmp("green", p.name_start, p.name_end - p.name_start) == 0) {
            *o_green = p.predicate;
        } else if (strncmp("blue", p.name_start, p.name_end - p.name_start) == 0) {
            *o_blue = p.predicate;
        }
    }

    return 0;
}

int AnimToolCountStr(
        const char* image_path,
        int index_of_frame,
        int x, int y, int width, int height,
        const char* predicates,
        int* count
) {
    CountPredicate red = CountPredicateDefault;
    CountPredicate green = CountPredicateDefault;
    CountPredicate blue = CountPredicateDefault;
    CountPredicate alpha = CountPredicateDefault;

    if (ParsePredicates(predicates, &red, &green, &blue, &alpha)) {
        return 0;
    }

    return AnimToolCount(image_path, index_of_frame, x, y, width, height, red, green, blue, alpha, count);
}