/**
 * HDR 引擎主类
 * 提供包围曝光合成、单张转换等核心功能
 */

#ifndef HDR_ENGINE_H
#define HDR_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include "engine/Common.h"

class HdrEngine {
public:
    HdrEngine();
    ~HdrEngine();

    /**
     * 包围曝光合成 HDR
     * @param rawPaths RAW 文件路径列表 (按曝光顺序)
     * @param outputPathHdr HDR 输出路径
     * @param outputPathPng10 PNG 10bit 输出路径
     * @param alignImages 是否对齐图像
     * @return 合成结果
     */
    BracketResult bracketedConvert(
        const std::vector<std::string>& rawPaths,
        const char* outputPathHdr,
        const char* outputPathPng10,
        bool alignImages = true
    );

    /**
     * 单张 RAW 转 HDR (线性转换)
     */
    ConversionResult singleConvert(
        const char* rawPath,
        const char* outputPath,
        OutputFormat format
    );

    /**
     * 获取库版本
     */
    std::string getVersion() const;

    /**
     * 释放资源
     */
    void release();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

#endif // HDR_ENGINE_H
