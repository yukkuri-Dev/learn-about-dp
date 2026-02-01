#include <stdio.h>
#include <string.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include "ui.h"
#include "file_manager.h"

struct font *ui_font = NULL;
int scroll_offset = 0;
int selected_index = 0;
int prev_selected_index = 0;
int refresh_needed = 1;
static char display_name[80];  // 再利用されるバッファ

void ui_init(void) {
    ui_font = get_font();
    if (ui_font == NULL) {
        // フォント取得失敗
        set_pen(create_rgb16(255, 0, 0));
        draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        lcdc_copy_vram();
        while(1);  // ハング
    }
    
    // タイトル
    set_pen(create_rgb16(255, 255, 0));  // 黄色
    render_text(10, 10, "=== File List (drv0) ===");
    
    // 終了メッセージ
    set_pen(create_rgb16(0, 255, 0));  // 緑色
    const char *exit_msg = "Press POWER or BACK key to exit.";
    render_text(
        (SCREEN_WIDTH - strlen(exit_msg) * ui_font->width) / 2,
        SCREEN_HEIGHT - ui_font->height - 10,
        exit_msg
    );
}

void ui_clear_screen(void) {
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 30, SCREEN_WIDTH, MAX_DISPLAY * (ui_font->height + 2));
}

void ui_draw_file_list(int total_files) {
    int y_pos = 30;
    
    ui_clear_screen();
    
    // ファイルリストを描画
    for (int i = 0; i < MAX_DISPLAY && (scroll_offset + i) < total_files; i++) {
        set_pen(create_rgb16(255, 255, 255));
        
        int idx = scroll_offset + i;
        if (file_list[idx].type == 0) {
            sprintf(display_name, "[DIR]  %s", file_list[idx].name);
        } else if (file_list[idx].type == 1) {
            sprintf(display_name, "[FILE] %s", file_list[idx].name);
        } else {
            sprintf(display_name, "[?]    %s", file_list[idx].name);
        }
        
        render_text(10, y_pos + i * (ui_font->height + 2), display_name);
    }
    
    // ファイル数を表示
    set_pen(create_rgb16(0, 255, 255));  // シアン
    sprintf(display_name, "Total: %d files", total_files);
    render_text(10, y_pos + MAX_DISPLAY * (ui_font->height + 2) + 10, display_name);
}

void ui_update_cursor(int total_files) {
    int y_pos = 30;
    int prev_screen_pos = prev_selected_index - scroll_offset;
    int screen_pos = selected_index - scroll_offset;
    
    // 前のカーソルを消去（黒で上書き）
    if (prev_screen_pos >= 0 && prev_screen_pos < MAX_DISPLAY) {
        set_pen(create_rgb16(0, 0, 0));
        render_text(0, y_pos + prev_screen_pos * (ui_font->height + 2), ">");
    }
    
    // 新しいカーソルを描画
    if (screen_pos >= 0 && screen_pos < MAX_DISPLAY) {
        set_pen(create_rgb16(0, 0, 255));  // 青色
        render_text(0, y_pos + screen_pos * (ui_font->height + 2), ">");
    }
    
    // デバッグ情報を表示
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(400, 10, 120, 20);
    set_pen(create_rgb16(255, 255, 255));
    sprintf(display_name, "Sel: %d/%d", selected_index + 1, total_files);
    render_text(400, 10, display_name);
}

void ui_update_vram(void) {
    lcdc_copy_vram();
}
