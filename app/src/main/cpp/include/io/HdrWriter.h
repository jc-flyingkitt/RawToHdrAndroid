/**
 * HDR 文件写入器 (Radiance HDR .hdr 格式)
 *
 * Radiance HDR 格式规范:
 * - 文件头: 包含尺寸、像素顺序等信息
 * - 像素数据: 32bit float RGB (大端序)
 * - 无损: 保留完整的动态范围信息
 */

#ifndef HDR_WRITER_H
#define HDR_WRITER_H

#include "engine/Common.h"
#include <memory>

class HdrWriter {
public:
    HdrWriter();
    ~HdrWriter();

    /**
     * 写入 HDR 文件
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

private:
    /**
     * 写入文件头
     */
    bool writeHeader(int fd, size_t width, size_t height);

    /**
     * 写入像素数据 (大端序 32bit float)
     */
    bool writePixels(int fd, const float* data, size_t width, size_t height);

    /**
     * float → 大端序字节
     */
    void floatToBigEndian(float val, unsigned char* out);
};

#endif // HDR_WRITER_H
