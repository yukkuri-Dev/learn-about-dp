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

char filename[64];        // ← ファイル名を受け取るバッファ
unsigned long type;       // ← ファイルタイプを受け取る変数
char search_path[128];    // ← 検索パスを作成
int ret, handle;

static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

int main(void) {
    struct font *fnt = get_font();
    
    // 検索パスを構築
    sprintf(search_path, "%s*", drive[0]);
    
    // ファイルを検索
    ret = sys_findfirst(search_path, &handle, filename, &type);
    
    // ===== 描画開始 =====
    
    // 1. まず背景を黒で塗りつぶす
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // 2. 結果に応じてテキストを描画
    if (ret == 0) {
        // ファイルが見つかった場合
        set_pen(create_rgb16(255, 255, 255));  // 白色
        // strlen() を使って実際の文字列の長さを計算
        render_text(
            (SCREEN_WIDTH - strlen(filename) * fnt->width) / 2,
            SCREEN_HEIGHT / 2 - fnt->height * 2,
            filename
        );
        
        // 検索を終了（重要！）
        sys_findclose(handle);
    } else {
        // ファイルが見つからなかった場合
        set_pen(create_rgb16(255, 255, 255));  // 白色
        const char *msg = "Oh no! not found";
        render_text(
            (SCREEN_WIDTH - strlen(msg) * fnt->width) / 2,
            SCREEN_HEIGHT / 2 - fnt->height * 2,
            msg
        );
    }
    
    // 3. 終了メッセージを描画
    set_pen(create_rgb16(0, 255, 0));  // 緑色
    const char *exit_msg = "Press POWER or BACK key to exit.";
    render_text(
        (SCREEN_WIDTH - strlen(exit_msg) * fnt->width) / 2,
        SCREEN_HEIGHT / 2,
        exit_msg
    );
    
    // 4. VRAMにコピー（画面に反映）
    lcdc_copy_vram();
    
    // ===== 入力待機ループ =====
    while (1) {
        keypad_read();
        if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
            return -2;  // 終了
        }
    }
}