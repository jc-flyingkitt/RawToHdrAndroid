package com.rawtohdr.rawtohdr.converter

import android.util.Log
import com.rawtohdr.rawtohdr.RawToHdrApplication

/**
 * 原生 HDR 引擎封装
 * 通过 JNI 调用 C++ libhdr 库执行 RAW 解码和 HDR 合成
 */
class NativeHdrEngine {

    companion object {
        const val TAG = "NativeHdrEngine"

        init {
            System.loadLibrary("hdr-engine")
        }
    }

    /**
     * 单张 RAW 转 HDR
     * @param rawPath RAW 文件路径 (CR2 或 CR3)
     * @param outputPath 输出文件路径
     * @param format 输出格式
     * @return ConversionResult
     */
    external fun convertSingleRaw(
        rawPath: String,
        outputPath: String,
        format: Int
    ): String

    /**
     * 包围曝光合成 HDR
     * @param rawPaths RAW 文件路径列表 (按曝光顺序)
     * @param outputPathHdr HDR 输出路径
     * @param outputPathPng10 PNG 10bit 输出路径
     * @param align 是否对齐图像
     * @return BracketConversionResult 的 JSON 字符串
     */
    external fun convertBracketed(
        rawPaths: Array<String>,
        outputPathHdr: String,
        outputPathPng10: String,
        align: Boolean
    ): String

    /**
     * 获取库版本
     */
    external fun getLibVersion(): String

    /**
     * 释放原生资源
     */
    external fun release()
}
