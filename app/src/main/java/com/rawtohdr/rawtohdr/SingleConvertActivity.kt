package com.rawtohdr.rawtohdr

import android.Manifest
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Bundle
import android.provider.DocumentsContract
import android.view.View
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.rawtohdr.rawtohdr.converter.ConversionResult
import com.rawtohdr.rawtohdr.converter.NativeHdrEngine
import com.rawtohdr.rawtohdr.converter.OutputFormat
import com.rawtohdr.rawtohdr.converter.SingleRawConverter
import com.rawtohdr.rawtohdr.databinding.ActivitySingleConvertBinding

/**
 * 单张 RAW 转 HDR 界面
 * 支持:
 * - 选择 CR2/CR3 文件
 * - 输出 HDR (.hdr) + 10bit PNG
 * - 显示转换进度
 */
class SingleConvertActivity : AppCompatActivity() {

    private lateinit var binding: ActivitySingleConvertBinding

    // 选择的 RAW 文件路径
    private var selectedRawPath: String? = null

    // 文件选择器
    private val filePickerLauncher = registerForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        if (uri != null) {
            resolveFilePath(uri)
        }
    }

    // 权限请求
    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { grantResults ->
        val allGranted = grantResults.values.all { it }
        if (!allGranted) {
            Toast.makeText(this, R.string.permission_required, Toast.LENGTH_LONG).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySingleConvertBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupUI()
    }

    private fun setupUI() {
        // 选择文件按钮
        binding.btnPickFile.setOnClickListener {
            checkPermissionsAndPickFile()
        }

        // 转换按钮
        binding.btnConvert.setOnClickListener {
            if (selectedRawPath == null) {
                Toast.makeText(this, R.string.single_pick_file, Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            performConversion()
        }
    }

    private fun checkPermissionsAndPickFile() {
        val permissions = if (android.os.Build.VERSION.SDK_INT >= 33) {
            arrayOf(Manifest.permission.READ_MEDIA_IMAGES)
        } else {
            arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE)
        }

        val missing = permissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (missing.isNotEmpty()) {
            permissionLauncher.launch(missing.toTypedArray())
        }

        // 打开文件选择器，筛选 CR2/CR3 格式
        filePickerLauncher.launch(arrayOf(
            "image/x-canon-cr2",
            "image/x-canon-cr3",
            "image/x-raw",
            "*/*" // 兜底
        ))
    }

    /**
     * 从 Content URI 解析出真实文件路径
     */
    private fun resolveFilePath(uri: Uri) {
        try {
            val docId = DocumentsContract.getDocumentId(uri)
            val parts = docId.split(":")
            val filePath = if (parts.size >= 2) {
                "/storage/${parts[0]}/${parts[1]}"
            } else {
                parts[0]
            }

            // 验证文件是否为支持的 RAW 格式
            val lowerPath = filePath.lowercase()
            if (!lowerPath.endsWith(".cr2") && !lowerPath.endsWith(".cr3")) {
                Toast.makeText(this, R.string.file_not_supported, Toast.LENGTH_LONG).show()
                return
            }

            selectedRawPath = filePath
            binding.tvFilePath.text = filePath
            binding.tvFilePath.visibility = View.VISIBLE
            binding.btnConvert.isEnabled = true

            Toast.makeText(this, "已选择: ${filePath.substringAfterLast("/")}", Toast.LENGTH_SHORT).show()
        } catch (e: Exception) {
            // 无法解析路径时，使用 ContentResolver 复制文件
            copyFileFromUri(uri)
        }
    }

    /**
     * 通过 ContentResolver 将文件复制到缓存目录
     */
    private fun copyFileFromUri(uri: Uri) {
        try {
            val cacheDir = cacheDir
            val fileName = "input_raw_${System.currentTimeMillis()}.cr2"
            val destFile = java.io.File(cacheDir, fileName)

            contentResolver.openInputStream(uri)?.use { input ->
                destFile.outputStream().use { output ->
                    input.copyTo(output)
                }
            }

            selectedRawPath = destFile.absolutePath
            binding.tvFilePath.text = selectedRawPath
            binding.tvFilePath.visibility = View.VISIBLE
            binding.btnConvert.isEnabled = true

            Toast.makeText(this, "已选择文件", Toast.LENGTH_SHORT).show()
        } catch (e: Exception) {
            Toast.makeText(this, "无法读取文件: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    /**
     * 执行转换 (后台线程)
     */
    private fun performConversion() {
        val rawPath = selectedRawPath ?: return

        // 检查输出格式
        val formats = mutableListOf<OutputFormat>()
        if (binding.cbOutputHdr.isChecked) formats.add(OutputFormat.HDR)
        if (binding.cbOutputPng10.isChecked) formats.add(OutputFormat.PNG10BIT)

        if (formats.isEmpty()) {
            Toast.makeText(this, "请至少选择一种输出格式", Toast.LENGTH_SHORT).show()
            return
        }

        // 更新 UI 状态
        binding.btnConvert.isEnabled = false
        binding.btnPickFile.isEnabled = false
        binding.progressBar.visibility = View.VISIBLE
        binding.progressBar.isIndeterminate = true
        binding.tvResult.visibility = View.GONE

        // 后台线程执行转换
        Thread {
            try {
                val converter = SingleRawConverter()
                val baseOutputPath = buildOutputBasePath(rawPath)

                val results = converter.convert(rawPath, formats, baseOutputPath)

                // 回到主线程
                runOnUiThread {
                    handleConversionResults(results)
                }
            } catch (e: Exception) {
                runOnUiThread {
                    binding.tvResult.text = "转换失败: ${e.message}"
                    binding.tvResult.visibility = View.VISIBLE
                    binding.tvResult.setTextColor(
                        ContextCompat.getColor(this, R.color.color_error)
                    )
                }
            } finally {
                runOnUiThread {
                    binding.btnConvert.isEnabled = true
                    binding.btnPickFile.isEnabled = true
                    binding.progressBar.visibility = View.GONE
                }
            }
        }.start()
    }

    /**
     * 构建输出基础路径 (在 Download 目录下)
     */
    private fun buildOutputBasePath(rawPath: String): String {
        val fileName = rawPath.substringAfterLast("/").substringBeforeLast(".")
        return "${RawToHdrApplication.outputDir}/$fileName"
    }

    /**
     * 处理转换结果 (主线程)
     */
    private fun handleConversionResults(results: Map<OutputFormat, ConversionResult>) {
        val sb = StringBuilder("✅ 转换完成\n\n")

        for ((format, result) in results) {
            when (result) {
                is ConversionResult.Success -> {
                    val formatName = when (format) {
                        OutputFormat.HDR -> "HDR (.hdr)"
                        OutputFormat.PNG10BIT -> "10bit PNG"
                    }
                    sb.appendLine("$formatName: ${result.outputPath}")
                }
                is ConversionResult.Error -> {
                    val formatName = when (format) {
                        OutputFormat.HDR -> "HDR (.hdr)"
                        OutputFormat.PNG10BIT -> "10bit PNG"
                    }
                    sb.appendLine("$formatName: ❌ ${result.message}")
                }
            }
        }

        binding.tvResult.text = sb.toString()
        binding.tvResult.visibility = View.VISIBLE
        binding.tvResult.setTextColor(
            ContextCompat.getColor(this, R.color.color_primary)
        )

        // 如果全部成功，用绿色显示
        val allSuccess = results.values.all { it is ConversionResult.Success }
        if (allSuccess) {
            binding.tvResult.setTextColor(
                ContextCompat.getColor(this, R.color.color_success)
            )
        }
    }
}