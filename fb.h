// fb.h
#ifndef FB_H
#define FB_H

#include <linux/fb.h>

// 讓其他檔案能看到這些全域變數
extern struct fb_var_screeninfo vinfo;
extern struct fb_fix_screeninfo finfo;
extern char *fbp;

void fb_init();
void fb_draw_rect(int x, int y, int w, int h, char color);
void fb_clear();
void fb_close();
void fb_draw_border(int x, int y, int w, int h, char color);
void fb_draw_hline_thick(int x, int y, int length, int thickness, char color);
void fb_draw_char(int x, int y, char c, char color);
void fb_put_pixel(int x, int y, char color);
void touch_init();

typedef struct {
    int screen_w;
    int screen_h;
    int key_rows;
    int key_cols;
    int key;
    int start_x;
    int box_h;
    int start_y;
    int start_key_y;
} Layout;

extern Layout layout;
void init_layout();

void poll_touch();
extern int touch_active;

#endif
