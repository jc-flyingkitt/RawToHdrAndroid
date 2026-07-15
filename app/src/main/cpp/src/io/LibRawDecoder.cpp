/**
 * libraw 解码器实现
 * 使用 libraw 库解码 CR2/CR3 RAW 文件
 */

#include "io/LibRawDecoder.h"
#include <libraw/libraw.h>
#include <android/log.h>
#include <cstring>
#include <cmath>

#define LOG_TAG "LibRawDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

RawDecoder::RawDecoder() {
    LOGD("RawDecoder constructed");
}

RawDecoder::~RawDecoder() {
    LOGD("RawDecoder destroyed");
}

void RawDecoder::setUseCameraProfile(bool use) {
    useCameraProfile_ = use;
}

void RawDecoder::setDemosaicAlgorithm(int algorithm) {
    demosaicAlgorithm_ = std::max(0, std::min(15, algorithm));
}

void RawDecoder::setHalfSize(bool half) {
    halfSize_ = half;
}

DecodeResult RawDecoder::decode(const char* filePath) {
    LOGD("Decoding: %s", filePath);

    if (!filePath || strlen(filePath) == 0) {
        return DecodeResult::fail("Empty file path");
    }

    // 创建 libraw 实例
    LibRaw rawProcessor;

    // 设置文件名
    int ret = rawProcessor.open_file(filePath);
    if (ret != LIBRAW_SUCCESS) {
        return DecodeResult::fail("Failed to open RAW file: " + std::string(LibRaw::strerror(ret)));
    }

    LOGD("Opened RAW file: %s", filePath);

    // 设置解码参数
    rawProcessor.imgdata.params.use_camera_wb = 1;          // 使用相机白平衡
    rawProcessor.imgdata.params.output_bps = 16;            // 输出 16bit
    rawProcessor.imgdata.params.half_size = halfSize_ ? 1 : 0;
    rawProcessor.imgdata.params.user_mul[0] = 1.0f;         // R
    rawProcessor.imgdata.params.user_mul[1] = 1.0f;         // G
    rawProcessor.imgdata.params.user_mul[2] = 1.0f;         // B
    rawProcessor.imgdata.params.user_mul[3] = 1.0f;         // G
    rawProcessor.imgdata.params.output_color = 1;           // sRGB
    rawProcessor.imgdata.params.gamm[0] = 1.0f;             // 线性 gamma
    rawProcessor.imgdata.params.gamm[1] = 1.0f;
    rawProcessor.imgdata.params.no_auto_bright = 1;         // 不自动调亮

    // 解析文件头
    ret = rawProcessor.unpack();
    if (ret != LIBRAW_SUCCESS) {
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to unpack RAW: " + std::string(LibRaw::strerror(ret)));
    }

    // 使用 libraw 的 dcraw_process 进行 demosaic 和颜色处理
    ret = rawProcessor.dcraw_process();
    if (ret != LIBRAW_SUCCESS) {
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to process RAW: " + std::string(LibRaw::strerror(ret)));
    }

    // 获取处理后的图像
    libraw_processed_image_t* processed = rawProcessor.dcraw_make_mem_image(&ret);
    if (!processed || ret != LIBRAW_SUCCESS) {
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to make memory image");
    }

    int width = processed->width;
    int height = processed->height;
    int colors = processed->colors;

    LOGD("RAW processed: %dx%d, colors=%d, bits=%d", width, height, colors, processed->bits);

    // 分配输出缓冲区 (32bit float RGB)
    size_t pixelSize = 3 * sizeof(float);
    size_t stride = width * pixelSize;
    auto image = std::make_unique<ImageData>();
    image->width = width;
    image->height = height;
    image->stride = stride;
    image->pixelSize = pixelSize;
    image->format = PixelFormat::FLOAT32;
    image->ptr = static_cast<float*>(malloc(height * stride));

    if (!image->ptr) {
        LibRaw::dcraw_clear_mem(processed);
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to allocate output buffer");
    }

    memset(image->ptr, 0, height * stride);

    if (processed->bits == 16) {
        // 16bit 数据
        unsigned short* src16 = reinterpret_cast<unsigned short*>(processed->data);
        size_t srcChannels = processed->colors;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t srcIdx = (y * width + x) * srcChannels;
                size_t dstIdx = (y * width + x) * 3;
                image->ptr[dstIdx]     = src16[srcIdx]     / 65535.0f;
                image->ptr[dstIdx + 1] = src16[srcIdx + 1] / 65535.0f;
                image->ptr[dstIdx + 2] = (srcChannels > 2) ? src16[srcIdx + 2] / 65535.0f : 0.0f;
            }
        }
    } else {
        // 8bit 数据
        unsigned char* src8 = processed->data;
        size_t srcChannels = processed->colors;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t srcIdx = (y * width + x) * srcChannels;
                size_t dstIdx = (y * width + x) * 3;
                image->ptr[dstIdx]     = src8[srcIdx]     / 255.0f;
                image->ptr[dstIdx + 1] = src8[srcIdx + 1] / 255.0f;
                image->ptr[dstIdx + 2] = (srcChannels > 2) ? src8[srcIdx + 2] / 255.0f : 0.0f;
            }
        }
    }

    LOGD("Decoded: %dx%d %s image", width, height, colors > 1 ? "color" : "mono");

    LibRaw::dcraw_clear_mem(processed);
    rawProcessor.recycle();

    return DecodeResult::success(std::move(image));
}