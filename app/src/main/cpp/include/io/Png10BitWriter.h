/**
 * 10bit PNG 写入器
 * 将 HDR 数据输出为 10bit 高色深 PNG 文件
 */

#ifndef PNG_10BIT_WRITER_H
#define PNG_10BIT_WRITER_H

#include "engine/Common.h"
#include <memory>

class Png10BitWriter {
public:
    Png10BitWriter();
    ~Png10BitWriter();

    /**
     * 写入 10bit PNG 文件
     * @param data 32bit float RGB 数据
     * @param width 宽度
     * @param height 高度
     * @param outputPath 输出文件路径
     * @return 写入结果
     */
    ConversionResult write(
        const float* data,
        size_t width,
        size_t height,
        const char* outputPath
    );

    /**
     * 设置压缩级别 (0-9)
     */
    void setCompressionLevel(int level);

private:
    int compressionLevel_ = 6;

    /**
     * 将 float RGB 转换为 10bit 数据 (12 bytes per pixel)
     */
    void convertTo10bit(const float* src, unsigned short* dst, size_t count);

    bool writeIhdrChunk(int fd, size_t width, size_t height);
    bool writeCompressedRows(int fd, unsigned short* data, size_t width, size_t height);
    bool writeIendChunk(int fd);
};

#endif // PNG_10BIT_WRITER_H
