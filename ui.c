#include "fb.h"
#include <string.h>
#include <stdint.h>
#include "font.h"      // 保留用于画键盘图标（控制符号）
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"
#include "font_ft.h"   // 新增

#define MAX_LEN 512
#define KEY_SPACE 0x89

extern Layout layout;

static FTFont g_font;

char input_buffer[MAX_LEN];
int input_len = 0;
int cursor_index = 0;
int cursor_x = 0;
int cursor_y = 0;

const uint8_t keys[4][12] = {
    { 'q','w','e','r','t','y','u','i','o','p', 0, 0 },
    { 'a','s','d','f','g','h','j','k','l', 0x83 , ';', 0 },
    { 0x80 , 'z','x','c','v','b','n','m', 0x86 , 0x84 , 0 },
    { 0x82 , ',', '.', KEY_SPACE, KEY_SPACE, KEY_SPACE, KEY_SPACE, 0x85 , 0x87 , 0x88 , 0 }
};

extern const uint8_t keys[4][12];

void draw_input_box() {
    fb_draw_hline_thick(0, layout.key, layout.screen_w, 5, 0x00);
    fb_draw_hline_thick(0, layout.box_h, layout.screen_w, 5, 0x00);

    cursor_x = layout.start_x;
    cursor_y = layout.start_y;
}

void draw_keyboard() {
    for (int row = 0; row < layout.key_rows; row++) {
        for (int col = 0; col < layout.key_cols; col++) {
            uint8_t ch = keys[row][col];
            if (ch != 0) {
                int x = layout.start_x + col * layout.key + layout.key / 4;
                int y = layout.start_key_y + row * layout.key + layout.key / 4;
                // 键盘仍使用 8x8 控制符号画（你已有的函数）
                fb_draw_char(x, y, ch, 0x00);
            }
        }
    }
}

// 初始化 FreeType 字体（在 main.c 调用）
int ui_init_freetype(const char* font_path, int pixel_size) {
    return ft_font_init(&g_font, font_path, pixel_size);
}

void input_char(uint8_t ch) {
    // 清除旧光标（用覆盖法）
    // 先擦输入区域
    fb_draw_rect(layout.start_x, layout.start_y,
                 layout.screen_w - layout.start_x * 2,
                 layout.box_h - 10, 0xFF);

    // 处理特殊键
    if (ch == 0x83) {  // Backspace
        if (cursor_index > 0) {
            memmove(&input_buffer[cursor_index - 1], &input_buffer[cursor_index], input_len - cursor_index);
            input_len--;
            cursor_index--;
        }
    } else if (ch == 0x84) {  // Enter
        if (input_len < MAX_LEN - 1) {
            memmove(&input_buffer[cursor_index + 1], &input_buffer[cursor_index], input_len - cursor_index);
            input_buffer[cursor_index] = '\n';
            input_len++;
            cursor_index++;
        }
    } else if (ch >= 0x85 && ch <= 0x88) {  // Arrow keys
        if (ch == 0x85 && cursor_index > 0) cursor_index--;          // ←
        if (ch == 0x88 && cursor_index < input_len) cursor_index++;  // →
    } else {
        if (ch == KEY_SPACE) ch = ' ';
        if (input_len < MAX_LEN - 1) {
            memmove(&input_buffer[cursor_index + 1], &input_buffer[cursor_index], input_len - cursor_index);
            input_buffer[cursor_index] = ch;
            input_len++;
            cursor_index++;
        }
    }

    // 重新绘制整段文本（ASCII -> UTF-8 同步）
    // 注意：这里 input_buffer 是原始字节；我们直接当作 UTF-8 使用
    int x = layout.start_x;
    int y = layout.start_y;
    // 截取到 cursor_index 的字符串用于定位光标（简单实现：全部绘制）
    char temp[MAX_LEN + 1];
    memcpy(temp, input_buffer, input_len);
    temp[input_len] = '\0';
    ft_draw_text_utf8(&g_font, x, y, temp, 0x00);

    // 简单光标：在末尾绘制一个下划线（用位图方法）
    // 这里可以通过测量宽度来计算光标位置，但为简化，我们在末尾画一条细横线
    // 你也可以用 ft 计算最后一个字的 advance 求出光标 x
    // 先粗略刷新即可
    int rc = system("/usr/bin/fbink -s");
    (void)rc;

    if (ch == 0x80) { // Shift/退出键
        save_input_to_file("/mnt/us/input.txt");
        exit(0);
    }
}

void save_input_to_file(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open file for writing");
        return;
    }
    fwrite(input_buffer, 1, input_len, fp);
    fclose(fp);
}

void handle_touch(int x, int y) {
    for (int row = 0; row < layout.key_rows; row++) {
        for (int col = 0; col < layout.key_cols; col++) {
            int key_x = layout.start_x + col * layout.key;
            int key_y = layout.start_key_y + row * layout.key;

            if (x >= key_x && x < key_x + layout.key &&
                y >= key_y && y < key_y + layout.key) {

                uint8_t ch = keys[row][col];
                if (ch != 0) {
                    input_char(ch);
                }
            }
        }
    }
}
