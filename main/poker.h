#ifndef __POKER_H__
#define __POKER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========= 滤镜 ID 枚举 ========= */
typedef enum {
    POKER_FILTER_NONE = 0,

    POKER_FILTER_SOFT_MEMORY,   // 童年失焦
    POKER_FILTER_OIL,           // 油画
    POKER_FILTER_HALFTONE,      // 彩色网点
    POKER_FILTER_DREAMCORE,     // Dreamcore
    POKER_FILTER_WARM,          // 暖色复古
    POKER_FILTER_COLD,          // 冷色忧郁
    POKER_FILTER_VHS,           // VHS 噪点
    POKER_FILTER_PIXEL,         // 像素风
    POKER_FILTER_CCTV,          // 监控绿
    POKER_FILTER_LOMO,          // Lomo

    /* 单色强对比 */
    POKER_FILTER_MONO_RED,
    POKER_FILTER_MONO_YELLOW,
    POKER_FILTER_MONO_BLUE,
    POKER_FILTER_MONO_BLACK,
    POKER_FILTER_MONO_WHITE,

    POKER_FILTER_MAX
} poker_filter_t;

/* ========= API ========= */

/**
 * @brief 对 RGB565 图像应用滤镜（就地修改）
 */
void poker_apply_filter(uint16_t *buf, int w, int h, poker_filter_t filter_id);

/**
 * @brief 根据滤镜 ID 获取中文名称
 */
const char *poker_filter_name(int filter_id);

#ifdef __cplusplus
}
#endif

#endif /* __POKER_H__ */
