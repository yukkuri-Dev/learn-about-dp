#ifndef LIBCT_INPUT_H
#define LIBCT_INPUT_H

#include <stdint.h>

/*
 * Return the first currently-pressed key code (using same codes as
 * `exword_keys.h`). If no key is pressed, return -1.
 */
int ct_get_current_keycode(void);
char keycode_to_text(int keycode);

#endif /* LIBCT_INPUT_H */
