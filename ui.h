// ui.h
#ifndef UI_H
#define UI_H

void draw_input_box();
void draw_keyboard();
void handle_touch(int x, int y);
void save_input_to_file(const char *filename);

int ui_init_freetype(const char* font_path, int pixel_size);

#endif
