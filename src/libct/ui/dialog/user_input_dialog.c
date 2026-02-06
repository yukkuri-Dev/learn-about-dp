#include <libct/print.h>
#include <libct/input.h>
#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <string.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320


char *user_input_dialog(void){
    ct_screen_clear(create_rgb16(0,0,0));
    struct font *fnt = get_font();

    ct_print(10,10,"Please enter a string",create_rgb16(255,255,255));
    ct_print(0,30,">",create_rgb16(0,0,255));
    ct_print((SCREEN_WIDTH - strlen("Press POWER to Enter Press BACK to Cancel.") * fnt->width) / 2,
        SCREEN_HEIGHT - fnt->height - 10,
        "Press POWER to Enter Press BACK to Cancel.",
        create_rgb16(0, 255, 0));
    lcdc_copy_vram();

    static char buf[256];
    static int idx = 0;
    buf[0] = '\0';
    idx = 0;

    while (1) {
        keypad_read();
        int keycode = ct_get_current_keycode();
        if (keycode == -1) continue;

        /* Handle control keys first */
        if (get_key_state(KEY_ENTER)) {
            /* Return current buffer (may be empty) */
            while(get_key_state(KEY_ENTER)) { keypad_read(); }
            return buf;
        }
        if (keycode == KEY_BACK) {
            return NULL;
        }

        /* Handle backspace key */
        if (keycode == KEY_BACKSPACE) {
            if (idx > 0) buf[--idx] = '\0';
            /* Clear input area then draw */
            ct_screen_rect(10 + fnt->width, 30, SCREEN_WIDTH - (10 + fnt->width), fnt->height, create_rgb16(0,0,0));
            ct_print(10 + fnt->width, 30, buf, create_rgb16(255,255,255));
            lcdc_copy_vram();
            /* Wait until key is released */
            while (get_key_state(keycode)) { keypad_read(); }
            continue;
        }

        /* Translate keycode to text (if available) */
        char now_input = keycode_to_text(keycode);
        if (!now_input) {
            /* Unmapped key - ignore */
            while (get_key_state(keycode)) { keypad_read(); }
            continue;
        }

        /* Backspace handling */
        if (now_input == '\b' || now_input == 127) {
            if (idx > 0) buf[--idx] = '\0';
            /* Clear input area then draw */
            ct_screen_rect(10 + fnt->width, 30, SCREEN_WIDTH - (10 + fnt->width), fnt->height, create_rgb16(0,0,0));
            ct_print(10 + fnt->width, 30, buf, create_rgb16(255,255,255));
            lcdc_copy_vram();
            /* Wait until key is released */
            while (get_key_state(keycode)) { keypad_read(); }
            continue;
        } else {
            if (idx < (int)sizeof(buf) - 1) {
                buf[idx++] = now_input;
                buf[idx] = '\0';
            }
        }

        /* Draw input */
        ct_print(10 + fnt->width, 30, buf, create_rgb16(255,255,255));
        lcdc_copy_vram();

        /* Wait until key is released */
        while (get_key_state(keycode)) {
            keypad_read();
        }
    }
}
int yes_or_no_dialog(const char *message,uint16_t color){
    ct_screen_clear(create_rgb16(0,0,0));
    struct font *fnt = get_font();
    ct_print(10,SCREEN_HEIGHT / 2,message,color);
    ct_print((SCREEN_WIDTH - strlen("Press ENTER for Yes, BACK for No.") * fnt->width) / 2,
        SCREEN_HEIGHT - fnt->height - 10,
        "Press ENTER for Yes, BACK for No.",
        create_rgb16(0, 255, 0));
    lcdc_copy_vram();

    while (1) {
        keypad_read();
        if (get_key_state(KEY_ENTER)) {
            while(get_key_state(KEY_ENTER)) { keypad_read(); }
            return 0; // Yes
        }
        if (get_key_state(KEY_BACK)) {
            while(get_key_state(KEY_BACK)) { keypad_read(); }
            return 1; //No
        }
    }
}