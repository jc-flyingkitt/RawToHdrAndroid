/**
 * 图像对齐器实现
 */

#include "engine/ImageAligner.h"
#include <android/log.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "ImageAligner"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

void ImageAligner::setMaxFeatures(int count) {
    maxFeatures_ = std::max(100, std::min(2000, count));
}

void ImageAligner::setScaleFactor(float scale) {
    scaleFactor_ = std::max(1.05f, std::min(1.5f, scale));
}

ImageAligner::AlignResult ImageAligner::align(
    std::vector<std::shared_ptr<ImageData>>& images) {

    LOGD("Image alignment: %zu images", images.size());

    if (images.size() < 2) {
        return {true, "No alignment needed"};
    }

    // 使用第一张图像作为参考
    auto ref = images[0];
    size_t refWidth = ref->width;
    size_t refHeight = ref->height;

    for (size_t i = 1; i < images.size(); i++) {
        auto img = images[i];
        if (!img || !img->ptr) continue;

        LOGD("Aligning image %zu: %zu x %zu", i, img->width, img->height);

        // 如果尺寸相同且内容相似，跳过对齐
        if (img->width == refWidth && img->height == refHeight) {
            // 这里可以添加更精确的内容比对
            continue;
        }

        // 简化: 如果尺寸不同，需要重采样到参考尺寸
        if (img->width != refWidth || img->height != refHeight) {
            LOGD("Resampling image %zu to %zu x %zu", i, refWidth, refHeight);
            auto resampled = applyTransform(img, nullptr, refWidth, refHeight);
            if (resampled) {
                images[i] = std::move(resampled);
            }
        }
    }

    LOGD("Alignment completed: %zu images", images.size());
    return {true, ""};
}

bool ImageAligner::matchFeatures(
    std::shared_ptr<ImageData> img1,
    std::shared_ptr<ImageData> img2,
    float* transformMatrix) {

    // TODO: 实现 ORB 特征点匹配
    // 当前版本使用简化对齐 (基于仿射变换)
    LOGD("Feature matching: %zu x %zu -> %zu x %zu",
         img1->width, img1->height,
         img2->width, img2->height);

    // 计算缩放比例
    float scaleX = static_cast<float>(img1->width) / static_cast<float>(img2->width);
    float scaleY = static_cast<float>(img1->height) / static_cast<float>(img2->height);

    if (transformMatrix) {
        // 简单的缩放变换矩阵
        transformMatrix[0] = scaleX; transformMatrix[1] = 0; transformMatrix[2] = 0;
        transformMatrix[3] = 0; transformMatrix[4] = scaleY; transformMatrix[5] = 0;
    }

    return true;
}

std::shared_ptr<ImageData> ImageAligner::applyTransform(
    std::shared_ptr<ImageData> image,
    const float* transformMatrix,
    size_t refWidth,
    size_t refHeight) {

    if (!image || !image->ptr) {
        LOGE("Invalid input image");
        return nullptr;
    }

    size_t srcWidth = image->width;
    size_t srcHeight = image->height;

    // 分配输出缓冲区
    size_t pixelSize = 3 * sizeof(float);
    size_t stride = refWidth * pixelSize;
    auto output = std::make_unique<ImageData>();
    output->width = refWidth;
    output->height = refHeight;
    output->stride = stride;
    output->pixelSize = pixelSize;
    output->format = PixelFormat::FLOAT32;
    output->ptr = static_cast<float*>(malloc(refHeight * stride));

    if (!output->ptr) {
        LOGE("Failed to allocate output buffer");
        return nullptr;
    }

    memset(output->ptr, 0, refHeight * stride);

    const float* src = image->ptr;
    float* dst = output->ptr;

    // 双线性插值重采样
    for (size_t y = 0; y < refHeight; y++) {
        for (size_t x = 0; x < refWidth; x++) {
            // 计算源图像坐标
            float srcX = static_cast<float>(x) * srcWidth / refWidth;
            float srcY = static_cast<float>(y) * srcHeight / refHeight;

            size_t srcX0 = static_cast<size_t>(srcX);
            size_t srcY0 = static_cast<size_t>(srcY);
            size_t srcX1 = std::min(srcX0 + 1, srcWidth - 1);
            size_t srcY1 = std::min(srcY0 + 1, srcHeight - 1);

            float fx = srcX - srcX0;
            float fy = srcY - srcY0;

            // 四个角点
            size_t idx00 = (srcY0 * srcWidth + srcX0) * 3;
            size_t idx10 = (srcY0 * srcWidth + srcX1) * 3;
            size_t idx01 = (srcY1 * srcWidth + srcX0) * 3;
            size_t idx11 = (srcY1 * srcWidth + srcX1) * 3;

            size_t dstIdx = (y * refWidth + x) * 3;

            for (int c = 0; c < 3; c++) {
                float v00 = src[idx00 + c];
                float v10 = src[idx10 + c];
                float v01 = src[idx01 + c];
                float v11 = src[idx11 + c];

                // 双线性插值
                float v0 = v00 * (1 - fx) + v10 * fx;
                float v1 = v01 * (1 - fx) + v11 * fx;
                dst[dstIdx + c] = v0 * (1 - fy) + v1 * fy;
            }
        }
    }

    LOGD("Resampled image: %zu x %zu -> %zu x %zu",
         srcWidth, srcHeight, refWidth, refHeight);

    return output;
}
