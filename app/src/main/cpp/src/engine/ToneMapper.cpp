/**
 * 色调映射器实现
 */

#include "engine/ToneMapper.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "ToneMapper"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

ToneMapper::ToneMapper() {}
ToneMapper::~ToneMapper() {}

void ToneMapper::setExposure(float exposure) {
    exposure_ = std::max(0.1f, std::min(10.0f, exposure));
}

void ToneMapper::setSaturation(float saturation) {
    saturation_ = std::max(0.5f, std::min(3.0f, saturation));
}

void ToneMapper::setContrast(float contrast) {
    contrast_ = std::max(0.5f, std::min(3.0f, contrast));
}

std::unique_ptr<ImageData> ToneMapper::map(
    std::shared_ptr<ImageData> hdrImage,
    Algorithm algorithm) {

    if (!hdrImage || !hdrImage->ptr) {
        LOGE("Invalid HDR image");
        return nullptr;
    }

    size_t width = hdrImage->width;
    size_t height = hdrImage->height;

    LOGD("Tone mapping: %zu x %zu, algorithm=%d", width, height, static_cast<int>(algorithm));

    // 分配 8bit RGB 输出缓冲区
    size_t pixelSize = 3;
    size_t stride = width * pixelSize;
    auto output = std::make_unique<ImageData>();
    output->width = width;
    output->height = height;
    output->stride = stride;
    output->pixelSize = pixelSize;
    output->format = PixelFormat::RGB888;
    output->ptr = static_cast<float*>(malloc(height * stride));

    if (!output->ptr) {
        LOGE("Failed to allocate output buffer");
        return nullptr;
    }

    const float* src = hdrImage->ptr;
    unsigned char* dst = reinterpret_cast<unsigned char*>(output->ptr);

    for (size_t i = 0; i < width * height * 3; i += 3) {
        // 读取 HDR 像素 (float)
        float r = src[i];
        float g = src[i + 1];
        float b = src[i + 2];

        // 应用曝光
        r *= exposure_;
        g *= exposure_;
        b *= exposure_;

        // Reinhard 色调映射
        r = r / (1.0f + r);
        g = g / (1.0f + g);
        b = b / (1.0f + b);

        // 应用饱和度和对比度
        float luminance = (r + g + b) / 3.0f;
        r = luminance + (r - luminance) * saturation_;
        g = luminance + (g - luminance) * saturation_;
        b = luminance + (b - luminance) * saturation_;

        // 裁剪并转换为 8bit
        dst[i] = static_cast<unsigned char>(std::min(255.0f, std::max(0.0f, r * 255.0f)));
        dst[i + 1] = static_cast<unsigned char>(std::min(255.0f, std::max(0.0f, g * 255.0f)));
        dst[i + 2] = static_cast<unsigned char>(std::min(255.0f, std::max(0.0f, b * 255.0f)));
    }

    LOGD("Tone mapping completed: %zu x %zu", width, height);
    return output;
}
