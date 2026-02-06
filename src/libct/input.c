#include "input.h"
#include <sh4a/input/keypad.h>
#include <libdataplus/sh4a/input/keypad.h>
#include <libdataplus/sh4a/input/exword_keys.h>

/*
 * Scan all possible key codes and return the first key currently held down.
 * Key codes format: (col+1)*10 + row (as used across the project).
 * We iterate col 0..8 (represents 1..9 when encoded) and row 1..8.
 */

int ct_get_current_keycode(void){
    for (int col = 0; col <= 8; ++col) {
        for (int row = 1; row <= 8; ++row) {
            uint8_t key_code = (uint8_t)((col + 1) * 10 + row);
            if (get_key_state(key_code)) {
                return (int)key_code;
            }
        }
    }
    return -1;
}
char keycode_to_text(int keycode){
    switch(keycode){
        case KEY_CHAR_A: return 'A';
        case KEY_CHAR_B: return 'B';
        case KEY_CHAR_C: return 'C';
        case KEY_CHAR_D: return 'D';
        case KEY_CHAR_E: return 'E';
        case KEY_CHAR_F: return 'F';
        case KEY_CHAR_G: return 'G';
        case KEY_CHAR_H: return 'H';
        case KEY_CHAR_I: return 'I';
        case KEY_CHAR_J: return 'J';
        case KEY_CHAR_K: return 'K';
        case KEY_CHAR_L: return 'L';
        case KEY_CHAR_M: return 'M';
        case KEY_CHAR_N: return 'N';
        case KEY_CHAR_O: return 'O';
        case KEY_CHAR_P: return 'P';
        case KEY_CHAR_Q: return 'Q';
        case KEY_CHAR_R: return 'R';
        case KEY_CHAR_S: return 'S';
        case KEY_CHAR_T: return 'T';
        case KEY_CHAR_U: return 'U';
        case KEY_CHAR_V: return 'V';
        case KEY_CHAR_W: return 'W';
        case KEY_CHAR_X: return 'X';
        case KEY_CHAR_Y: return 'Y';
        case KEY_CHAR_Z: return 'Z';
        default: return '[IDK]'; // Unknown
    }
}