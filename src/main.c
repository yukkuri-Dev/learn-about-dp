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
int ret, handle;
int selected_index = 0;
int prev_selected_index = 0;
int refresh_needed = 1;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

// 選択されたファイル名を取得
char* get_selected_filename(void) {
    if (selected_index >= 0 && selected_index < total_files) {
        return file_list[selected_index].name;
    }
    return NULL;
}

// 選択されたファイルタイプを取得
unsigned long get_selected_filetype(void) {
    if (selected_index >= 0 && selected_index < total_files) {
        return file_list[selected_index].type;
    }
    return 0;
}

int main(void) {
    memmgr_init();
    size_t path_len = strlen(drive[0]) + strlen("*") + 1;
    char *path = memmgr_alloc(path_len);
    if (path == NULL) {
      set_pen(create_rgb16(255, 0, 0));  // 赤色
      render_text(10, 10, "Memory allocation failed!");
      lcdc_copy_vram();
      while (1) {
        keypad_read();
        if (get_key_state(KEY_POWER)) {
            memmgr_free(path);
            return -2;
        }
      }
    }
    struct font *fnt = get_font();
    int y_pos = 30;  // 描画開始Y座標
    char display_name[80];  // 表示用バッファ
    
    // 背景を黒で塗りつぶす
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sprintf(path, "%s%s", drive[0], "*");  // "\\\\drv0\\*"
    // タイトル
    set_pen(create_rgb16(255, 255, 0));  // 黄色
    sprintf(display_name, "=== File List: %s ===", path);  
    render_text(10, 10, display_name);  //display_nameを表示

    // この行は削除（上で既にpathを作っている）
    // sprintf(search_path, "%s*", drive[0]);  ← 削除
    
    // すべてのファイルを読み込む
    ret = sys_findfirst(path, &handle, filename, &type);
    
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
        for (int i = 0; i < MAX_DISPLAY && i < total_files; i++) {
            set_pen(create_rgb16(255, 255, 255));
            
            if (file_list[i].type == 5) {
                sprintf(display_name, "[DIR]  %s", file_list[i].name);
            } else if (file_list[i].type == 1) {
                sprintf(display_name, "[FILE] %s", file_list[i].name);
            } else {
                sprintf(display_name, "[?]    %s", file_list[i].name);
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
    char *exit_msg = "Press POWER to exit. Press BACK to root.";
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
        if (get_key_state(KEY_POWER)) {
            memmgr_free(path);
            return -2;
        }
        if (get_key_state(KEY_BACK)){
          return 0;
        }
        if (get_key_state(KEY_ENTER)){
          // 選択されたファイル名を取得
          char *selected_file = get_selected_filename();
          unsigned long selected_type = get_selected_filetype();
          
          if (selected_file != NULL && selected_type == 5) {
              // ディレクトリの場合、そのディレクトリに移動
              char *new_path = memmgr_alloc(256);
              char *search_str = memmgr_alloc(256);
              if (new_path != NULL && search_str != NULL) {
                  // 現在のpathから"*"を除去してディレクトリパスを取得
                  strcpy(new_path, path);
                  char *star_pos = strchr(new_path, '*');
                  if (star_pos != NULL) {
                      *star_pos = '\0';
                  }
                  
                  // 選択したディレクトリを追加
                  strcat(new_path, selected_file);
                  strcat(new_path, "\\");
                  
                  // 検索用のパスを作成（"*"付き）
                  sprintf(search_str, "%s*", new_path);
                  
                  // 新しいディレクトリのファイル一覧を読み込む
                  total_files = 0;
                  selected_index = 0;
                  prev_selected_index = 0;
                  scroll_offset = 0;
                  
                  ret = sys_findfirst(search_str, &handle, filename, &type);
                  if (ret == 0) {
                      // 最初のファイルを保存
                      strncpy(file_list[total_files].name, filename, 63);
                      file_list[total_files].name[63] = '\0';
                      file_list[total_files].type = type;
                      total_files++;
                      
                      // 残りのファイルを読み込む
                      while (total_files < MAX_FILES) {
                          ret = sys_findnext(handle, filename, &type);
                          if (ret != 0) break;
                          strncpy(file_list[total_files].name, filename, 63);
                          file_list[total_files].name[63] = '\0';
                          file_list[total_files].type = type;
                          total_files++;
                      }
                      sys_findclose(handle);
                  }
                  
                  // pathを更新
                  strcpy(path, search_str);
                  memmgr_free(new_path);
                  memmgr_free(search_str);
                  
                  // 画面全体を再描画
                  set_pen(create_rgb16(0, 0, 0));
                  draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                  
                  // ヘッダーを表示
                  set_pen(create_rgb16(255, 255, 0));
                  sprintf(display_name, "=== File List: %s ===", path);
                  render_text(10, 10, display_name);
                  
                  if (total_files > 0) {
                      // ファイル一覧を表示
                      for (int i = 0; i < MAX_DISPLAY && i < total_files; i++) {
                          set_pen(create_rgb16(255, 255, 255));
                          if (file_list[i].type == 5) {
                              sprintf(display_name, "[DIR]  %s", file_list[i].name);
                          } else if (file_list[i].type == 1) {
                              sprintf(display_name, "[FILE] %s", file_list[i].name);
                          } else {
                              sprintf(display_name, "[?]    %s", file_list[i].name);
                          }
                          render_text(10, 30 + i * (fnt->height + 2), display_name);
                      }
                      
                      // ファイル数を表示
                      set_pen(create_rgb16(0, 255, 255));
                      sprintf(display_name, "Total: %d files", total_files);
                      render_text(10, 30 + MAX_DISPLAY * (fnt->height + 2) + 10, display_name);
                  } else {
                      // ファイルが見つからなかった
                      set_pen(create_rgb16(255, 0, 0));
                      render_text(10, 30, "No files found in this directory");
                  }
                  
                  lcdc_copy_vram();
                  refresh_needed = 1;
              }
          }
          
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
                      if (file_list[idx].type == 5) {
                          sprintf(display_name, "[DIR]  %s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          sprintf(display_name, "[FILE] %s", file_list[idx].name);
                      } else {
                          sprintf(display_name, "[?]    %s", file_list[idx].name);
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
                      if (file_list[idx].type == 5) {
                          sprintf(display_name, "[DIR]  %s", file_list[idx].name);
                      } else if (file_list[idx].type == 1) {
                          sprintf(display_name, "[FILE] %s", file_list[idx].name);
                      } else {
                          sprintf(display_name, "[?]    %s", file_list[idx].name);
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
