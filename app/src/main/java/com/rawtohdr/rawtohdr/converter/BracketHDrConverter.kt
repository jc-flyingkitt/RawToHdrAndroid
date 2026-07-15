package com.rawtohdr.rawtohdr.converter

import android.util.Log

/**
 * 包围曝光合成 HDR 转换器
 * 接收多张不同曝光的 RAW 文件，合成高动态范围 HDR 图像
 */
class BracketHDrConverter {

    companion object {
        const val TAG = "BracketHDRConverter"
        const val MIN_FILES = 3
        const val MAX_FILES = 9
    }

    private val engine = NativeHdrEngine()

    /**
     * 包围曝光合成
     * @param files RAW 文件路径列表 (按曝光顺序排列)
     * @param outputFormats 输出格式列表
     * @param alignImages 是否启用图像对齐
     * @param toneMapping 是否启用色调映射
     */
    fun convertBracketed(
        files: List<String>,
        outputFormats: List<OutputFormat>,
        alignImages: Boolean = true,
        toneMapping: Boolean = false
    ): ConversionResult {
        // 验证文件数量
        if (files.size < MIN_FILES) {
            return ConversionResult.Error("需要至少 $MIN_FILES 张 RAW 文件")
        }
        if (files.size > MAX_FILES) {
            return ConversionResult.Error("最多支持 $MAX_FILES 张 RAW 文件")
        }

        val baseOutputPath = files.first().substringBeforeLast(".").replace("/DCIM", "/RawToHdr")
        val hdrOutput = "$baseOutputPath.bracket.hdr"
        val pngOutput = "$baseOutputPath.bracket.png"

        try {
            Log.d(TAG, "Starting bracketed HDR conversion: ${files.size} files")
            Log.d(TAG, "Files: $files")

            val json = engine.convertBracketed(
                rawPaths = files.toTypedArray(),
                outputPathHdr = hdrOutput,
                outputPathPng10 = pngOutput,
                align = alignImages
            )

            // 解析结果
            return parseBracketResult(json)
        } catch (e: Exception) {
            Log.e(TAG, "Bracketed HDR conversion failed", e)
            return ConversionResult.Error(e.message ?: "Conversion failed")
        }
    }

    private fun parseBracketResult(json: String): ConversionResult {
        return try {
            // 简单解析 JSON
            if (json.contains("\"success\":true") || json.contains("success:true")) {
                ConversionResult.Success(
                    json.substringAfter("outputPath\":\"").substringBefore("\""),
                    OutputFormat.HDR
                )
            } else {
                val msg = json.substringAfter("error\":\"").substringBefore("\"")
                ConversionResult.Error(msg)
            }
        } catch (e: Exception) {
            ConversionResult.Error("Failed to parse result: $json")
        }
    }

    fun release() {
        engine.release()
    }
}
