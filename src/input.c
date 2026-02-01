#include <sh4a/input/keypad.h>
#include "ui.h"
#include "input.h"

int handle_input(int total_files) {
    keypad_read();
    
    // 終了キー
    if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) {
        return -1;  // 終了
    }
    
    // 上キー
    if (get_key_state(KEY_UP)) {
        if (selected_index > 0) {
            prev_selected_index = selected_index;
            selected_index--;
            
            if (selected_index < scroll_offset) {
                scroll_offset = selected_index;
            }
            
            refresh_needed = 1;
        }
        
        while (get_key_state(KEY_UP)) {
            keypad_read();
        }
    }
    
    // 下キー
    if (get_key_state(KEY_DOWN)) {
        if (selected_index < total_files - 1) {
            prev_selected_index = selected_index;
            selected_index++;
            
            if (selected_index >= scroll_offset + MAX_DISPLAY) {
                scroll_offset = selected_index - MAX_DISPLAY + 1;
            }
            
            refresh_needed = 1;
        }
        
        while (get_key_state(KEY_DOWN)) {
            keypad_read();
        }
    }
    
    return 0;  // 継続
}
