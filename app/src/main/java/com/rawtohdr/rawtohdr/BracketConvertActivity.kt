package com.rawtohdr.rawtohdr

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.rawtohdr.rawtohdr.databinding.ActivityBracketConvertBinding
import com.rawtohdr.rawtohdr.converter.BracketHDrConverter
import com.rawtohdr.rawtohdr.converter.ConversionResult
import com.rawtohdr.rawtohdr.converter.NativeHdrEngine
import com.rawtohdr.rawtohdr.converter.OutputFormat

class BracketConvertActivity : AppCompatActivity() {

    private lateinit var binding: ActivityBracketConvertBinding
    private val selectedFiles = mutableListOf<String>()
    private lateinit var converter: BracketHDrConverter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityBracketConvertBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // 初始化原生引擎
        converter = BracketHDrConverter()

        setupUI()
    }

    override fun onDestroy() {
        super.onDestroy()
        converter.release()
    }

    private fun setupUI() {
        binding.btnAddFiles.setOnClickListener {
            // 选择多个 RAW 文件 (3-5 张不同曝光)
            selectRawFiles()
        }

        binding.btnMerge.setOnClickListener {
            if (selectedFiles.size < 3) {
                Toast.makeText(this, R.string.need_3_or_more, Toast.LENGTH_LONG).show()
                return@setOnClickListener
            }
            performConversion()
        }
    }

    private fun selectRawFiles() {
        // 使用文件选择器选择 RAW 文件
        // 实现文件选择逻辑，支持 CR2/CR3 扩展名
        Toast.makeText(this, "选择 RAW 文件", Toast.LENGTH_SHORT).show()
    }

    private fun performConversion() {
        binding.progressBar.visibility = View.VISIBLE
        binding.btnMerge.isEnabled = false

        // 在后台线程执行转换
        Thread {
            val result = converter.convertBracketed(
                files = selectedFiles,
                outputFormats = listOf(OutputFormat.HDR, OutputFormat.PNG10BIT),
                alignImages = binding.switchAlign.isChecked,
                toneMapping = binding.switchToneMap.isChecked
            )

            // 回到主线程更新 UI
            runOnUiThread {
                binding.progressBar.visibility = View.GONE
                binding.btnMerge.isEnabled = true

                when (result) {
                    is ConversionResult.Success -> {
                        Toast.makeText(this, R.string.bracket_success, Toast.LENGTH_LONG).show()
                        // 显示输出文件路径
                    }
                    is ConversionResult.Error -> {
                        Toast.makeText(this, "${R.string.bracket_error}: ${result.message}", Toast.LENGTH_LONG).show()
                    }
                }
            }
        }.start()
    }
}
