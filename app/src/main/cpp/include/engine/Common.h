/**
 * 公共类型定义
 */

#ifndef HDR_COMMON_H
#define HDR_COMMON_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

// ============================================================
// 像素格式
// ============================================================

enum class PixelFormat {
    NONE = 0,
    RGB888 = 1,       // 8bit RGB
    RGB161616 = 2,    // 16bit RGB (用于 10bit+6bit padding)
    FLOAT32 = 3,      // 32bit float RGB (HDR 标准)
    BGGR8 = 4,        // 8bit Bayer (RAW)
    BGGR12 = 5,       // 12bit Bayer
    BGGR14 = 6,       // 14bit Bayer
};

// ============================================================
// 输出格式
// ============================================================

enum class OutputFormat {
    HDR = 0,          // Radiance HDR (.hdr)
    PNG10BIT = 1,     // PNG 10bit
};

// ============================================================
// 图像数据结构
// ============================================================

struct ImageData {
    float* ptr;       // 32bit float RGB 像素数据
    size_t width;
    size_t height;
    size_t stride;    // 行步长 (字节)
    size_t pixelSize; // 单像素字节数
    PixelFormat format;

    ImageData() : ptr(nullptr), width(0), height(0),
                  stride(0), pixelSize(0), format(PixelFormat::NONE) {}

    ~ImageData() {
        if (ptr) {
            free(ptr);
            ptr = nullptr;
        }
    }

    // 非拷贝，仅移动
    ImageData(const ImageData&) = delete;
    ImageData& operator=(const ImageData&) = delete;
    ImageData(ImageData&& other) noexcept {
        ptr = other.ptr; other.ptr = nullptr;
        width = other.width; height = other.height;
        stride = other.stride; pixelSize = other.pixelSize;
        format = other.format;
    }
    ImageData& operator=(ImageData&& other) noexcept {
        if (ptr) free(ptr);
        ptr = other.ptr; other.ptr = nullptr;
        width = other.width; height = other.height;
        stride = other.stride; pixelSize = other.pixelSize;
        format = other.format;
        return *this;
    }
};

// ============================================================
// 结果类型
// ============================================================

struct ConversionResult {
    bool ok;
    std::string outputPath;
    std::string error;
    std::shared_ptr<ImageData> image; // HDR 合成后的图像数据

    static ConversionResult success(const std::string& path) {
        return {true, path, "", nullptr};
    }
    static ConversionResult success(std::shared_ptr<ImageData> img) {
        return {true, "", "", std::move(img)};
    }
    static ConversionResult fail(const std::string& err) {
        return {false, "", err, nullptr};
    }
};

struct BracketResult {
    bool ok;
    std::string outputPath;
    std::string error;

    static BracketResult success(const std::string& path) {
        return {true, path, ""};
    }
    static BracketResult fail(const std::string& err) {
        return {false, "", err};
    }
};

// ============================================================
// 辅助函数
// ============================================================

namespace hdr {

/**
 * 计算 gamma 校正 (sRGB)
 */
inline float srgbToLinear(float gamma) {
    if (gamma <= 0.04045f)
        return gamma / 12.92f;
    return powf((gamma + 0.055f) / 1.055f, 2.4f);
}

/**
 * Linear → sRGB gamma
 */
inline float linearToSrgb(float linear) {
    if (linear <= 0.0031308f)
        return linear * 12.92f;
    return 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
}

/**
 * 计算曝光值 (EV)
 */
inline float computeEv(float aperture, float shutterSpeed, float iso) {
    return log2f(aperture * aperture * iso / (shutterSpeed * 1000.0f));
}

} // namespace hdr

#endif // HDR_COMMON_H
