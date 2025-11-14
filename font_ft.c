#include "font_ft.h"
#include "fb.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <string.h>
#include <stdlib.h>

// 將 FreeType 灰度位圖拷貝到 framebuffer（Kindle 是 Y8）
static void fb_blit_gray8(int dst_x, int dst_y, int w, int h,
                          const uint8_t* src, int src_pitch,
                          uint8_t fg_color)
{
    (void)fg_color; // 目前未使用，避免警告

    for (int row = 0; row < h; row++) {
        const uint8_t* s = src + row * src_pitch;
        for (int col = 0; col < w; col++) {
            int x = dst_x + col;
            int y = dst_y + row;
            long location = y * finfo.line_length + x;
            fbp[location] = s[col]; // 灰度直接寫入
        }
    }
}

// UTF-8 解析輔助
static uint32_t utf8_next_codepoint(const char** p) {
    const unsigned char* s = (const unsigned char*)*p;
    if (!*s) return 0;
    uint32_t cp = 0;
    if (s[0] < 0x80) { cp = s[0]; *p += 1; return cp; }
    else if ((s[0] & 0xE0) == 0xC0) {
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *p += 2; return cp;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *p += 3; return cp;
    } else if ((s[0] & 0xF8) == 0xF0) {
        cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
             ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *p += 4; return cp;
    } else {
        *p += 1; return 0xFFFD; // 非法字元
    }
}

int ft_font_init(FTFont* font, const char* font_path, int pixel_size) {
    memset(font, 0, sizeof(*font));
    FT_Library lib = NULL;
    if (FT_Init_FreeType(&lib)) return -1;

    FT_Face face = NULL;
    if (FT_New_Face(lib, font_path, 0, &face)) {
        FT_Done_FreeType(lib);
        return -2;
    }

    if (FT_Set_Pixel_Sizes(face, 0, pixel_size)) {
        FT_Done_Face(face);
        FT_Done_FreeType(lib);
        return -3;
    }

    font->ft_lib = lib;
    font->ft_face = face;
    font->pixel_size = pixel_size;
    font->baseline = (int)face->size->metrics.ascender / 64;

    return 0;
}

void ft_font_close(FTFont* font) {
    if (font->ft_face) FT_Done_Face((FT_Face)font->ft_face);
    if (font->ft_lib) FT_Done_FreeType((FT_Library)font->ft_lib);
    memset(font, 0, sizeof(*font));
}

void ft_draw_glyph(FTFont* font, int x, int y, uint32_t codepoint, uint8_t color) {
    FT_Face face = (FT_Face)font->ft_face;
    if (!face) return;

    FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
    if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) return;
    if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) return;

    FT_GlyphSlot g = face->glyph;
    int bw = g->bitmap.width;
    int bh = g->bitmap.rows;
    int pitch = g->bitmap.pitch;

    int dst_x = x + g->bitmap_left;
    int dst_y = y + font->baseline - g->bitmap_top;

    if (bw > 0 && bh > 0) {
        fb_blit_gray8(dst_x, dst_y, bw, bh, g->bitmap.buffer, pitch, color);
    }
}

void ft_draw_text_utf8(FTFont* font, int x, int y, const char* utf8, uint8_t color) {
    FT_Face face = (FT_Face)font->ft_face;
    if (!face) return;

    const char* p = utf8;
    int pen_x = x;
    int pen_y = y;
    FT_UInt prev_gi = 0;

    while (*p) {
        uint32_t cp = utf8_next_codepoint(&p);
        if (cp == '\n') {
            pen_x = x;
            pen_y += font->pixel_size + 6;
            continue;
        }

        FT_UInt gi = FT_Get_Char_Index(face, cp);
        if (FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT)) continue;

        if (prev_gi && gi) {
            FT_Vector kern;
            if (FT_Get_Kerning(face, prev_gi, gi, FT_KERNING_DEFAULT, &kern) == 0) {
                pen_x += kern.x / 64;
            }
        }
        prev_gi = gi;

        ft_draw_glyph(font, pen_x, pen_y, cp, color);
        pen_x += face->glyph->advance.x / 64;
    }
}
