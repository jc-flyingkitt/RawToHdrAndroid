package com.rawtohdr.rawtohdr

import android.app.Application
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log

class RawToHdrApplication : Application() {

    companion object {
        const val TAG = "RawToHdr"
        @Volatile
        var instance: RawToHdrApplication? = null
            private set

        // 主线程 Handler
        val mainHandler: Handler by lazy { Handler(Looper.getMainLooper()) }

        // 输出目录 (SDCard/Download)
        val outputDir: String by lazy {
            val ext = instance?.externalCacheDir
            ext?.absolutePath ?: "/sdcard/Download/RawToHdr"
        }
    }

    override fun onCreate() {
        super.onCreate()
        instance = this
        Log.d(TAG, "Application started, outputDir: $outputDir")
    }
}
