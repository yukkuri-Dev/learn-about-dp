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
 * @param keycode 変換するキーコード。(exword_keys.hの定義を使用し、対応する文字がない場合は0を返す)
 * @return 対応する文字。対応する文字がない場合は0（'\0'）を返す。
 */
char keycode_to_text(int keycode);

#endif /* LIBCT_INPUT_H */
