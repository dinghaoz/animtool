//
// Created by Dinghao Zeng on 2023/2/3.
//

#include "animenc.h"
#include "quantizer.h"

#include "webp/encode.h"
#include "webp/mux.h"
#include "../imageio/imageio_util.h"

#include "check_gif.h"

#include "gif_lib.h"

#include "check.h"
#include "logger.h"
#include "utils/defer.h"

#include <cstdlib>
#include <cmath>
#include <unordered_map>

struct AnimEncoder {
public:
    virtual int Init(int canvas_width, int canvas_height, const AnimEncoderOptions* options) = 0;
    virtual int AddFrame(WebPPicture* pic, int start_ts, int end_ts, const AnimFrameOptions* options) = 0;
    virtual int Export(int final_ts, int loop_count, const char* output_path) = 0;
    virtual const char* GetFileExt() const = 0;
    virtual ~AnimEncoder() = default;;
};


struct AnimEncoderWebP : public AnimEncoder {
private:
    WebPAnimEncoder* impl;
public:
    ~AnimEncoderWebP() override {
        if (impl)
            WebPAnimEncoderDelete(impl);
    }

    const char* GetFileExt() const override {
        return ".webp";
    }

    int Init(int canvas_width, int canvas_height, const AnimEncoderOptions* options) override{
        require(!impl);
        WebPAnimEncoderOptions encoder_options;
        check(WebPAnimEncoderOptionsInit(&encoder_options));

        encoder_options.verbose = options->verbose;
        encoder_options.minimize_size = options->minimize_size;
        encoder_options.anim_params.bgcolor = options->bgcolor;


        auto encoder = WebPAnimEncoderNew(
                canvas_width,
                canvas_height,
                &encoder_options);
        checkf(encoder, "Failed to create WebPAnimEncoderNew");

        impl = encoder;

        return 1;
    }

    int AddFrame(WebPPicture* pic, int start_ts, int end_ts, const AnimFrameOptions* options) override {
        require(impl);

        WebPConfig config;
        check(WebPConfigInit(&config));

        config.lossless = options->lossless;
        config.method = options->method;
        config.pass = options->pass;
        config.quality = options->quality;


        checkf(WebPAnimEncoderAdd(impl, pic, start_ts, &config), "%s", WebPAnimEncoderGetError(impl));

        return 1;
    }

    static int SetLoopCount(int loop_count, WebPData* const webp_data) {
        int ok = 1;
        WebPMuxError err;
        uint32_t features;
        WebPMuxAnimParams new_params;
        WebPMux* const mux = WebPMuxCreate(webp_data, 1);
        if (mux == nullptr) return 0;

        err = WebPMuxGetFeatures(mux, &features);
        ok = (err == WEBP_MUX_OK);
        if (!ok || !(features & ANIMATION_FLAG)) goto End;

        err = WebPMuxGetAnimationParams(mux, &new_params);
        ok = (err == WEBP_MUX_OK);
        if (ok) {
            new_params.loop_count = loop_count;
            err = WebPMuxSetAnimationParams(mux, &new_params);
            ok = (err == WEBP_MUX_OK);
        }
        if (ok) {
            WebPDataClear(webp_data);
            err = WebPMuxAssemble(mux, webp_data);
            ok = (err == WEBP_MUX_OK);
        }

        End:
        WebPMuxDelete(mux);
        if (!ok) {
            logger::e("Error during loop-count setting");
        }
        return ok;
    }

    int Export(int final_ts, int loop_count, const char* output_path) override {
        require(impl);

        checkf(WebPAnimEncoderAdd(impl, nullptr, final_ts, nullptr), "%s", WebPAnimEncoderGetError(impl));

        WebPData webp_out_data;
        WebPDataInit(&webp_out_data);
        defer(WebPDataClear(&webp_out_data));
        checkf(WebPAnimEncoderAssemble(impl, &webp_out_data), "%s", WebPAnimEncoderGetError(impl));

        if (loop_count > 0) {
            logger::d("SetLoopCount %d", loop_count);
            check(AnimEncoderWebP::SetLoopCount(loop_count, &webp_out_data));
        }

        check(ImgIoUtilWriteFile(output_path, webp_out_data.bytes, webp_out_data.size));
        logger::i("File created at %s", output_path);

        return 1;
    }
};

class ByteArray {
private:
    int capacity;
    GifByteType* buffer;
    int count;

public:
    ByteArray(): capacity(0), buffer(0), count(0) {
    }

    ~ByteArray() {
        if (buffer)
            free(buffer);
    }

    const GifByteType* GetBytes() const {
        return buffer;
    }

    int GetCount() const {
        return count;
    }

    int Append(const GifByteType* bytes, int size) {
        if (count + size > capacity) {
            int new_cap = (count + size - capacity) * 2 + capacity;
            auto new_buf = reinterpret_cast<GifByteType*>(malloc(new_cap));
            check(new_buf);

            if (buffer) {
                memcpy(new_buf, buffer, count);
                free(buffer);
            }

            buffer = new_buf;
            capacity = new_cap;
        }

        memcpy(buffer + count, bytes, size);
        count += size;

        return 1;
    }
};


struct AnimEncoderGif : public AnimEncoder {
    static int FileOutputFunc(GifFileType * fileType, const GifByteType * bytes, int size) {
        auto encoder = reinterpret_cast<AnimEncoderGif*>(fileType->UserData);
        check(encoder->buffer.Append(bytes, size));
        return size;
    }
    static const uint8_t    COLOR_RES = 8;             // color位数, 0~8 
    static const int        COLOR_COUNT = 1 << COLOR_RES;       // color数量，这里使用256
    static const int TRANSPARENT_INDEX = 0;

    static int Distance(GifColorType l, GifColorType r) {
        return (l.Red - r.Red) * (l.Red - r.Red) +
               (l.Green - r.Green) * (l.Green - r.Green) +
               (l.Blue - r.Blue) * (l.Blue - r.Blue);
    }

    static int FindIndex(const ColorMapObject* cmo, GifColorType color) {
        int min_dist = 0;
        int min_idx = -1;
        for (int i=TRANSPARENT_INDEX + 1; i<cmo->ColorCount; ++i) {
            auto dist = Distance(cmo->Colors[i], color);
            if (min_idx < 0 || min_dist > dist) {
                min_idx = i;
                min_dist = dist;
            }
        }

        return min_idx;
    }

public:
    ~AnimEncoderGif() override {
        if (impl) {
            int gif_error = 0;
            if (!DGifCloseFile(impl, &gif_error)) {
                log_gif_error("DGifCloseFile", gif_error);
            }
        }
    }

    const char* GetFileExt() const override {
        return ".gif";
    }

    int Init(int canvas_width, int canvas_height, const AnimEncoderOptions* options) override  {
        require(!impl);

        logger::d("GIF Encode via giflib(%d.%d.%d)", GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE);

        impl = EGifOpen(this, FileOutputFunc, NULL);
        check(impl);

        EGifSetGifVersion(impl, 1);

        check_gif(EGifPutScreenDesc(impl, canvas_width, canvas_height, COLOR_RES, 0, 0), impl);

        static const GifByteType aeLen = 11;
        static const char *aeBytes = { "NETSCAPE2.0" };
        static const GifByteType aeSubLen = 3;
        static GifByteType aeSubBytes[aeSubLen] = { 1, 0, 0 };
        check_gif(EGifPutExtensionLeader(impl, APPLICATION_EXT_FUNC_CODE), impl);
        check_gif(EGifPutExtensionBlock(impl, aeLen, aeBytes), impl);
        check_gif(EGifPutExtensionBlock(impl, aeSubLen, aeSubBytes), impl);
        check_gif(EGifPutExtensionTrailer(impl), impl);

        return 1;
    }

    static void QuantizerVisit(void* ctx, int i, uint8_t r, uint8_t g, uint8_t b) {
        auto cmap = reinterpret_cast<ColorMapObject*>(ctx);
        auto& color = cmap->Colors[i+1];
        color.Red = r;
        color.Green = g;
        color.Blue = b;
    }

    int AddFrame(WebPPicture* pic, int start_ts, int end_ts, const AnimFrameOptions* options) override {
        require(impl);
        require(pic->use_argb);

        int end_ts_ten_ms = static_cast<int>(round(end_ts / 10.0));
        int delay_ten_ms = end_ts_ten_ms - duration_ten_ms;
        duration_ten_ms = end_ts_ten_ms;

        GraphicsControlBlock gcb {
            .DisposalMode = DISPOSE_BACKGROUND,
            .UserInputFlag = false,
            .DelayTime = delay_ten_ms,
            .TransparentColor = TRANSPARENT_INDEX
        };

        GifByteType extension[4];
        check_gif(EGifGCBToExtension(&gcb, extension), impl);
        check_gif(EGifPutExtension(impl, GRAPHICS_EXT_FUNC_CODE, sizeof(extension), extension), impl);

        auto color_map = GifMakeMapObject(COLOR_COUNT, nullptr);
        check(color_map);
        defer(GifFreeMapObject(color_map));

        WuQuantizer quantizer;
        check(quantizer.Init(pic->width, pic->height));


        for (int y=0; y<pic->height; ++y) {
            auto argb_line = pic->argb + pic->argb_stride * y;
            for (int x=0; x<pic->width; ++x) {
                GifColorType pixel = {
                        .Red = static_cast<GifByteType>((argb_line[x] >> 16) & 0x000000FF),
                        .Green = static_cast<GifByteType>((argb_line[x] >> 8) & 0x000000FF),
                        .Blue = static_cast<GifByteType>(argb_line[x] & 0x000000FF)
                };

                quantizer.AddPixel(x, y, pixel.Red, pixel.Green, pixel.Blue);
            }
        }

        check(quantizer.Build(COLOR_COUNT - TRANSPARENT_INDEX - 1, color_map, QuantizerVisit));


        check_gif(EGifPutImageDesc(impl, 0, 0, pic->width, pic->height, false, color_map), impl);

        std::unordered_map<uint32_t, uint8_t> cached_index_map;

        for (int y=0; y<pic->height; ++y) {
            auto argb_line = pic->argb + pic->argb_stride * y;
            for (int x=0; x<pic->width; ++x) {
                auto pixel = argb_line[x];
                auto rgb = pixel & 0x00FFFFFF;
                auto alpha = pixel >> 24;
                if (alpha < 64) {
                    check_gif(EGifPutPixel(impl, 0), impl);
                } else {
                    if (cached_index_map.find(rgb) != cached_index_map.end()) {
                        check_gif(EGifPutPixel(impl, cached_index_map[rgb]), impl);
                    } else {

                        GifColorType color = {
                                .Red = static_cast<GifByteType>((rgb >> 16) & 0x000000FF),
                                .Green = static_cast<GifByteType>((rgb >> 8) & 0x000000FF),
                                .Blue = static_cast<GifByteType>(rgb & 0x000000FF)
                        };

                        auto color_index = FindIndex(color_map, color);
                        requiref(color_index > TRANSPARENT_INDEX, "map count %d, index=%d", color_map->ColorCount, color_index);
                        auto color_index_uint8 = static_cast<uint8_t>(color_index);

                        cached_index_map[rgb] = color_index_uint8;
                        check_gif(EGifPutPixel(impl, color_index_uint8), impl);
                    }
                }
            }
        }

        return 1;
    }


    int Export(int final_ts, int loop_count, const char* output_path) override {
        require(impl);

        int gif_error = 0;
        if (!EGifCloseFile(impl, &gif_error)) {
            log_gif_error("EGifCloseFile", gif_error);
            return 0;
        }
        impl = 0;

        require(buffer.GetCount() > 0);
        check(ImgIoUtilWriteFile(output_path, buffer.GetBytes(), buffer.GetCount()));

        logger::i("File created at %s", output_path);

        return 1;
    }
private:
    GifFileType* impl;
    ByteArray buffer;
    int duration_ten_ms;
};

AnimEncoder* AnimEncoderNew(const char* format, int canvas_width, int canvas_height, const AnimEncoderOptions* options) {
    require(format);

    AnimEncoder* encoder = 0;

    if (!strcasecmp(format, "webp") || strlen(format) == 0) {
        encoder = new AnimEncoderWebP();
        check(encoder);
    } else if (!strcasecmp(format, "gif")) {
        encoder = new AnimEncoderGif();
        check(encoder);
    } else {
        notreached("Unknown AnimEncoder format `%s`", format);
    }

    if (!encoder->Init(canvas_width, canvas_height, options)) {
        AnimEncoderDelete(encoder);
        return 0;
    }

    return encoder;
}


int AnimEncoderAddFrame(AnimEncoder* encoder, WebPPicture* pic, int start_ts, int end_ts, const AnimFrameOptions* options) {
    return encoder->AddFrame(pic, start_ts, end_ts, options);
}

int AnimEncoderExport(AnimEncoder* encoder, int final_ts, int loop_count, const char* output_path) {
    return encoder->Export(final_ts, loop_count, output_path);
}

const char* AnimEncoderGetFileExt(const AnimEncoder* encoder) {
    return encoder->GetFileExt();
}

void AnimEncoderDelete(AnimEncoder* encoder) {
   delete encoder;
}


