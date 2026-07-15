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

# ============================================================
# libraw 库 (静态编译)
# ============================================================

# libraw 源代码目录
set(LIBRAW_SRC_DIR ${CMAKE_SOURCE_DIR}/libraw/src/libraw)

# 递归搜索所有子目录中的 .cpp 文件（libraw 0.21.3 有 decoders/ demosaic/ metadata/ 等子目录）
file(GLOB_RECURSE LIBRAW_SOURCES
    ${LIBRAW_SRC_DIR}/*.cpp
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

# 链接库（合并为一次调用，保证链接顺序）
target_link_libraries(hdr-engine
    libraw-static
    log
    z
)

# ============================================================
# 安装规则
# ============================================================

install(TARGETS hdr-engine DESTINATION lib)
