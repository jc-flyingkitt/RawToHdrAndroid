# RawToHdr Android - 佳能 RAW 转 Ultra HDR

## 项目概述

将佳能 CR2/CR3 RAW 格式文件批量转换为 Google HDR (.hdr) 标准格式和 10bit PNG，支持无损转换。

## 功能特性

- **输入格式**: CR2 / CR3 (佳能 EOS 全系列 RAW)
- **输出格式**:
  - `.hdr` — Radiance HDR 标准格式 (32bit float RGBE, 无损)
  - `.png` — 10bit 高色深 PNG (16bit 通道, 低 10bit 有效)
- **转换模式**:
  - 单张 RAW 转 HDR (线性输出)
  - 包围曝光合成 HDR (3-9 张不同曝光 RAW 合成)
- **平台**: Android 8.0+ (API 26+), arm64-v8a 优先
- **兼容性**: 小米、荣耀、一加等主流品牌

## 技术架构

```
┌─────────────────────────────────────────────────┐
│                   Android UI                    │
│              (Kotlin + Material3)               │
├─────────────────────────────────────────────────┤
│                 JNI Bridge                      │
│           (JniInterface.cpp)                    │
├─────────────────────────────────────────────────┤
│              C++ HDR Engine                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │LibRaw    │  │HdrSynth  │  │ ToneMapper   │  │
│  │Decoder   │  │Engine    │  │ (Reinhard)   │  │
│  └──────────┘  └──────────┘  └──────────────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │HdrWriter │  │PngWriter │  │ ImageAligner │  │
│  │(.hdr)    │  │(10bit)   │  │ (ORB-based)  │  │
│  └──────────┘  └──────────┘  └──────────────┘  │
└─────────────────────────────────────────────────┘
```

## 核心算法

### 1. RAW 解码 (libraw)
- 支持 CR2/CR3 全格式
- Demosaic: Biliner / AMG (Amelia) 算法
- 输出: 32bit float RGB 线性数据

### 2. HDR 合成 (Debevec 2008)
- 响应曲线估算
- 多曝光加权合并
- 保留完整动态范围

### 3. 输出格式
- **.hdr**: Radiance RGBE 格式, 大端序 32bit float
- **.png**: 16bit 通道 PNG, 10bit 有效数据

## 构建说明

### 前置要求
- Android Studio Hedgehog (2023.1.1+) 或更高
- JDK 17
- Android SDK API 35
- NDK r26+
- CMake 3.22+

### 步骤

1. **下载 libraw 源码**
```bash
cd app/src/main/cpp/libraw
git clone https://github.com/LibRaw/LibRaw.git libraw-src
# 将 libraw-src/LibRaw 目录内容复制到 app/src/main/cpp/libraw/src/libraw/
```

2. **导入项目**
```bash
# 在 Android Studio 中:
# File → New → Import Project
# 选择 RawToHdrAndroid 目录
```

3. **构建**
```bash
# 或使用命令行:
cd RawToHdrAndroid
./gradlew assembleDebug
```

4. **安装**
```bash
./gradlew installDebug
```

## 文件结构

```
RawToHdrAndroid/
├── app/
│   ├── build.gradle.kts
│   ├── proguard-rules.pro
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── java/com/rawtohdr/rawtohdr/
│       │   ├── RawToHdrApplication.kt
│       │   ├── MainActivity.kt
│       │   ├── SingleConvertActivity.kt
│       │   ├── BracketConvertActivity.kt
│       │   └── converter/
│       │       ├── ConversionResult.kt
│       │       ├── NativeHdrEngine.kt
│       │       ├── SingleRawConverter.kt
│       │       └── BracketHDrConverter.kt
│       ├── cpp/
│       │   ├── CMakeLists.txt
│       │   ├── include/
│       │   │   ├── engine/
│       │   │   │   ├── HdrEngine.h
│       │   │   │   ├── HdrSynthesis.h
│       │   │   │   ├── ToneMapper.h
│       │   │   │   ├── ImageAligner.h
│       │   │   │   └── Common.h
│       │   │   └── io/
│       │   │       ├── LibRawDecoder.h
│       │   │       ├── HdrWriter.h
│       │   │       └── Png10BitWriter.h
│       │   ├── src/
│       │   │   ├── jni/JniInterface.cpp
│       │   │   ├── engine/
│       │   │   │   ├── HdrEngine.cpp
│       │   │   │   ├── HdrSynthesis.cpp
│       │   │   │   ├── ToneMapper.cpp
│       │   │   │   └── ImageAligner.cpp
│       │   │   └── io/
│       │   │       ├── LibRawDecoder.cpp
│       │   │       ├── HdrWriter.cpp
│       │   │       └── Png10BitWriter.cpp
│       │   └── libraw/
│       │       ├── include/libraw/libraw.h
│       │       └── src/libraw/ (需要手动下载)
│       └── res/
│           ├── layout/
│           ├── values/
│           └── mipmap-*/
├── gradle/
├── build.gradle.kts
├── settings.gradle.kts
└── gradle.properties
```

## 许可证

MIT License
