/**
 * 10bit PNG 写入器实现
 *
 * PNG 格式支持 16bit 色深，我们可以将 10bit 数据放入 16bit 通道中
 * (低 10bit 有效，高 6bit 填充零)
 *
 * PNG 使用 zlib 压缩
 */

#include "io/Png10BitWriter.h"
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <zlib.h>
#include <cmath>
#include <algorithm>

#define LOG_TAG "Png10BitWriter"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// PNG 魔数
static const unsigned char PNG_SIG[8] = {137, 80, 78, 71, 13, 10, 26, 10};

Png10BitWriter::Png10BitWriter() {}
Png10BitWriter::~Png10BitWriter() {}

void Png10BitWriter::setCompressionLevel(int level) {
    compressionLevel_ = std::max(0, std::min(9, level));
}

ConversionResult Png10BitWriter::write(
    const float* data,
    size_t width,
    size_t height,
    const char* outputPath) {

    LOGD("Writing 10bit PNG: %zu x %zu -> %s", width, height, outputPath);

    if (!data || !outputPath || width == 0 || height == 0) {
        return ConversionResult::fail("Invalid parameters");
    }

    int fd = open(outputPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return ConversionResult::fail("Failed to open output file");
    }

    // 写入 PNG 魔数
    if (::write(fd, PNG_SIG, 8) != 8) {
        close(fd);
        return ConversionResult::fail("Failed to write PNG signature");
    }

    // 写入 IHDR chunk
    if (!writeIhdrChunk(fd, width, height)) {
        close(fd);
        return ConversionResult::fail("Failed to write IHDR chunk");
    }

    // 分配 16bit 数据缓冲区 (10bit 有效数据)
    size_t pixels = width * height;
    unsigned short* pngData = new unsigned short[pixels * 3]; // RGB

    // 转换 float → 10bit
    convertTo10bit(data, pngData, pixels * 3);

    // 压缩数据
    if (!writeCompressedRows(fd, pngData, width, height)) {
        close(fd);
        delete[] pngData;
        return ConversionResult::fail("Failed to write image data");
    }

    delete[] pngData;

    // 写入 IEND chunk
    if (!writeIendChunk(fd)) {
        close(fd);
        return ConversionResult::fail("Failed to write IEND chunk");
    }

    close(fd);
    LOGD("10bit PNG write completed: %s", outputPath);
    return ConversionResult::success(outputPath);
}

void Png10BitWriter::convertTo10bit(
    const float* src,
    unsigned short* dst,
    size_t count) {

    for (size_t i = 0; i < count; i++) {
        float val = src[i];
        // 钳制到 [0, 1]
        val = std::max(0.0f, std::min(1.0f, val));
        // 转换为 10bit (0-1023)
        dst[i] = static_cast<unsigned short>(val * 1023.0f);
    }
}

bool Png10BitWriter::writeIhdrChunk(int fd, size_t width, size_t height) {
    // IHDR chunk: width, height, bit depth, color type, compression, filter, interlace
    unsigned char ihdr[13] = {
        0, 0, 0, 0,           // width (4 bytes big-endian)
        0, 0, 0, 0,           // height
        16,                   // bit depth: 16
        2,                    // color type: RGB
        0,                    // compression: deflate
        0,                    // filter: adaptive
        0                     // interlace: none
    };

    // 写入宽度和高度 (大端序)
    ihdr[0] = (width >> 24) & 0xFF;
    ihdr[1] = (width >> 16) & 0xFF;
    ihdr[2] = (width >> 8) & 0xFF;
    ihdr[3] = width & 0xFF;

    ihdr[4] = (height >> 24) & 0xFF;
    ihdr[5] = (height >> 16) & 0xFF;
    ihdr[6] = (height >> 8) & 0xFF;
    ihdr[7] = height & 0xFF;

    // 写入 chunk: length, type, data, CRC
    uint32_t length = 13;
    unsigned char lengthBytes[4];
    lengthBytes[0] = (length >> 24) & 0xFF;
    lengthBytes[1] = (length >> 16) & 0xFF;
    lengthBytes[2] = (length >> 8) & 0xFF;
    lengthBytes[3] = length & 0xFF;

    if (::write(fd, lengthBytes, 4) != 4) return false;

    // Chunk type "IHDR"
    const char* type = "IHDR";
    if (::write(fd, type, 4) != 4) return false;

    // Chunk data
    if (::write(fd, ihdr, 13) != 13) return false;

    // CRC (简化: 实际项目应该计算正确 CRC)
    // 这里写入一个占位 CRC (实际使用时需要 zlib 计算)
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const unsigned char*>(type), 4);
    crc = crc32(crc, ihdr, 13);

    unsigned char crcBytes[4];
    crcBytes[0] = (crc >> 24) & 0xFF;
    crcBytes[1] = (crc >> 16) & 0xFF;
    crcBytes[2] = (crc >> 8) & 0xFF;
    crcBytes[3] = crc & 0xFF;

    if (::write(fd, crcBytes, 4) != 4) return false;

    LOGD("IHDR chunk written: %zu x %zu", width, height);
    return true;
}

bool Png10BitWriter::writeCompressedRows(
    int fd,
    unsigned short* data,
    size_t width,
    size_t height) {

    // 准备 zlib 压缩
    z_stream strm;
    memset(&strm, 0, sizeof(z_stream));

    if (deflateInit(&strm, compressionLevel_) != Z_OK) {
        LOGE("zlib init failed");
        return false;
    }

    size_t rowSize = width * 3 * 2; // 3 channels * 2 bytes (16bit)
    unsigned char* rowData = new unsigned char[rowSize + 1]; // +1 for filter byte
    unsigned char* compressed = new unsigned char[65536];

    for (size_t y = 0; y < height; y++) {
        // 添加 filter 字节 (0 = None)
        rowData[0] = 0;

        // 复制行数据 (16bit RGB)
        unsigned short* row = data + y * width * 3;
        unsigned char* dst = rowData + 1;
        for (size_t x = 0; x < width * 3; x++) {
            dst[x * 2] = (row[x] >> 8) & 0xFF;
            dst[x * 2 + 1] = row[x] & 0xFF;
        }

        // 压缩
        strm.avail_in = rowSize + 1;
        strm.next_in = rowData;

        do {
            strm.avail_out = 65536;
            strm.next_out = compressed;

            int ret = deflate(&strm, Z_SYNC_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                LOGE("deflate failed: %d", ret);
                deflateEnd(&strm);
                delete[] rowData;
                delete[] compressed;
                return false;
            }

            size_t outLen = 65536 - strm.avail_out;
            if (::write(fd, compressed, outLen) != (ssize_t)outLen) {
                deflateEnd(&strm);
                delete[] rowData;
                delete[] compressed;
                return false;
            }
        } while (strm.avail_in > 0);
    }

    // 最终 flush
    do {
        strm.avail_out = 65536;
        strm.next_out = compressed;

        int ret = deflate(&strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&strm);
            delete[] rowData;
            delete[] compressed;
            return false;
        }

        size_t outLen = 65536 - strm.avail_out;
        if (::write(fd, compressed, outLen) != (ssize_t)outLen) {
            deflateEnd(&strm);
            delete[] rowData;
            delete[] compressed;
            return false;
        }
    } while (strm.avail_out == 0);

    deflateEnd(&strm);
    delete[] rowData;
    delete[] compressed;

    LOGD("Compressed rows written: %zu x %zu", width, height);
    return true;
}

bool Png10BitWriter::writeIendChunk(int fd) {
    // IEND chunk: 0 length, "IEND", CRC
    uint32_t length = 0;
    unsigned char lengthBytes[4] = {0, 0, 0, 0};
    if (::write(fd, lengthBytes, 4) != 4) return false;

    const char* type = "IEND";
    if (::write(fd, type, 4) != 4) return false;

    // CRC of "IEND"
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const unsigned char*>(type), 4);

    unsigned char crcBytes[4];
    crcBytes[0] = (crc >> 24) & 0xFF;
    crcBytes[1] = (crc >> 16) & 0xFF;
    crcBytes[2] = (crc >> 8) & 0xFF;
    crcBytes[3] = crc & 0xFF;

    if (::write(fd, crcBytes, 4) != 4) return false;

    LOGD("IEND chunk written");
    return true;
}
