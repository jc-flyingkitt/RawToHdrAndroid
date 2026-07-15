/**
 * HdrEngine 实现
 */

#include "engine/HdrEngine.h"
#include "engine/HdrSynthesis.h"
#include "engine/ToneMapper.h"
#include "engine/ImageAligner.h"
#include "io/LibRawDecoder.h"
#include "io/HdrWriter.h"
#include "io/Png10BitWriter.h"
#include <android/log.h>

#define LOG_TAG "HdrEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class HdrEngine::Impl {
public:
    HdrSynthesis synthesizer;
    ToneMapper toneMapper;
    ImageAligner aligner;
};

HdrEngine::HdrEngine() : pimpl(std::make_unique<Impl>()) {
    LOGD("HdrEngine constructed");
}

HdrEngine::~HdrEngine() {
    if (pimpl) {
        LOGD("HdrEngine destroyed");
        pimpl.reset();
    }
}

BracketResult HdrEngine::bracketedConvert(
    const std::vector<std::string>& rawPaths,
    const char* outputPathHdr,
    const char* outputPathPng10,
    bool alignImages) {

    LOGD("bracketedConvert: %zu files", rawPaths.size());

    if (rawPaths.size() < 3) {
        return BracketResult::fail("Need at least 3 RAW files for bracketed HDR");
    }

    // Step 1: 解码所有 RAW 文件
    std::vector<std::shared_ptr<ImageData>> images;
    images.reserve(rawPaths.size());

    for (const auto& path : rawPaths) {
        LOGD("Decoding: %s", path.c_str());
        RawDecoder decoder;
        auto decodeResult = decoder.decode(path.c_str());

        if (!decodeResult.ok) {
            LOGE("Failed to decode %s: %s", path.c_str(), decodeResult.error.c_str());
            return BracketResult::fail("Decode failed: " + decodeResult.error);
        }

        images.push_back(std::move(decodeResult.image));
    }

    LOGD("Decoded %zu images successfully", images.size());

    // Step 2: 图像对齐 (可选)
    if (alignImages) {
        LOGD("Aligning images...");
        auto alignResult = pimpl->aligner.align(images);
        if (!alignResult.ok) {
            LOGE("Image alignment failed: %s", alignResult.error.c_str());
            return BracketResult::fail("Alignment failed: " + alignResult.error);
        }
    }

    // Step 3: HDR 合成 (基于 Debevec 算法)
    LOGD("Performing HDR synthesis...");
    auto synthesisResult = pimpl->synthesizer.synthesize(images);

    if (!synthesisResult.ok) {
        LOGE("HDR synthesis failed: %s", synthesisResult.error.c_str());
        return BracketResult::fail("Synthesis failed: " + synthesisResult.error);
    }

    // Step 4: 输出 HDR 文件
    LOGD("Writing HDR file: %s", outputPathHdr);
    HdrWriter hdrWriter;
    auto hdrWriteResult = hdrWriter.write(
        synthesisResult.image->data->ptr,
        synthesisResult.image->data->width,
        synthesisResult.image->data->height,
        outputPathHdr
    );

    if (!hdrWriteResult.ok) {
        LOGE("HDR write failed: %s", hdrWriteResult.error.c_str());
        return BracketResult::fail("HDR write failed: " + hdrWriteResult.error);
    }

    // Step 5: 输出 10bit PNG
    LOGD("Writing 10bit PNG file: %s", outputPathPng10);
    Png10BitWriter pngWriter;
    auto pngWriteResult = pngWriter.write(
        synthesisResult.image->data->ptr,
        synthesisResult.image->data->width,
        synthesisResult.image->data->height,
        outputPathPng10
    );

    if (!pngWriteResult.ok) {
        LOGE("PNG write failed: %s", pngWriteResult.error.c_str());
        return BracketResult::fail("PNG write failed: " + pngWriteResult.error);
    }

    LOGD("Bracketed HDR conversion completed successfully");
    return BracketResult::success(outputPathHdr);
}

ConversionResult HdrEngine::singleConvert(
    const char* rawPath,
    const char* outputPath,
    OutputFormat format) {

    LOGD("singleConvert: %s -> %s (format=%d)", rawPath, outputPath, static_cast<int>(format));

    // 解码 RAW
    RawDecoder decoder;
    auto decodeResult = decoder.decode(rawPath);

    if (!decodeResult.ok) {
        return ConversionResult::fail(decodeResult.error);
    }

    // 写入
    if (format == OutputFormat::HDR) {
        HdrWriter writer;
        return writer.write(
            decodeResult.image->data->ptr,
            decodeResult.image->data->width,
            decodeResult.image->data->height,
            outputPath
        );
    } else {
        Png10BitWriter writer;
        return writer.write(
            decodeResult.image->data->ptr,
            decodeResult.image->data->width,
            decodeResult.image->data->height,
            outputPath
        );
    }
}

std::string HdrEngine::getVersion() const {
    return "HdrEngine-Android v1.0.0";
}

void HdrEngine::release() {
    if (pimpl) {
        pimpl.reset();
        LOGD("HdrEngine released");
    }
}
