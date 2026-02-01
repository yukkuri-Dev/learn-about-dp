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
#include <memmgr.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320
#define MAX_DISPLAY 15  // 画面に表示する最大ファイル数

// グローバル変数（最小限）
int total_files = 0;
int scroll_offset = 0;
char real_path[48] = "";
int selected_index = 0;
int prev_selected_index = 0;
int refresh_needed = 1;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

// ディレクトリ内のファイル数をカウント
int count_files(const char *path) {
    char temp_filename[32];
    unsigned long temp_type;
    int temp_handle;
    int count = 0;
    char temp_search[48];
    int is_root = (strcmp(path, drive[0]) == 0 || strcmp(path, drive[1]) == 0);
    
    // ルートでない場合は".."を追加
    if (!is_root) {
        count = 1;
    }
    
    sprintf(temp_search, "%s*", path);
    if (sys_findfirst(temp_search, &temp_handle, temp_filename, &temp_type) == 0) {
        count++;
        while (sys_findnext(temp_handle, temp_filename, &temp_type) == 0) {
            count++;
        }
        sys_findclose(temp_handle);
    }
    return count;
}

// 指定されたインデックスのファイル情報を取得（0始まり）
// ルートでない場合、インデックス0は".."を返す
int get_file_at_index(const char *path, int index, char *out_name, unsigned long *out_type) {
    char temp_filename[32];
    unsigned long temp_type;
    int temp_handle;
    int current = 0;
    char temp_search[48];
    int is_root = (strcmp(path, drive[0]) == 0 || strcmp(path, drive[1]) == 0);
    
    // ルートでない場合、インデックス0は".."
    if (!is_root && index == 0) {
        strcpy(out_name, "..");
        *out_type = 0;  // ディレクトリ
        return 0;
    }
    
    // ルートでない場合はインデックスを1減らす
    int file_index = is_root ? index : index - 1;
    
    sprintf(temp_search, "%s*", path);
    if (sys_findfirst(temp_search, &temp_handle, temp_filename, &temp_type) != 0) {
        return -1;  // ファイルが見つからない
    }
    
    if (current == file_index) {
        strncpy(out_name, temp_filename, 31);
        out_name[31] = '\0';
        *out_type = temp_type;
        sys_findclose(temp_handle);
        return 0;
    }
    current++;
    
    while (sys_findnext(temp_handle, temp_filename, &temp_type) == 0) {
        if (current == file_index) {
            strncpy(out_name, temp_filename, 31);
            out_name[31] = '\0';
            *out_type = temp_type;
            sys_findclose(temp_handle);
            return 0;
        }
        current++;
    }
    
    sys_findclose(temp_handle);
    return -1;  // インデックスが範囲外
}

int main(void) {
    struct font *fnt;
    int y_pos = 30;
    char *path_buffer = NULL;      // 動的確保
    char *display_buffer = NULL;   // 動的確保
    char temp_name[32];            // 再利用される一時変数
    unsigned long temp_type;       // 再利用される一時変数
    char selected_name[32];        // 選択されたファイル名
    unsigned long selected_type;   // 選択されたファイル型
    int idx;                       // インデックス
    int screen_pos;                // スクリーン上の位置
    int prev_screen_pos;           // 前のスクリーン上の位置
    size_t len, name_len;          // 文字列長
    char *last_sep;                // 最後のセパレータ
    
    // メモリマネージャを初期化（malloc使用前に必須）
    memmgr_init();
    
    // まず画面を初期化（malloc前に初期化が必要）
    set_pen(create_rgb16(255, 255, 255));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    lcdc_copy_vram();
    
    // 動的にメモリ確保
    path_buffer = (char*)malloc(48);
    display_buffer = (char*)malloc(48);
    if (!path_buffer || !display_buffer) {
        // メモリ確保失敗
        set_pen(create_rgb16(255, 0, 0));
        draw_rect(0, 0, 50, 50);
        lcdc_copy_vram();
        if (path_buffer) free(path_buffer);
        if (display_buffer) free(display_buffer);
        while(1) { keypad_read(); if(get_key_state(KEY_POWER)) return -1; }
    }
    
    // フォントを取得
    fnt = get_font();
    if (fnt == NULL) {
        set_pen(create_rgb16(255, 0, 0));
        draw_rect(0, 0, 100, 100);
        lcdc_copy_vram();
        free(path_buffer);
        free(display_buffer);
        while(1) { keypad_read(); if(get_key_state(KEY_POWER)) return -1; }
    }
    
    // 初回のみ初期パスを設定
    if (real_path[0] == '\0') {
        strcpy(real_path, drive[0]);
    }
    
reload_directory:
    // 変数をリセット
    scroll_offset = 0;
    selected_index = 0;
    prev_selected_index = 0;
    refresh_needed = 1;
    
    // 背景を黒で塗りつぶす
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // タイトル
    set_pen(create_rgb16(255, 255, 0));
    sprintf(path_buffer, "===%.40s", real_path);
    render_text(10, 10, path_buffer);
    
    // ファイル数をカウント（キャッシュしない）
    total_files = count_files(real_path);
    
    // 最初のページを描画（必要な分だけファイル情報を取得）
    if (total_files > 0) {
        for (int i = 0; i < MAX_DISPLAY && i < total_files; i++) {
            if (get_file_at_index(real_path, i, temp_name, &temp_type) == 0) {
                set_pen(create_rgb16(255, 255, 255));
                
                if (temp_type == 0) {
                    sprintf(display_buffer, "[DIR]  %.36s", temp_name);
                } else if (temp_type == 1) {
                    sprintf(display_buffer, "[FILE] %.36s", temp_name);
                } else {
                    sprintf(display_buffer, "[%lu]    %.32s", temp_type, temp_name);
                }
                
                render_text(10, y_pos + i * (fnt->height + 2), display_buffer);
            }
        }
    }
    
    // ファイル数を表示
    set_pen(create_rgb16(0, 255, 255));
    sprintf(display_buffer, "%d files", total_files);
    render_text(10, y_pos + MAX_DISPLAY * (fnt->height + 2) + 10, display_buffer);
    
    // 終了メッセージ
    set_pen(create_rgb16(0, 255, 0));
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
            free(path_buffer);
            free(display_buffer);
            return -2;
        }
        if (get_key_state(KEY_ENTER)) {
            if (get_file_at_index(real_path, selected_index, selected_name, &selected_type) == 0) {
                // ディレクトリの場合、パスを更新して再読み込み
                if (selected_type == 0) {
                    // ".." の場合は親ディレクトリに移動
                    if (strcmp(selected_name, "..") == 0) {
                        last_sep = strrchr(real_path, '\\');
                        // ドライブルート(例: "\\drv0\")より深い位置にいる場合のみ親に移動
                        if (last_sep != NULL && (last_sep - real_path) > 6) {
                            *last_sep = '\0';
                        }
                    } else {
                        // サブディレクトリに移動
                        len = strlen(real_path);
                        name_len = strlen(selected_name);
                        
                        // バッファサイズチェック（null文字含む）
                        if (len + name_len + 2 <= 48) {
                            if (len > 0 && real_path[len - 1] != '\\') {
                                strcat(real_path, "\\");
                            }
                            strcat(real_path, selected_name);
                        }
                    }
                    
                    // ディレクトリを再読み込み
                    goto reload_directory;
                }
            }
            // キーを離すまで待機
            while (get_key_state(KEY_ENTER))
            {
                keypad_read();
            }
        }
        if (get_key_state(KEY_UP)){
            if (selected_index > 0) {
                prev_selected_index = selected_index;
                selected_index--;
                refresh_needed = 1;
                
                // スクロールが必要か確認
                if (selected_index < scroll_offset) {
                    scroll_offset = selected_index;
                    // スクロール後は笛書きを描画しない（refresh_neededで対応）
                }
            }
            // キーを離すまで待機
            while (get_key_state(KEY_UP))
            {
                keypad_read();
            }
        }
        if (get_key_state(KEY_DOWN)){
            if (selected_index < total_files - 1) {
                prev_selected_index = selected_index;
                selected_index++;
                refresh_needed = 1;
                
                // スクロールが必要か確認
                if (selected_index >= scroll_offset + MAX_DISPLAY) {
                    scroll_offset = selected_index - MAX_DISPLAY + 1;
                    // スクロール後は笛書きを描画しない（refresh_neededで対応）
                }
            }
            // キーを離すまで待機
            while (get_key_state(KEY_DOWN))
            {
                keypad_read();
            }
        }
        if (refresh_needed) {
            // 前のカーソルを消去（黒で上書き）
            prev_screen_pos = prev_selected_index - scroll_offset;
            if (prev_screen_pos >= 0 && prev_screen_pos < MAX_DISPLAY) {
                set_pen(create_rgb16(0, 0, 0));
                render_text(0, 30 + prev_screen_pos * (fnt->height + 2), ">");
            }
            
            // 新しいカーソルを描画
            screen_pos = selected_index - scroll_offset;
            if (screen_pos >= 0 && screen_pos < MAX_DISPLAY) {
                set_pen(create_rgb16(0, 0, 255));  // 青色
                render_text(0, 30 + screen_pos * (fnt->height + 2), ">");  // 選択インジケータ
            }
            
            // デバッグ情報を表示
            set_pen(create_rgb16(0, 0, 0));
            draw_rect(400, 10, 120, 20);  // 前の情報を消去
            set_pen(create_rgb16(255, 255, 255));
            sprintf(display_buffer, "Sel: %d/%d", selected_index + 1, total_files);
            render_text(400, 10, display_buffer);
            
            lcdc_copy_vram();
            refresh_needed = 0;
        }
    }
    
    // メモリ解放（実際にはここに到達しないが念のため）
    free(path_buffer);
    free(display_buffer);
    return 0;
}