#include <libdataplus/graphics/color.h>
#include <libdataplus/graphics/text.h>

/**
 * @brief Prints text at the specified coordinates with the given color
 * @param x The x-coordinate where the text will be printed
 * @param y The y-coordinate where the text will be printed
 * @param text Pointer to the null-terminated string to be printed
 * @param color The color value to use for rendering the text
 * @return void
 */
void ct_print(uint16_t x,uint16_t y,char *text,uint16_t color);
/**
 * @brief Clears the entire screen with the specified color
 * @param color The color value to fill the screen with
 * @return void
 */
void ct_screen_clear(uint16_t color);