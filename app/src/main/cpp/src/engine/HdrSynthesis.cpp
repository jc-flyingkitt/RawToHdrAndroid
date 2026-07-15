/**
 * HDR 合成实现
 */

#include "engine/HdrSynthesis.h"
#include <android/log.h>
#include <cmath>
#include <numeric>

#define LOG_TAG "HdrSynthesis"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

ConversionResult HdrSynthesis::synthesize(
    std::vector<std::shared_ptr<ImageData>>& images) {

    LOGD("HDR synthesis: %zu images", images.size());

    if (images.size() < 3) {
        return ConversionResult::fail("Need at least 3 images");
    }

    // 获取参考尺寸
    auto ref = images[0];
    size_t width = ref->width;
    size_t height = ref->height;

    // Step 1: 估算响应曲线
    LOGD("Estimating response curve...");
    std::vector<float> responseCurve(4096); // 假设 RAW 最大 12bit
    estimateResponseCurve(images, responseCurve);

    // Step 2: 计算权重
    LOGD("Computing weights...");
    auto weights = computeWeights(images);

    // Step 3: 合并图像
    LOGD("Merging images...");
    auto merged = mergeImages(images, weights);

    if (!merged || !merged->ptr) {
        return ConversionResult::fail("Merge failed");
    }

    return ConversionResult::success("");
}

ConversionResult HdrSynthesis::singleToHdr(
    std::shared_ptr<ImageData> image) {

    if (!image || !image->ptr) {
        return ConversionResult::fail("Invalid image data");
    }

    LOGD("Single to HDR: %zu x %zu", image->width, image->height);
    // 单张转换: 直接返回线性数据
    return ConversionResult::success("");
}

void HdrSynthesis::estimateResponseCurve(
    std::vector<std::shared_ptr<ImageData>>& images,
    std::vector<float>& responseCurve) {

    // 简化的 Debevec 响应曲线估算
    // 基于多张图像的平均像素值来估计
    size_t samples = 0;
    std::fill(responseCurve.begin(), responseCurve.end(), 0.0f);

    for (size_t i = 0; i < images.size(); i++) {
        auto img = images[i];
        if (!img || !img->ptr) continue;

        // 从图像中采样像素 (随机采样 10000 个像素)
        const size_t sampleCount = 10000;
        for (size_t s = 0; s < sampleCount; s++) {
            // 简化: 只采样中心区域
            size_t x = rand() % img->width;
            size_t y = rand() % img->height;
            size_t idx = (y * img->width + x) * 3;

            // 获取 RGB 值
            float r = img->ptr[idx];
            float g = img->ptr[idx + 1];
            float b = img->ptr[idx + 2];

            // 计算平均亮度
            float luminance = (r + g + b) / 3.0f;

            // 记录到响应曲线
            if (luminance > 0 && luminance < 4096) {
                size_t bin = static_cast<size_t>(luminance);
                responseCurve[bin] += luminance;
                samples++;
            }
        }
    }

    // 归一化
    if (samples > 0) {
        for (auto& val : responseCurve) {
            val /= samples;
        }
    }

    LOGD("Response curve estimated with %zu samples", samples);
}

std::vector<float> HdrSynthesis::computeWeights(
    std::vector<std::shared_ptr<ImageData>>& images) {

    std::vector<float> weights(images.size(), 0.0f);

    // 基于曝光值的权重 (中间曝光权重最高)
    size_t mid = images.size() / 2;
    for (size_t i = 0; i < images.size(); i++) {
        float dist = static_cast<float>(std::abs(static_cast<int>(i) - static_cast<int>(mid)));
        weights[i] = 1.0f / (1.0f + dist * dist);
    }

    // 归一化
    float sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
    if (sum > 0) {
        for (auto& w : weights) {
            w /= sum;
        }
    }

    LOGD("Weights computed: %zu images", images.size());
    return weights;
}

std::unique_ptr<ImageData> HdrSynthesis::mergeImages(
    std::vector<std::shared_ptr<ImageData>>& images,
    const std::vector<float>& weights) {

    // 获取参考尺寸
    auto ref = images[0];
    size_t width = ref->width;
    size_t height = ref->height;

    // 分配输出缓冲区 (float RGB)
    size_t pixelSize = 3 * sizeof(float);
    size_t stride = width * pixelSize;
    auto output = std::make_unique<ImageData>();
    output->width = width;
    output->height = height;
    output->stride = stride;
    output->pixelSize = pixelSize;
    output->format = PixelFormat::FLOAT32;
    output->ptr = static_cast<float*>(malloc(height * stride));

    if (!output->ptr) {
        LOGE("Failed to allocate output buffer: %zu x %zu", width, height);
        return nullptr;
    }

    // 初始化输出为零
    memset(output->ptr, 0, height * stride);

    // 逐像素加权合并
    for (size_t i = 0; i < images.size(); i++) {
        auto img = images[i];
        if (!img || !img->ptr) continue;

        float weight = weights[i];
        size_t imgStride = img->stride;

        // 逐像素合并
        for (size_t y = 0; y < height; y++) {
            float* dstRow = output->ptr + y * width * 3;
            const float* srcRow = img->ptr + y * width * 3;

            for (size_t x = 0; x < width; x++) {
                // 加权累加
                dstRow[x * 3 + 0] += srcRow[x * 3 + 0] * weight;
                dstRow[x * 3 + 1] += srcRow[x * 3 + 1] * weight;
                dstRow[x * 3 + 2] += srcRow[x * 3 + 2] * weight;
            }
        }
    }

    // 归一化 (除以权重和)
    float weightSum = std::accumulate(weights.begin(), weights.end(), 0.0f);
    if (weightSum > 0) {
        size_t totalPixels = width * height * 3;
        float* ptr = output->ptr;
        for (size_t i = 0; i < totalPixels; i++) {
            ptr[i] /= weightSum;
        }
    }

    LOGD("Merged image: %zu x %zu, %zu bytes", width, height, height * stride);
    return output;
}
