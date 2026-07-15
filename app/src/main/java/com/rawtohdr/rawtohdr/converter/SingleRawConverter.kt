package com.rawtohdr.rawtohdr.converter

import android.util.Log

/**
 * 单张 RAW 转 HDR 转换器
 */
class SingleRawConverter {

    companion object {
        const val TAG = "SingleRawConverter"
    }

    private val engine = NativeHdrEngine()

    /**
     * 转换单张 RAW 为 HDR 格式
     */
    fun convert(
        rawPath: String,
        outputFormats: List<OutputFormat>,
        baseOutputPath: String
    ): Map<OutputFormat, ConversionResult> {
        val results = mutableMapOf<OutputFormat, ConversionResult>()

        for (format in outputFormats) {
            try {
                val outputPath = buildOutputPath(baseOutputPath, format)
                val json = engine.convertSingleRaw(rawPath, outputPath, format.ordinal)

                // 解析 JSON 结果
                val result = parseConversionResult(json)
                results[format] = result
                Log.d(TAG, "Converted $rawPath -> $outputPath: $result")
            } catch (e: Exception) {
                results[format] = ConversionResult.Error(e.message ?: "Unknown error")
                Log.e(TAG, "Conversion failed for $format", e)
            }
        }

        return results
    }

    private fun buildOutputPath(basePath: String, format: OutputFormat): String {
        val name = basePath.substringBeforeLast(".")
        return "$name${format.extension}"
    }

    private fun parseConversionResult(json: String): ConversionResult {
        return try {
            val isOk = json.startsWith("\"success\"") || json.startsWith("success")
            if (isOk) {
                ConversionResult.Success(json.trim('"'), OutputFormat.HDR)
            } else {
                ConversionResult.Error(json.trim('"'))
            }
        } catch (e: Exception) {
            ConversionResult.Error("JSON parse error: $json")
        }
    }
}
