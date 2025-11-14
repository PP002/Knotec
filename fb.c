#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "fb.h"
#include <string.h>
#include "font.h"
#include <linux/input.h>
#include "ui.h"

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

Layout layout;

void init_layout() {
    layout.screen_w = vinfo.xres;
    layout.screen_h = vinfo.yres;
    layout.key_rows = 4;
    layout.key_cols = 10;
    layout.key = layout.screen_w / (layout.key_cols + 1);
    layout.start_x = layout.key / 2;
    layout.start_y = layout.start_x;
    layout.box_h = layout.screen_h - ((layout.key_rows + 1) * layout.key);
    layout.start_key_y = layout.box_h + layout.start_x;
}

int fb_fd;
char *fbp;

void fb_init() {
    fb_fd = open("/dev/fb0", O_RDWR);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    long screensize = vinfo.yres_virtual * finfo.line_length;
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
}

void fb_clear() {
    size_t screensize = (size_t)vinfo.yres_virtual * finfo.line_length;
    memset(fbp, 0xFF, screensize); // 白色背景
}

void fb_draw_rect(int x, int y, int w, int h, char color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            long location = j * finfo.line_length + i;
            fbp[location] = color;
        }
    }
}

void fb_draw_hline_thick(int x, int y, int length, int thickness, char color) {
    for (int t = 0; t < thickness; t++) {
        for (int i = 0; i < length; i++) {
            long location = (y + t) * finfo.line_length + (x + i);
            fbp[location] = color;
        }
    }
}

void fb_put_pixel(int x, int y, char color) {
    long location = y * finfo.line_length + x;
    fbp[location] = color;
}

extern unsigned char font8x8_basic[128][8]; // 字型表

#define FONT_SCALE 4

void fb_draw_char(int x, int y, char c, char color) {
    unsigned char row_data;
    for (int row = 0; row < 8; row++) {
        if ((unsigned char)c >= 0x80 && (unsigned char)c <= 0x9F)
            row_data = font8x8_control[(unsigned char)c - 0x80][row];
        else
            row_data = font8x8_basic[(unsigned char)c][row];

        for (int col = 0; col < 8; col++) {
            if (row_data & (1 << (7 - col))) {
                for (int dy = 0; dy < FONT_SCALE; dy++) {
                    for (int dx = 0; dx < FONT_SCALE; dx++) {
                        int px = x + col * FONT_SCALE + dx;
                        int py = y + row * FONT_SCALE + dy;
                        fb_put_pixel(px, py, color);
                    }
                }
            }
        }
    }
}

void fb_draw_border(int x, int y, int w, int h, char color) {
    fb_draw_rect(x, y, w, 1, color);
    fb_draw_rect(x, y + h - 1, w, 1, color);
    fb_draw_rect(x, y, 1, h, color);
    fb_draw_rect(x + w - 1, y, 1, h, color);
}

int touch_fd;
int touch_active = 0;

void touch_init() {
    touch_fd = open("/dev/input/event2", O_RDONLY);
    if (touch_fd < 0) {
        perror("Failed to open touch device");
        exit(1);
    }
    if (ioctl(touch_fd, EVIOCGRAB, 1) < 0) {
        perror("EVIOCGRAB failed");
        exit(1);
    }
}

void poll_touch() {
    struct input_event ev;
    int x = -1, y = -1;
    int tracking = 0;
    int has_position = 0;

    while (read(touch_fd, &ev, sizeof(struct input_event)) > 0) {
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_TRACKING_ID) {
                if (ev.value < 0) {
                    tracking = 0;
                    has_position = 0;
                } else {
                    tracking = 1;
                    x = y = -1;
                    has_position = 0;
                }
            } else if (ev.code == ABS_MT_POSITION_X) {
                x = ev.value;
                has_position = 1;
            } else if (ev.code == ABS_MT_POSITION_Y) {
                y = ev.value;
                has_position = 1;
            }
        }

        if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            if (tracking && has_position && x >= 0 && y >= 0) {
                handle_touch(x, y);

                // ✅ 每次觸控後刷新
                int rc = system("/usr/bin/fbink -s");
                (void)rc;

                x = y = -1;
                has_position = 0;
            }
        }
    }
}

void fb_close() {
    if (touch_fd > 0) {
        ioctl(touch_fd, EVIOCGRAB, 0);
        close(touch_fd);
    }
    munmap(fbp, vinfo.yres_virtual * finfo.line_length);
    close(fb_fd);
}
