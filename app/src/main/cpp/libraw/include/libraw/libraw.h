/**
 * libraw 头文件 stub
 * 实际使用时需要下载完整的 libraw 库
 * 下载地址: https://github.com/LibRaw/LibRaw
 */

#ifndef LIBRAW_H
#define LIBRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * libraw 核心结构体 (stub)
 * 完整定义请参考 libraw.h
 */

typedef struct LibRaw LibRaw;

/**
 * libraw 错误码
 */
enum {
    LIBRAW_SUCCESS = 0,
    LIBRAW_ERR_FILE_NOT_FOUND = -1,
    LIBRAW_ERR_UNKNOWN_FORMAT = -2,
    LIBRAW_ERR_FILE_NOT_LOADED = -3,
    LIBRAW_ERR_OUT_OF_MEM = -4,
    LIBRAW_ERR_UNSUPPORTED = -5,
    LIBRAW_ERR_LIBOPENRAW_DEPRECATED = -100,
};

/**
 * 自动白平衡模式
 */
enum {
    LIBRAW_USE_CAMWB = 0,
    LIBRAW_USE_ASF = 1,
    LIBRAW_USE_CUSTOM = 2,
    LIBRAW_USE_KERNEL = 3,
};

/**
 * libraw 函数 (声明)
 */

// 打开文件
int LibRaw_open_file(LibRaw* lr, const char* fname);

// 解析 RAW 数据
int LibRaw_unpack(LibRaw* lr);

// 释放资源
void LibRaw_recycle(LibRaw* lr);

// 获取错误信息
const char* LibRaw_strerror(int err);

// 获取图像数据
void* LibRaw_get_raw_data(LibRaw* lr);

// 获取图像尺寸
unsigned int LibRaw_get_width(LibRaw* lr);
unsigned int LibRaw_get_height(LibRaw* lr);
unsigned int LibRaw_get_bits(LibRaw* lr);
unsigned int LibRaw_get_bayer(LibRaw* lr);

#ifdef __cplusplus
}
#endif

// C++ 接口
class LibRaw {
public:
    struct opflags_t {
        int rgb_colors_count;
    };

    struct sizes_t {
        int raw_width;
        int raw_height;
        int bayer;
        int raw_bits;
    };

    struct color_t {
        opflags_t rgb_colors_count;
    };

    struct rawdata_t {
        unsigned short* raw_image;
    };

    struct imgdata_t {
        struct {
            int use_camera_wb;
            int use_camera_prof;
            int output_bps;
            int half_size;
            float user_mul[4];
            int output_color;
            int tone_curve;
        } params;

        struct sizes_t sizes;
        struct color_t color;
        struct rawdata_t rawdata;
    } imgdata;

    int open_file(const char* fname);
    int unpack();
    void recycle();
    static const char* strerror(int err);
};

#endif // LIBRAW_H
