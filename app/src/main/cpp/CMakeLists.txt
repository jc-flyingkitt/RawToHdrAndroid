cmake_minimum_required(VERSION 3.22.1)
project(hdr-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 编译优化
add_definitions(-O3 -ffast-math -funroll-loops)
add_definitions(-fvisibility=hidden)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/libhdr/include)
include_directories(${CMAKE_SOURCE_DIR}/libraw/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# ============================================================
# 构建 HDR 引擎库 (libhdr-engine.so)
# ============================================================

add_library(hdr-engine SHARED
    src/jni/JniInterface.cpp
    src/engine/HdrEngine.cpp
    src/engine/HdrSynthesis.cpp
    src/engine/ToneMapper.cpp
    src/engine/ImageAligner.cpp
    src/io/HdrWriter.cpp
    src/io/Png10BitWriter.cpp
    src/io/LibRawDecoder.cpp
)

# 链接库
target_link_libraries(hdr-engine
    log
    z
)

# ============================================================
# libraw 库 (静态编译)
# ============================================================

# libraw 源代码目录 (需用户从 https://github.com/LibRaw/LibRaw 下载)
# 解压后放在 app/src/main/cpp/libraw/ 目录下
set(LIBRAW_SRC_DIR ${CMAKE_SOURCE_DIR}/libraw/src/libraw)

file(GLOB LIBRAW_SOURCES
    ${LIBRAW_SRC_DIR}/libraw_c_api.cpp
    ${LIBRAW_SRC_DIR}/libraw_ip.cpp
    ${LIBRAW_SRC_DIR}/dcraw_common.cpp
    ${LIBRAW_SRC_DIR}/dcraw_dcraw.cpp
    ${LIBRAW_SRC_DIR}/dcraw_fast.cpp
    ${LIBRAW_SRC_DIR}/lut_gamma.cpp
    ${LIBRAW_SRC_DIR}/lut_rgb_curve.cpp
    ${LIBRAW_SRC_DIR}/memstream.cpp
)

add_library(libraw-static STATIC ${LIBRAW_SOURCES})

# libraw 依赖
target_include_directories(libraw-static PUBLIC
    ${CMAKE_SOURCE_DIR}/libraw/include
)

# libraw 编译选项
target_compile_definitions(libraw-static PRIVATE
    -DLIBRAW_BUILD_VERSION="0.21.3-Android"
    -D__STDC_LIMIT_MACROS
    -D__STDC_CONSTANT_MACROS
)

target_link_libraries(hdr-engine libraw-static)

# ============================================================
# 安装规则
# ============================================================

install(TARGETS hdr-engine DESTINATION lib)
