/**
 * @brief ユーザーからの文字列入力ダイアログを表示する。
 * @return 入力された文字列。キャンセルされた場合はNULLを返す。
 */
char *user_input_dialog(void);
/**
 * @brief ユーザーにYes/Noの選択を促すダイアログを表示する。
 * @param message 表示するメッセージ。
 * @param color メッセージの表示色。
 * @return Yesが選択された場合は0、Noが選択された場合は1
 * @note ユーザーがENTERキーを押すとYes、BACKキーを押すとNoとみなされる。
 */
int yes_or_no_dialog(const char *message,uint16_t color);