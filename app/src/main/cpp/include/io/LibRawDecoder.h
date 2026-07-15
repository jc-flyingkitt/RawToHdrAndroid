/**
 * libraw RAW 解码器封装
 * 支持 CR2 / CR3 格式解码
 * 输出: 32bit float RGB 线性图像
 */

#ifndef LIB_RAW_DECODER_H
#define LIB_RAW_DECODER_H

#include "engine/Common.h"
#include <memory>
#include <string>

struct DecodeResult {
    bool ok;
    std::shared_ptr<ImageData> image;
    std::string error;

    static DecodeResult success(std::shared_ptr<ImageData> img) {
        return {true, std::move(img), ""};
    }
    static DecodeResult fail(const std::string& err) {
        return {false, nullptr, err};
    }
};

class RawDecoder {
public:
    RawDecoder();
    ~RawDecoder();

    /**
     * 解码 RAW 文件
     * @param filePath CR2 或 CR3 文件路径
     * @return 解码后的图像 (32bit float RGB)
     */
    DecodeResult decode(const char* filePath);

    /**
     * 设置解码选项
     */
    void setUseCameraProfile(bool use);
    void setDemosaicAlgorithm(int algorithm);
    void setHalfSize(bool half);

private:
    bool useCameraProfile_ = true;
    int demosaicAlgorithm_ = 5; // AMG (Amelia)
    bool halfSize_ = false;
};

#endif // LIB_RAW_DECODER_H
