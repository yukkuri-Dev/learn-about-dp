#ifndef LIBCT_INPUT_H
#define LIBCT_INPUT_H

#include <stdint.h>

/** 
 * @brief 現在押されているキーのキーコードを取得する。
 * @return 押されているキーのキーコード。押されていない場合は-1を返す。
 */
int ct_get_current_keycode(void);
/**
 * @brief キーコードを対応する文字に変換する。
 * @param keycode 変換するキーコード。(exword_keys.hの定義を使用し、文字ではないものは0を返す)
 * @return 入力された文字。キャンセルされた場合はNULLを返す。
 */
char keycode_to_text(int keycode);

#endif /* LIBCT_INPUT_H */
