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
#define MAX_FILES 100   // 読み込む最大ファイル数

// ファイル情報を保存する構造体
struct file_entry {
    char name[64];
    unsigned long type;
};

struct file_entry file_list[MAX_FILES];
int total_files = 0;
int scroll_offset = 0;

char filename[64];
unsigned long type;
char search_path[128];
char real_path[128];  // グローバル変数に変更
int ret, handle;
int selected_index = 0;
int prev_selected_index = 0;
int refresh_needed = 1;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

int main(void) {
    char path[128];
    struct font *fnt = get_font();
    int y_pos = 30;  // 描画開始Y座標
    char display_name[80];  // 表示用バッファ
    strcpy(real_path, drive[0]);  // 初期パスを内蔵ドライブに設定
    
reload_directory:  // ディレクトリ再読み込みのラベル
    // グローバル変数をリセット
    total_files = 0;
    scroll_offset = 0;
    selected_index = 0;
    prev_selected_index = 0;
    refresh_needed = 1;
    
    // 背景を黒で塗りつぶす
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // タイトル
    set_pen(create_rgb16(255, 255, 0));  // 黄色
    sprintf(path, "===File Manager - path:%s", real_path);
    render_text(10, 10, path);
    
    // 検索パスを構築（ルートディレクトリのすべてのファイル）
    sprintf(search_path, "%s*", real_path);
    
    // すべてのファイルを読み込む
    ret = sys_findfirst(search_path, &handle, filename, &type);
    
    if (ret == 0) {
        // 最初のファイルを保存
        strcpy(file_list[total_files].name, filename);
        file_list[total_files].type = type;
        total_files++;
        
        // 残りのファイルを順次取得して保存
        while (total_files < MAX_FILES) {
            ret = sys_findnext(handle, filename, &type);
            if (ret != 0) break;
            
            strcpy(file_list[total_files].name, filename);
            file_list[total_files].type = type;
            total_files++;
        }
        
        sys_findclose(handle);
        
        // 最初のページを描画
        // ".." エントリを先頭に追加（ルートでない場合）
        if (strcmp(real_path, drive[0]) != 0 && strcmp(real_path, drive[1]) != 0) {
          // 既存のファイルを1つ後ろにシフト
          for (int i = total_files; i > 0; i--) {
            strcpy(file_list[i].name, file_list[i-1].name);
            file_list[i].type = file_list[i-1].type;
          }
          // 先頭に ".." を追加
          strcpy(file_list[0].name, "..");
          file_list[0].type = 0;  // ディレクトリタイプ
          total_files++;
        }
        for (int i = 0; i < MAX_DISPLAY && i < total_files; i++) {
            set_pen(create_rgb16(255, 255, 255));
            
            if (file_list[i].type == 0) {
                // type == 0 はディレクトリ
                sprintf(display_name, "[DIR]  %s", file_list[i].name);
            } else if (file_list[i].type == 1) {
                // type == 1 はファイル
                sprintf(display_name, "[FILE] %s", file_list[i].name);
            } else {
                // それ以外は不明
                sprintf(display_name, "[%lu]    %s",file_list[i].type, file_list[i].name);
            }
            
            render_text(10, y_pos + i * (fnt->height + 2), display_name);
        }
        
        // ファイル数を表示
        set_pen(create_rgb16(0, 255, 255));  // シアン
        sprintf(display_name, "Total: %d files", total_files);
        render_text(10, y_pos + MAX_DISPLAY * (fnt->height + 2) + 10, display_name);
        
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
    
    // 初期カーソルを描画
    if (total_files > 0) {
        set_pen(create_rgb16(0, 0, 255));  // 青色
        render_text(0, 30, ">");  // 最初の項目にカーソル
    }
    
    // VRAMにコピー（画面に反映）
    lcdc_copy_vram();
    
    // 入力待機ループ
    while (1) {
        keypad_read();
        if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
            return -2;
        }
        if (get_key_state(KEY_ENTER)) {
            // 選択された項目を処理
            if (file_list[selected_index].type == 0) {
                // ディレクトリの場合、パスを更新して再読み込み
                if (strcmp(file_list[selected_index].name, "..") == 0) {
                    // 親ディレクトリに移動
                    char *last_sep = strrchr(real_path, '\\');
                    // ルートディレクトリではない場合のみ移動
                    if (last_sep != NULL && last_sep > real_path + 5) {
                        *last_sep = '\0'; // 最後のセパレータ以降を削除
                    }
                } else {
                    // サブディレクトリに移動
                    // パスの最後に\があるか確認
                    size_t len = strlen(real_path);
                    if (len > 0 && real_path[len - 1] != '\\') {
                        strcat(real_path, "\\");
                    }
                    strcat(real_path, file_list[selected_index].name);
                }
                
                // ディレクトリを再読み込み
                goto reload_directory;
            }
            // ファイルの場合は何もしない（将来的にファイル操作を追加可能）
            while (get_key_state(KEY_ENTER))
            {
                keypad_read();
            }
        }
        if (get_key_state(KEY_UP)){
          if (selected_index > 0) {
              prev_selected_index = selected_index;
              selected_index--;
              
              // スクロールが必要か確認
              if (selected_index < scroll_offset) {
                  scroll_offset = selected_index;
                  // 画面全体を再描画
                  set_pen(create_rgb16(0, 0, 0));
                  draw_rect(0, 30, SCREEN_WIDTH, MAX_DISPLAY * (fnt->height + 2));
                  
                  for (int i = 0; i < MAX_DISPLAY && (scroll_offset + i) < total_files; i++) {
                      set_pen(create_rgb16(255, 255, 255));
                      int idx = scroll_offset + i;
                      if (file_list[idx].type == 0) {
                          sprintf(display_name, "[DIR]  %s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          sprintf(display_name, "[FILE] %s", file_list[idx].name);
                      } else {
                          sprintf(display_name, "[%lu]    %s", file_list[idx].type, file_list[idx].name);
                      }
                      render_text(10, 30 + i * (fnt->height + 2), display_name);
                  }
              }
              
              refresh_needed = 1;
              while (get_key_state(KEY_UP))
              {
                  keypad_read();
              }
          }
        }
        if (get_key_state(KEY_DOWN)){
          if (selected_index < total_files - 1) {
              prev_selected_index = selected_index;
              selected_index++;
              
              // スクロールが必要か確認
              if (selected_index >= scroll_offset + MAX_DISPLAY) {
                  scroll_offset = selected_index - MAX_DISPLAY + 1;
                  // 画面全体を再描画
                  set_pen(create_rgb16(0, 0, 0));
                  draw_rect(0, 30, SCREEN_WIDTH, MAX_DISPLAY * (fnt->height + 2));
                  
                  for (int i = 0; i < MAX_DISPLAY && (scroll_offset + i) < total_files; i++) {
                      set_pen(create_rgb16(255, 255, 255));
                      int idx = scroll_offset + i;
                      if (file_list[idx].type == 0) {
                          sprintf(display_name, "[DIR]  %s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          sprintf(display_name, "[FILE] %s", file_list[idx].name);
                      } else {
                          sprintf(display_name, "[%lu]    %s", file_list[idx].type, file_list[idx].name);
                      }
                      render_text(10, 30 + i * (fnt->height + 2), display_name);
                  }
              }
              
              refresh_needed = 1;
              while (get_key_state(KEY_DOWN))
              {
                  keypad_read();
              }
              
          }
        }
        if (refresh_needed) {
            // 前のカーソルを消去（黒で上書き）
            int prev_screen_pos = prev_selected_index - scroll_offset;
            if (prev_screen_pos >= 0 && prev_screen_pos < MAX_DISPLAY) {
                set_pen(create_rgb16(0, 0, 0));
                render_text(0, 30 + prev_screen_pos * (fnt->height + 2), ">");
            }
            
            // 新しいカーソルを描画
            int screen_pos = selected_index - scroll_offset;
            if (screen_pos >= 0 && screen_pos < MAX_DISPLAY) {
                set_pen(create_rgb16(0, 0, 255));  // 青色
                render_text(0, 30 + screen_pos * (fnt->height + 2), ">");  // 選択インジケータ
            }
            
            // デバッグ情報を表示
            set_pen(create_rgb16(0, 0, 0));
            draw_rect(400, 10, 120, 20);  // 前の情報を消去
            set_pen(create_rgb16(255, 255, 255));
            sprintf(display_name, "Sel: %d/%d", selected_index + 1, total_files);
            render_text(400, 10, display_name);
            
            lcdc_copy_vram();
            refresh_needed = 0;
        }
    }
}