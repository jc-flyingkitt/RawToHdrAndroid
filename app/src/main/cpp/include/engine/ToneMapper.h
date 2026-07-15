/**
 * 色调映射器 (Tone Mapper)
 * 将 HDR 图像映射到 SDR 显示空间 (sRGB)
 * 支持 Reinhard、Drago、Durand 等算法
 */

#ifndef TONE_MAPPER_H
#define TONE_MAPPER_H

#include "engine/Common.h"
#include <memory>

class ToneMapper {
public:
    enum class Algorithm {
        REINHARD,    // Reinhard operator (2002)
        DRAGO,       // Drago et al. (2003)
        DURAND,      // Durand et al. (2002)
        SIMPLE,      // Simple linear + gamma
    };

    ToneMapper();
    ~ToneMapper();

    /**
     * 执行色调映射
     * @param hdrImage HDR 图像 (float RGB)
     * @param algorithm 算法类型
     * @return 映射后的 SDR 图像 (8bit RGB)
     */
    std::unique_ptr<ImageData> map(
        std::shared_ptr<ImageData> hdrImage,
        Algorithm algorithm = Algorithm::REINHARD
    );

    /**
     * 设置参数
     */
    void setExposure(float exposure);
    void setSaturation(float saturation);
    void setContrast(float contrast);

private:
    float exposure_ = 1.0f;
    float saturation_ = 1.0f;
    float contrast_ = 1.0f;
};

#endif // TONE_MAPPER_H
