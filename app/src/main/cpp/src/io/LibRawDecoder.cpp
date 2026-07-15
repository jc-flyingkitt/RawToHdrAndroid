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

// 前向声明辅助函数
static inline int avg4(uint16_t* rawPtr, int width, int x, int y, int center);
static inline int avg9(uint16_t* rawPtr, int width, int height, int x, int y);

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
    rawProcessor.imgdata.params.use_camera_wb = LIBRAW_USE_CAMWB;
    rawProcessor.imgdata.params.use_camera_prof = useCameraProfile_ ? 1 : 0;
    rawProcessor.imgdata.params.output_bps = 16; // 输出 16bit
    rawProcessor.imgdata.params.half_size = halfSize_ ? 1 : 0;
    rawProcessor.imgdata.params.user_mul[0] = 1.0f; // R
    rawProcessor.imgdata.params.user_mul[1] = 1.0f; // G
    rawProcessor.imgdata.params.user_mul[2] = 1.0f; // B
    rawProcessor.imgdata.params.user_mul[3] = 1.0f; // G
    rawProcessor.imgdata.params.output_color = 1; // sRGB
    rawProcessor.imgdata.params.tone_curve = 0; // 线性

    // 解析文件头
    ret = rawProcessor.unpack();
    if (ret != LIBRAW_SUCCESS) {
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to unpack RAW: " + std::string(LibRaw::strerror(ret)));
    }

    // 获取图像信息
    const libraw_opflags_t& flags = rawProcessor.imgdata.color.rgb_colors_count;
    int width = rawProcessor.imgdata.sizes.raw_width;
    int height = rawProcessor.imgdata.sizes.raw_height;
    int color_count = rawProcessor.imgdata.color.rgb_colors_count;

    LOGD("RAW dimensions: %dx%d, colors=%d", width, height, color_count);

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
        rawProcessor.recycle();
        return DecodeResult::fail("Failed to allocate output buffer");
    }

    memset(image->ptr, 0, height * stride);

    // 获取 RAW 数据指针
    uint16_t* rawPtr = rawProcessor.imgdata.rawdata.raw_image;
    if (!rawPtr) {
        rawProcessor.recycle();
        return DecodeResult::fail("No RAW data available");
    }

    // 获取 Bayer 模式信息
    int bayer = rawProcessor.imgdata.sizes.bayer; // 0=RGGB, 1=GRBG, 2=GBRG, 3=BGGR
    int bitDepth = rawProcessor.imgdata.sizes.raw_bits;

    LOGD("Bayer mode: %d, bit depth: %d", bayer, bitDepth);

    // 简单的 demosaic (bilinear interpolation)
    // 将 Bayer 格式转换为 RGB
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 3;
            int rawIdx = y * width + x;
            int rawVal = rawPtr[rawIdx] >> (16 - bitDepth); // 归一化到 12bit

            // Bayer 插值 (简化版)
            float r, g, b;

            int bx = x % 2;
            int by = y % 2;
            int pos = by * 2 + bx;

            if (bayer == 0) { // RGGB
                if (pos == 0) { // R
                    r = rawVal / 4095.0f;
                    g = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    b = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else if (pos == 1) { // G
                    r = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    g = rawVal / 4095.0f;
                    b = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else if (pos == 2) { // G
                    r = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    g = rawVal / 4095.0f;
                    b = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else { // B
                    r = avg9(rawPtr, width, height, x, y) / 4095.0f;
                    g = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    b = rawVal / 4095.0f;
                }
            } else if (bayer == 3) { // BGGR
                if (pos == 0) { // B
                    b = rawVal / 4095.0f;
                    g = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    r = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else if (pos == 1) { // G
                    b = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    g = rawVal / 4095.0f;
                    r = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else if (pos == 2) { // G
                    b = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    g = rawVal / 4095.0f;
                    r = avg9(rawPtr, width, height, x, y) / 4095.0f;
                } else { // R
                    b = avg9(rawPtr, width, height, x, y) / 4095.0f;
                    g = avg4(rawPtr, width, x, y, rawVal) / 4095.0f;
                    r = rawVal / 4095.0f;
                }
            } else {
                // 默认: 直接输出灰度
                r = g = b = rawVal / 4095.0f;
            }

            image->ptr[idx] = r;
            image->ptr[idx + 1] = g;
            image->ptr[idx + 2] = b;
        }
    }

    LOGD("Decoded: %dx%d %s image", width, height, color_count > 1 ? "color" : "mono");
    rawProcessor.recycle();

    return DecodeResult::success(std::move(image));
}

// 辅助函数: 4 点平均
static inline int avg4(uint16_t* rawPtr, int width, int x, int y, int center) {
    int sum = 0;
    int count = 0;
    // 上下
    if (y > 0) { sum += rawPtr[(y - 1) * width + x]; count++; }
    if (y < width - 1) { sum += rawPtr[(y + 1) * width + x]; count++; }
    // 左右
    if (x > 0) { sum += rawPtr[y * width + (x - 1)]; count++; }
    if (x < width - 1) { sum += rawPtr[y * width + (x + 1)]; count++; }
    return count > 0 ? sum / count : center;
}

// 辅助函数: 9 点平均
static inline int avg9(uint16_t* rawPtr, int width, int height, int x, int y) {
    int sum = 0;
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                sum += rawPtr[ny * width + nx];
                count++;
            }
        }
    }
    return count > 0 ? sum / count : 0;
}
