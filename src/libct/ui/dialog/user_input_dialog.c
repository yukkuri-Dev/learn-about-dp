#include <libct/print.h>
#include <libct/input.h>
#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320


char user_input_dialog(void){
    ct_screen_clear(create_rgb16(0,0,0));
    struct font *fnt = get_font();
    char prompt[] = "";
    ct_print(10,10,"Please enter a string",create_rgb16(255,255,255));
    ct_print(0,30,">",create_rgb16(0,0,255));
    ct_print((SCREEN_WIDTH - strlen("Press POWER to Enter Press BACK to None.") * fnt->width) / 2,
        SCREEN_HEIGHT - fnt->height - 10,
        "Press POWER to Enter Press BACK to None.",
        create_rgb16(0, 255, 0));
    lcdc_copy_vram();
    while(1){              
        keypad_read();
        char now_input = keycode_to_text(ct_get_current_keycode());
        if (now_input == '[IDK]'){
            continue;
        }
        prompt[0] = now_input;
        ct_print(10 + fnt->width,30,prompt,create_rgb16(255,255,255));
        lcdc_copy_vram();
        while(get_key_state(now_input)){
            keypad_read();
        }
    }
}