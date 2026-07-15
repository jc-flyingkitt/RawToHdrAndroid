/**
 * JNI 接口层 - Android ↔ C++ HDR 引擎
 * 负责处理 Kotlin ↔ Native 的调用转换
 */

#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include "engine/HdrEngine.h"
#include "io/LibRawDecoder.h"
#include "io/HdrWriter.h"
#include "io/Png10BitWriter.h"

#define LOG_TAG "JniInterface"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 全局引擎实例
static std::unique_ptr<HdrEngine> g_engine;

extern "C" {

/**
 * Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_convertSingleRaw
 *
 * 参数:
 *   rawPath (java.lang.String) - RAW 文件路径
 *   outputPath (java.lang.String) - 输出文件路径
 *   format (int) - 0=HDR, 1=PNG10BIT
 */
JNIEXPORT jstring JNICALL
Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_convertSingleRaw(
    JNIEnv* env,
    jobject thiz,
    jstring rawPath,
    jstring outputPath,
    jint format) {

    if (!g_engine) {
        g_engine = std::make_unique<HdrEngine>();
        LOGD("Initialized HdrEngine");
    }

    const char* rawPathStr = env->GetStringUTFChars(rawPath, nullptr);
    const char* outputPathStr = env->GetStringUTFChars(outputPath, nullptr);

    LOGD("convertSingleRaw: %s -> %s (format=%d)", rawPathStr, outputPathStr, format);

    // 解码 RAW 文件
    RawDecoder decoder;
    auto result = decoder.decode(rawPathStr);

    if (!result.ok) {
        LOGE("RAW decode failed: %s", result.error.c_str());
        env->ReleaseStringUTFChars(rawPath, rawPathStr);
        env->ReleaseStringUTFChars(outputPath, outputPathStr);
        return env->NewStringUTF(result.error.c_str());
    }

    OutputFormat fmt = static_cast<OutputFormat>(format);
    ConversionResult convResult;

    // 执行转换
    if (fmt == OutputFormat::HDR) {
        // 将 RAW 的 linear RGB 数据直接写入 HDR 文件 (无损)
        HdrWriter writer;
        convResult = writer.write(result.image->data->ptr,
                                  result.image->width,
                                  result.image->height,
                                  outputPathStr);
    } else {
        // 写入 10bit PNG
        Png10BitWriter pngWriter;
        convResult = pngWriter.write(result.image->data->ptr,
                                     result.image->width,
                                     result.image->height,
                                     outputPathStr);
    }

    env->ReleaseStringUTFChars(rawPath, rawPathStr);
    env->ReleaseStringUTFChars(outputPath, outputPathStr);

    if (convResult.ok) {
        LOGD("Conversion successful: %s", convResult.outputPath.c_str());
        return env->NewStringUTF(convResult.outputPath.c_str());
    } else {
        LOGE("Conversion failed: %s", convResult.error.c_str());
        return env->NewStringUTF(convResult.error.c_str());
    }
}

/**
 * Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_convertBracketed
 *
 * 参数:
 *   rawPaths (java.lang.String[]) - RAW 文件路径数组
 *   outputPathHdr (java.lang.String) - HDR 输出路径
 *   outputPathPng10 (java.lang.String) - PNG 10bit 输出路径
 *   align (jboolean) - 是否对齐图像
 */
JNIEXPORT jstring JNICALL
Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_convertBracketed(
    JNIEnv* env,
    jobject thiz,
    jobjectArray rawPaths,
    jstring outputPathHdr,
    jstring outputPathPng10,
    jboolean align) {

    if (!g_engine) {
        g_engine = std::make_unique<HdrEngine>();
    }

    // 获取 RAW 文件路径列表
    int numFiles = env->GetArrayLength(rawPaths);
    std::vector<std::string> filePaths;
    filePaths.reserve(numFiles);

    for (int i = 0; i < numFiles; i++) {
        jstring file = (jstring)env->GetObjectArrayElement(rawPaths, i);
        const char* str = env->GetStringUTFChars(file, nullptr);
        filePaths.push_back(str);
        env->ReleaseStringUTFChars(file, str);
        env->DeleteLocalRef(file);
    }

    const char* hdrOutStr = env->GetStringUTFChars(outputPathHdr, nullptr);
    const char* pngOutStr = env->GetStringUTFChars(outputPathPng10, nullptr);

    LOGD("convertBracketed: %d files, align=%d", numFiles, align ? 1 : 0);
    for (const auto& f : filePaths) {
        LOGD("  - %s", f.c_str());
    }

    // 执行包围曝光合成
    BracketResult result = g_engine->bracketedConvert(
        filePaths, hdrOutStr, pngOutStr, align != JNI_FALSE
    );

    env->ReleaseStringUTFChars(outputPathHdr, hdrOutStr);
    env->ReleaseStringUTFChars(outputPathPng10, pngOutStr);

    if (result.ok) {
        std::string json = "{\"success\":true,\"outputPath\":\"" + result.outputPath + "\"}";
        LOGD("Bracketed HDR successful: %s", json.c_str());
        return env->NewStringUTF(json.c_str());
    } else {
        std::string json = "{\"success\":false,\"error\":\"" + result.error + "\"}";
        LOGE("Bracketed HDR failed: %s", json.c_str());
        return env->NewStringUTF(json.c_str());
    }
}

/**
 * Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_getLibVersion
 */
JNIEXPORT jstring JNICALL
Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_getLibVersion(
    JNIEnv* env,
    jobject thiz) {

    if (!g_engine) {
        g_engine = std::make_unique<HdrEngine>();
    }

    std::string version = g_engine->getVersion();
    LOGD("getLibVersion: %s", version.c_str());
    return env->NewStringUTF(version.c_str());
}

/**
 * Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_release
 */
JNIEXPORT void JNICALL
Java_com_rawtohdr_rawtohdr_converter_NativeHdrEngine_release(
    JNIEnv* env,
    jobject thiz) {

    if (g_engine) {
        g_engine->release();
        g_engine.reset();
        LOGD("HdrEngine released");
    }
}

} // extern "C"
