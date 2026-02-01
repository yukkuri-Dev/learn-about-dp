#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <stddef.h>
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
char real_path[128] = "";  // 空文字列で初期化
char path_buffer[128];      // グローバルに移動
char display_buffer[80];    // グローバルに移動
int ret, handle;
int selected_index = 0;
int prev_selected_index = 0;
int refresh_needed = 1;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

int main(void) {
    struct font *fnt;
    int y_pos = 30;  // 描画開始Y座標
    
    // まず画面を初期化してクラッシュポイントを特定
    set_pen(create_rgb16(255, 255, 255));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    lcdc_copy_vram();
    
    // フォントを取得
    fnt = get_font();
    if (fnt == NULL) {
        // フォント取得失敗の場合は赤い画面を表示
        set_pen(create_rgb16(255, 0, 0));
        draw_rect(0, 0, 100, 100);
        lcdc_copy_vram();
        while(1) { keypad_read(); if(get_key_state(KEY_POWER)) return -1; }
    }
    
    // 初回のみ初期パスを設定
    if (real_path[0] == '\0') {
        strcpy(real_path, drive[0]);
    }
    
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
    snprintf(path_buffer, sizeof(path_buffer), "===File Manager - path:%s", real_path);
    render_text(10, 10, path_buffer);
    
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
          // バッファオーバーフローを防ぐ
          if (total_files < MAX_FILES) {
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
        }
        for (int i = 0; i < MAX_DISPLAY && i < total_files; i++) {
            set_pen(create_rgb16(255, 255, 255));
            
            if (file_list[i].type == 0) {
                // type == 0 はディレクトリ
                snprintf(display_buffer, sizeof(display_buffer), "[DIR]  %.50s", file_list[i].name);
            } else if (file_list[i].type == 1) {
                // type == 1 はファイル
                snprintf(display_buffer, sizeof(display_buffer), "[FILE] %.50s", file_list[i].name);
            } else {
                // それ以外は不明
                snprintf(display_buffer, sizeof(display_buffer), "[%lu]    %.50s",file_list[i].type, file_list[i].name);
            }
            
            render_text(10, y_pos + i * (fnt->height + 2), display_buffer);
        }
        
        // ファイル数を表示
        set_pen(create_rgb16(0, 255, 255));  // シアン
        snprintf(display_buffer, sizeof(display_buffer), "Total: %d files", total_files);
        render_text(10, y_pos + MAX_DISPLAY * (fnt->height + 2) + 10, display_buffer);
        
    } else {
        // ファイルが見つからなかった
        set_pen(create_rgb16(255, 0, 0));  // 赤色
        render_text(10, y_pos, "No files found!");
    }
    
    // 終了メッセージ
    set_pen(create_rgb16(0, 255, 0));  // 緑色
    char *exit_msg = "Press POWER or BACK key to exit.";
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
                    // ルートディレクトリではない場合のみ移動（例: \\drv0\ より長い場合）
                    if (last_sep != NULL && last_sep > real_path + 6) {
                        *last_sep = '\0'; // 最後のセパレータ以降を削除
                    }
                } else {
                    // サブディレクトリに移動
                    size_t len = strlen(real_path);
                    size_t name_len = strlen(file_list[selected_index].name);
                    
                    // バッファオーバーフローを防ぐ (real_pathは128バイト)
                    if (len + name_len + 2 < 128) {
                        // パスの最後に\があるか確認
                        if (len > 0 && real_path[len - 1] != '\\') {
                            strcat(real_path, "\\");
                        }
                        strcat(real_path, file_list[selected_index].name);
                    }
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
                          sprintf(display_buffer, "[DIR]  %s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          sprintf(display_buffer, "[FILE] %s", file_list[idx].name);
                      } else {
                          sprintf(display_buffer, "[%lu]    %s", file_list[idx].type, file_list[idx].name);
                      }
                      render_text(10, 30 + i * (fnt->height + 2), display_buffer);
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
                          snprintf(display_buffer, sizeof(display_buffer), "[DIR]  %.50s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          snprintf(display_buffer, sizeof(display_buffer), "[FILE] %.50s", file_list[idx].name);
                      } else {
                          snprintf(display_buffer, sizeof(display_buffer), "[%lu]    %.50s", file_list[idx].type, file_list[idx].name);
                      }
                      render_text(10, 30 + i * (fnt->height + 2), display_buffer);
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
            snprintf(display_buffer, sizeof(display_buffer), "Sel: %d/%d", selected_index + 1, total_files);
            render_text(400, 10, display_buffer);
            
            lcdc_copy_vram();
            refresh_needed = 0;
        }
    }
}