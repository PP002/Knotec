#ifndef FONT_FT_H
#define FONT_FT_H

#include <stdint.h>

typedef struct {
    void* ft_lib;    // FT_Library
    void* ft_face;   // FT_Face
    int pixel_size;  // 字号像素高度
    int baseline;    // 基线偏移（用于对齐）
} FTFont;

// 初始化 FreeType 与字体
int ft_font_init(FTFont* font, const char* font_path, int pixel_size);

// 释放
void ft_font_close(FTFont* font);

// 渲染并绘制单个 Unicode 码点到 framebuffer（左上角位置）
void ft_draw_glyph(FTFont* font, int x, int y, uint32_t codepoint, uint8_t color);

// 绘制 UTF-8 字符串（自动解析 UTF-8）
void ft_draw_text_utf8(FTFont* font, int x, int y, const char* utf8, uint8_t color);

#endif
