#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscalls/syscalls.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320
#define MAX_DISPLAY 15  // 画面に表示する最大ファイル数

char filename[64];
unsigned long type;
char search_path[128];
int ret, handle;
int selected_index = 0;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

int main(void) {
    struct font *fnt = get_font();
    int y_pos = 30;  // 描画開始Y座標
    int file_count = 0;
    char display_name[80];  // 表示用バッファ
    
    // 背景を黒で塗りつぶす
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // タイトル
    set_pen(create_rgb16(255, 255, 0));  // 黄色
    render_text(10, 10, "=== File List (drv0) ===");
    
    // 検索パスを構築（ルートディレクトリのすべてのファイル）
    sprintf(search_path, "%s*", drive[0]);
    
    // 最初のファイルを検索
    ret = sys_findfirst(search_path, &handle, filename, &type);
    
    if (ret == 0) {
        // 最初のファイルを表示
        set_pen(create_rgb16(255, 255, 255));
        
        // ファイルタイプを表示
        if (type == 0) {
            sprintf(display_name, "[DIR]  %s", filename);
        } else if (type == 1) {
            sprintf(display_name, "[FILE] %s", filename);
        } else {
            sprintf(display_name, "[?]    %s", filename);
        }
        
        render_text(10, y_pos, display_name);
        y_pos += fnt->height + 2;
        file_count++;
        
        // 残りのファイルを順次取得して表示
        while (file_count < MAX_DISPLAY) {
            ret = sys_findnext(handle, filename, &type);
            
            // もうファイルがない場合は終了
            if (ret != 0) break;
            
            // ファイルタイプを表示
            if (type == 0) {
                sprintf(display_name, "[DIR]  %s", filename);
            } else if (type == 1) {
                sprintf(display_name, "[FILE] %s", filename);
            } else {
                sprintf(display_name, "[?]    %s", filename);
            }
            
            render_text(10, y_pos, display_name);
            y_pos += fnt->height + 2;
            file_count++;
        }
        
        // 検索を終了（重要！）
        sys_findclose(handle);
        
        // ファイル数を表示
        set_pen(create_rgb16(0, 255, 255));  // シアン
        sprintf(display_name, "Total: %d files", file_count);
        render_text(10, y_pos + 10, display_name);
        
    } else {
        // ファイルが見つからなかった
        set_pen(create_rgb16(255, 0, 0));  // 赤色
        render_text(10, y_pos, "No files found!");
    }
    
    // 終了メッセージ
    set_pen(create_rgb16(0, 255, 0));  // 緑色
    const char *exit_msg = "Press POWER or BACK key to exit.";
    render_text(
        (SCREEN_WIDTH - strlen(exit_msg) * fnt->width) / 2,
        SCREEN_HEIGHT - fnt->height - 10,
        exit_msg
    );
    
    // VRAMにコピー（画面に反映）
    lcdc_copy_vram();
    
    // 入力待機ループ
    while (1) {
        keypad_read();
        if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
            return -2;
        }
        if (get_key_state(KEY_UP)){
          if (selected_index > 0) {
              selected_index--;
              lcdc_copy_vram();
              while (get_key_state(KEY_UP))
              {
                  keypad_read();
              }
          }
        }
        if (get_key_state(KEY_DOWN)){
          if (selected_index < file_count - 1) {
              selected_index++;
              lcdc_copy_vram();
              while (get_key_state(KEY_DOWN))
              {
                  keypad_read();
              }
              
          }
        }
        set_pen(create_rgb16(0, 0, 255));  // 青色
        render_text(0, selected_index * (fnt->height + 2) + 30, ">");  // 選択インジケータ
    }
}