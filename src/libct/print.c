#include <libdataplus/graphics/color.h>
#include <libdataplus/graphics/text.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320

void ct_print(uint16_t x,uint16_t y,char *text,uint16_t color){
    set_pen(color);
    render_text(x,y,text);
}
void ct_screen_clear(uint16_t color){
    set_pen(color);
    draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
}