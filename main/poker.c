#include "poker.h"
#include <string.h>

/* ========= RGB565 工具 ========= */
static inline uint8_t r5(uint16_t c) { return (c >> 11) & 0x1F; }
static inline uint8_t g6(uint16_t c) { return (c >> 5) & 0x3F; }
static inline uint8_t b5(uint16_t c) { return c & 0x1F; }

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

/* ========= 滤镜实现 ========= */

/* 0️⃣ 原图 */
static void filter_none(uint16_t *buf, int w, int h)
{
    (void)buf;
    (void)w;
    (void)h;
}

/* 1️⃣ 童年失焦 */
static void filter_soft_memory(uint16_t *buf, int w, int h)
{
    for (int y = 1; y < h - 1; y++)
    {
        for (int x = 1; x < w - 1; x++)
        {
            int i = y * w + x;
            uint16_t c = buf[i];
            uint16_t c2 = buf[i - 1];
            uint16_t c3 = buf[i + 1];

            uint8_t r = (r5(c) + r5(c2) + r5(c3)) / 3;
            uint8_t g = (g6(c) + g6(c2) + g6(c3)) / 3;
            uint8_t b = (b5(c) + b5(c2) + b5(c3)) / 3;

            r = (r * 3 + 31) / 4;
            g = (g * 3 + 63) / 4;
            b = (b * 3 + 31) / 4;

            buf[i] = rgb565(r, g, b);
        }
    }
}

/* 2️⃣ 油画 */
static void filter_oil(uint16_t *buf, int w, int h)
{
    for (int y = 1; y < h - 1; y++)
    {
        for (int x = 1; x < w - 1; x++)
        {
            int i = y * w + x;
            uint16_t c1 = buf[i];
            uint16_t c2 = buf[i - w];
            uint16_t c3 = buf[i + w];

            uint8_t r = (r5(c1) + r5(c2) + r5(c3)) / 3;
            uint8_t g = (g6(c1) + g6(c2) + g6(c3)) / 3;
            uint8_t b = (b5(c1) + b5(c2) + b5(c3)) / 3;

            buf[i] = rgb565(r, g, b);
        }
    }
}

/* 3️⃣ 彩色网点 */
static void filter_halftone(uint16_t *buf, int w, int h)
{
    const int step = 4;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int i = y * w + x;
            if ((x % step) || (y % step))
            {
                buf[i] = rgb565(31, 63, 31);
            }
            else
            {
                uint16_t c = buf[i];
                uint8_t r = r5(c) & 0x18;
                uint8_t g = g6(c) & 0x38;
                uint8_t b = b5(c) & 0x18;
                buf[i] = rgb565(r, g, b);
            }
        }
    }
}

/* 4️⃣ Dreamcore */
static void filter_dreamcore(uint16_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        int r = r5(c) * 9 / 10 + 2;
        int g = g6(c) * 9 / 10 + 2;
        int b = b5(c) * 10 / 9;

        if (r > 31)
        {
            r = 31;
        }
        if (g > 63)
        {
            g = 63;
        }
        if (b > 31)
        {
            b = 31;
        }
        buf[i] = rgb565(r, g, b);
    }
}

/* 5️⃣ 暖色 */
static void filter_warm(uint16_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        int r = r5(c) + 2;
        int g = g6(c) + 1;
        if (r > 31)
        {
            r = 31;
        }

        if (g > 63)
        {
            g = 63;
        }
        buf[i] = rgb565(r, g, b5(c));
    }
}

/* 6️⃣ 冷色 */
static void filter_cold(uint16_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        int b = b5(c) + 2;
        if (b > 31)
            b = 31;
        buf[i] = rgb565(r5(c), g6(c), b);
    }
}

/* 7️⃣ VHS */
static void filter_vhs(uint16_t *buf, int w, int h)
{
    for (int y = 0; y < h; y++)
    {
        int noise = (y & 1) ? 1 : -1;
        for (int x = 0; x < w; x++)
        {
            int i = y * w + x;
            uint16_t c = buf[i];
            int r = r5(c) + noise, g = g6(c) + noise, b = b5(c) + noise;
            if (r < 0)
                r = 0;
            if (g < 0)
                g = 0;
            if (b < 0)
                b = 0;
            if (r > 31)
                r = 31;
            if (g > 63)
                g = 63;
            if (b > 31)
                b = 31;
            buf[i] = rgb565(r, g, b);
        }
    }
}

/* 8️⃣ 像素风 */
static void filter_pixel(uint16_t *buf, int w, int h)
{
    const int step = 4;
    for (int y = 0; y < h; y += step)
    {
        for (int x = 0; x < w; x += step)
        {
            uint16_t c = buf[y * w + x];
            for (int dy = 0; dy < step; dy++)
                for (int dx = 0; dx < step; dx++)
                    if (y + dy < h && x + dx < w)
                        buf[(y + dy) * w + (x + dx)] = c;
        }
    }
}

/* 9️⃣ CCTV */
static void filter_cctv(uint16_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        uint8_t g = (r5(c) + g6(c) + b5(c)) / 3;
        buf[i] = rgb565(0, g, 0);
    }
}

/* 🔟 Lomo */
static void filter_lomo(uint16_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        int r = r5(c) + 1;
        int b = b5(c) - 1;
        if (r > 31)
            r = 31;
        if (b < 0)
            b = 0;
        buf[i] = rgb565(r, g6(c), b);
    }
}

/* ========= 单色滤镜 ========= */
static void filter_mono_color(uint16_t *buf, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = buf[i];
        uint8_t gray = (r5(c) * 2 + g6(c) * 3 + b5(c) * 2) / 7;
        buf[i] = (gray > 20) ? rgb565(r, g, b) : rgb565(0, 0, 0);
    }
}

/* ========= 调度入口 ========= */
void poker_apply_filter(uint16_t *buf, int w, int h, poker_filter_t id)
{
    switch (id)
    {
    case POKER_FILTER_SOFT_MEMORY:
        filter_soft_memory(buf, w, h);
        break;
    case POKER_FILTER_OIL:
        filter_oil(buf, w, h);
        break;
    case POKER_FILTER_HALFTONE:
        filter_halftone(buf, w, h);
        break;
    case POKER_FILTER_DREAMCORE:
        filter_dreamcore(buf, w, h);
        break;
    case POKER_FILTER_WARM:
        filter_warm(buf, w, h);
        break;
    case POKER_FILTER_COLD:
        filter_cold(buf, w, h);
        break;
    case POKER_FILTER_VHS:
        filter_vhs(buf, w, h);
        break;
    case POKER_FILTER_PIXEL:
        filter_pixel(buf, w, h);
        break;
    case POKER_FILTER_CCTV:
        filter_cctv(buf, w, h);
        break;
    case POKER_FILTER_LOMO:
        filter_lomo(buf, w, h);
        break;

    case POKER_FILTER_MONO_RED:
        filter_mono_color(buf, w, h, 31, 0, 0);
        break;
    case POKER_FILTER_MONO_YELLOW:
        filter_mono_color(buf, w, h, 31, 63, 0);
        break;
    case POKER_FILTER_MONO_BLUE:
        filter_mono_color(buf, w, h, 0, 0, 31);
        break;
    case POKER_FILTER_MONO_BLACK:
        memset(buf, 0, w * h * 2);
        break;
    case POKER_FILTER_MONO_WHITE:
        for (int i = 0; i < w * h; i++)
            buf[i] = rgb565(31, 63, 31);
        break;

    case POKER_FILTER_NONE:
    default:
        break;
    }
}

/* ========= 名称 ========= */
const char *poker_filter_name(int id)
{
    switch (id)
    {
    case POKER_FILTER_NONE:
        return "原图";
    case POKER_FILTER_SOFT_MEMORY:
        return "童年失焦";
    case POKER_FILTER_OIL:
        return "油画";
    case POKER_FILTER_HALFTONE:
        return "彩色网点";
    case POKER_FILTER_DREAMCORE:
        return "Dreamcore";
    case POKER_FILTER_WARM:
        return "暖色复古";
    case POKER_FILTER_COLD:
        return "冷色忧郁";
    case POKER_FILTER_VHS:
        return "VHS 噪点";
    case POKER_FILTER_PIXEL:
        return "像素风";
    case POKER_FILTER_CCTV:
        return "监控绿";
    case POKER_FILTER_LOMO:
        return "Lomo";
    case POKER_FILTER_MONO_RED:
        return "单色红";
    case POKER_FILTER_MONO_YELLOW:
        return "单色黄";
    case POKER_FILTER_MONO_BLUE:
        return "单色蓝";
    case POKER_FILTER_MONO_BLACK:
        return "纯黑";
    case POKER_FILTER_MONO_WHITE:
        return "纯白";
    default:
        return "未知滤镜";
    }
}
