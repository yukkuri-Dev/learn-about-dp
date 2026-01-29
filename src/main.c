#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320

int main(void) {
  struct font *fnt = get_font();

  set_pen(create_rgb16(0, 0, 0));
  draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  set_pen(create_rgb16(255, 255, 255));
  render_text((SCREEN_WIDTH - (sizeof "Hello, world!" - 1) * fnt->width) / 2, SCREEN_HEIGHT / 2 - fnt->height * 2, "Hello, world!");
  set_pen(create_rgb16(0, 255, 0));
  render_text((SCREEN_WIDTH - (sizeof "Press POWER or BACK key to exit." - 1) * fnt->width) / 2, SCREEN_HEIGHT / 2, "Press POWER or BACK key to exit.");
  lcdc_copy_vram();
  
  while (1) {
    keypad_read();
    if (get_key_state(KEY_POWER) || get_key_state(KEY_BACK)) return -2;
  }
}
