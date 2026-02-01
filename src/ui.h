#ifndef UI_H
#define UI_H

#include <graphics/drawing.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320
#define MAX_DISPLAY 15  // 画面に表示する最大ファイル数

extern struct font *ui_font;
extern int scroll_offset;
extern int selected_index;
extern int prev_selected_index;
extern int refresh_needed;

// UI初期化
void ui_init(void);

// ファイルリストを描画
void ui_draw_file_list(int total_files);

// カーソルを更新
void ui_update_cursor(int total_files);

// 画面をクリア
void ui_clear_screen(void);

// VRAMを更新
void ui_update_vram(void);

#endif
