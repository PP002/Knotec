#include "fb.h"
#include "ui.h"
#include "font_ft.h"
#include <stdlib.h>
#include <unistd.h>

int main() {
    fb_init();
    fb_clear();

    init_layout();
    draw_input_box();
    draw_keyboard();

    // 初始化 FreeType 字体：像素高度 28（可调整）
    // 请替换为你实际的字体路径
    if (ui_init_freetype("/mnt/us/fonts/NotoSansCJK-Regular.otf", 28) != 0) {
        // 备选：系统字体路径
        ui_init_freetype("/usr/java/lib/fonts/Bookerly-Regular.ttf.ttf", 24);
    }

    int rc = system("/usr/bin/fbink -s");
    (void)rc;

    touch_init();
    poll_touch();

    save_input_to_file("/mnt/us/input.txt");
    fb_close();
    return 0;
}
