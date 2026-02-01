#include "file_manager.h"
#include "ui.h"
#include "input.h"

int main(void) {
    // UI初期化
    ui_init();
    
    // ファイル一覧を読み込み
    if (load_files("\\\\drv0\\") != 0) {
        return -1;
    }
    
    // 初期画面を描画
    ui_draw_file_list(total_files);
    ui_update_cursor(total_files);
    ui_update_vram();
    refresh_needed = 0;  // 初期化
    
    // メインループ
    while (1) {
        if (handle_input(total_files) < 0) {
            break;  // 終了
        }
        
        if (refresh_needed) {
            ui_draw_file_list(total_files);
            ui_update_cursor(total_files);
            ui_update_vram();
            refresh_needed = 0;
        }
    }
    
    return -2;
}