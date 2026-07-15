package com.rawtohdr.rawtohdr.converter

/**
 * 输出格式枚举
 */
enum class OutputFormat(val extension: String) {
    /** Radiance HDR 标准格式 (.hdr) */
    HDR(".hdr"),

    /** 10bit PNG 高色深 */
    PNG10BIT(".png"),
}

/**
 * 转换结果
 */
sealed class ConversionResult {
    data class Success(val outputPath: String, val format: OutputFormat) : ConversionResult()
    data class Error(val message: String, val errorCode: Int = -1) : ConversionResult()
}

/**
 * 包围曝光合成结果 (多张输出)
 */
data class BracketConversionResult(
    val outputs: Map<OutputFormat, String>,
    val isSuccess: Boolean,
    val errorMessage: String? = null
)
