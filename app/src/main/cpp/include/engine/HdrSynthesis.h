/**
 * HDR 合成算法 (Debevec 2008)
 * 将多张不同曝光的 RAW 图像合成为高动态范围图像
 *
 * 算法原理:
 * 1. 对每张图像，建立 V(x,y,e) = c_e * R(x,y) 关系
 *    其中 c_e 是曝光值 (linear), R 是响应函数
 * 2. 使用最小时刻误差 (least-squares) 求解响应曲线
 * 3. 合并多张图像得到 HDR 像素值
 */

#ifndef HDR_SYNTHESIS_H
#define HDR_SYNTHESIS_H

#include "engine/Common.h"
#include <memory>
#include <vector>

class HdrSynthesis {
public:
    /**
     * 合成多张不同曝光的图像
     * @param images 输入的 RAW 解码后的图像 (float RGB)
     * @return 合成后的 HDR 图像
     */
    ConversionResult synthesize(std::vector<std::shared_ptr<ImageData>>& images);

    /**
     * 从单张 RAW 提取 HDR (直接线性输出)
     */
    ConversionResult singleToHdr(std::shared_ptr<ImageData> image);

private:
    /**
     * 响应曲线估算 (Debevec's method)
     * 计算每个像素在不同曝光下的响应值
     */
    void estimateResponseCurve(
        std::vector<std::shared_ptr<ImageData>>& images,
        std::vector<float>& responseCurve
    );

    /**
     * 合并多张图像 (weighted average)
     */
    std::unique_ptr<ImageData> mergeImages(
        std::vector<std::shared_ptr<ImageData>>& images,
        const std::vector<float>& weights
    );

    /**
     * 计算权重 (基于像素亮度和曝光值)
     */
    std::vector<float> computeWeights(std::vector<std::shared_ptr<ImageData>>& images);
};

#endif // HDR_SYNTHESIS_H
