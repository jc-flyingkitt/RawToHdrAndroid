/**
 * 图像对齐器 (Image Aligner)
 * 使用 ORB 特征点进行多张图像的对齐
 * 对于手持拍摄的包围曝光图像，对齐是必要的
 */

#ifndef IMAGE_ALIGNER_H
#define IMAGE_ALIGNER_H

#include "engine/Common.h"
#include <memory>
#include <vector>

class ImageAligner {
public:
    struct AlignResult {
        bool ok;
        std::string error;
    };

    /**
     * 对齐多张图像到参考图像 (第一张)
     * @param images 输入的 RAW 解码后的图像
     * @return 对齐结果
     */
    AlignResult align(std::vector<std::shared_ptr<ImageData>>& images);

    /**
     * 设置对齐参数
     */
    void setMaxFeatures(int count);
    void setScaleFactor(float scale);

private:
    int maxFeatures_ = 500;
    float scaleFactor_ = 1.2f;

    /**
     * 检测并匹配特征点 (简化版)
     */
    bool matchFeatures(
        std::shared_ptr<ImageData> img1,
        std::shared_ptr<ImageData> img2,
        float* transformMatrix
    );

    /**
     * 应用仿射变换
     */
    std::shared_ptr<ImageData> applyTransform(
        std::shared_ptr<ImageData> image,
        const float* transformMatrix,
        size_t refWidth,
        size_t refHeight
    );
};

#endif // IMAGE_ALIGNER_H
