/**
 * HDR 文件写入器实现
 */

#include "io/HdrWriter.h"
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <algorithm>

#define LOG_TAG "HdrWriter"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

HdrWriter::HdrWriter() {}
HdrWriter::~HdrWriter() {}

ConversionResult HdrWriter::write(
    const float* data,
    size_t width,
    size_t height,
    const char* outputPath) {

    LOGD("Writing HDR: %zu x %zu -> %s", width, height, outputPath);

    if (!data || !outputPath || width == 0 || height == 0) {
        return ConversionResult::fail("Invalid parameters");
    }

    // 打开输出文件
    int fd = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return ConversionResult::fail("Failed to open output file");
    }

    // 写入文件头
    if (!writeHeader(fd, width, height)) {
        close(fd);
        return ConversionResult::fail("Failed to write header");
    }

    // 写入像素数据
    if (!writePixels(fd, data, width, height)) {
        close(fd);
        return ConversionResult::fail("Failed to write pixels");
    }

    close(fd);
    LOGD("HDR write completed: %s", outputPath);
    return ConversionResult::success(outputPath);
}

bool HdrWriter::writeHeader(int fd, size_t width, size_t height) {
    // Radiance HDR 文件头格式
    // 第一行: "#?RADIANCE" 或 "#?RGBE"
    // 后续行: 包含 EXPOSURE, FORMAT, 等
    // 最后一行: -W width +H height

    char header[256];
    int len;

    // 检查是否需要写 "#?RADIANCE"
    const char* signature = "#?RADIANCE\n";
    if (write(fd, signature, strlen(signature)) != (ssize_t)strlen(signature)) {
        return false;
    }

    // 写入尺寸信息
    len = snprintf(header, sizeof(header), "-W %zu\n", width);
    if (write(fd, header, len) != len) return false;

    len = snprintf(header, sizeof(header), "+H %zu\n", height);
    if (write(fd, header, len) != len) return false;

    // 写入格式信息
    len = snprintf(header, sizeof(header), "FORMAT=32-bit_rle_rgbe\n");
    if (write(fd, header, len) != len) return false;

    LOGD("HDR header written: %zu x %zu", width, height);
    return true;
}

bool HdrWriter::writePixels(int fd, const float* data, size_t width, size_t height) {
    // 使用 RLE 压缩写入 (Radiance 标准格式)
    // 每行单独编码

    size_t totalPixels = width * height;
    const size_t rowSize = width * 3 * sizeof(float);

    for (size_t y = 0; y < height; y++) {
        const float* row = data + y * width * 3;

        // 将 float RGB 转换为 RGBE 格式
        // RGBE: R, G, B, E (4 bytes per pixel)
        // 其中 E 是共享指数，R=G=B=E 时压缩率最高

        unsigned char* rgbeRow = new unsigned char[width * 4];

        for (size_t x = 0; x < width; x++) {
            size_t idx = y * width * 3 + x * 3;
            float r = data[idx];
            float g = data[idx + 1];
            float b = data[idx + 2];

            // 计算指数
            float maxVal = std::max({r, g, b});
            if (maxVal < 1e-32f) {
                // 暗像素: 直接写零
                rgbeRow[x * 4 + 0] = 0;
                rgbeRow[x * 4 + 1] = 0;
                rgbeRow[x * 4 + 2] = 0;
                rgbeRow[x * 4 + 3] = 0;
            } else {
                // 标准化并计算指数
                int exponent = static_cast<int>(floor(log2f(maxVal))) + 128;
                exponent = std::max(1, std::min(255, exponent));

                float scale = ldexp(1.0f, -exponent + 128);
                unsigned char rr = static_cast<unsigned char>(r * scale * 255.0f);
                unsigned char gg = static_cast<unsigned char>(g * scale * 255.0f);
                unsigned char bb = static_cast<unsigned char>(b * scale * 255.0f);

                rgbeRow[x * 4 + 0] = std::min(255U, rr);
                rgbeRow[x * 4 + 1] = std::min(255U, gg);
                rgbeRow[x * 4 + 2] = std::min(255U, bb);
                rgbeRow[x * 4 + 3] = static_cast<unsigned char>(exponent);
            }
        }

        // RLE 编码写入
        size_t bytesWritten = 0;
        for (size_t i = 0; i < width * 4; i++) {
            unsigned char val = rgbeRow[i];
            size_t run = 1;

            // 查找连续相同字节
            while (run < width * 4 - i - 1 && rgbeRow[i + run] == val && run < 128) {
                run++;
            }

            if (run > 2) {
                // RLE 编码: 0xFF, run+128, value
                unsigned char rleHeader[3] = {0xFF, static_cast<unsigned char>(run + 128), val};
                ssize_t w = write(fd, rleHeader, 3);
                if (w != 3) { delete[] rgbeRow; return false; }
                bytesWritten += 3;
                i += run - 1;
            } else {
                // 非压缩: 0x00, count, data
                unsigned char count = 1;
                ssize_t w = write(fd, &count, 1);
                if (w != 1) { delete[] rgbeRow; return false; }
                bytesWritten++;
                w = write(fd, &val, 1);
                if (w != 1) { delete[] rgbeRow; return false; }
                bytesWritten++;
            }
        }

        delete[] rgbeRow;
    }

    LOGD("HDR pixels written: %zu x %zu", width, height);
    return true;
}

void HdrWriter::floatToBigEndian(float val, unsigned char* out) {
    // 获取 float 的字节表示
    unsigned char* src = reinterpret_cast<unsigned char*>(&val);
    // 转换为大端序
    out[0] = src[3];
    out[1] = src[2];
    out[2] = src[1];
    out[3] = src[0];
}
