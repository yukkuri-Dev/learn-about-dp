/* jpn-font.h
 * Declarations for the half-width katakana font support.
 */

#ifndef _JPN_FONT_H
#define _JPN_FONT_H

#include <stdint.h>
#include <graphics/text.h>

#define HW_KATA_FIRST 0xA1
#define HW_KATA_LAST  0xDF
#define HW_KATA_COUNT (HW_KATA_LAST - HW_KATA_FIRST + 1)

extern const uint8_t japanese8x8[];
extern struct font jpn_system;

/* Get pointer to the built-in Japanese font (half-width katakana table). */
struct font * get_jpn_font(void);

/* Set the built-in Japanese font as active. Returns previous font. */
struct font * set_jpn_font(void);

/* Render a NUL-terminated byte string where bytes 0xA1..0xDF map to
 * half-width katakana glyphs in the japanese8x8 array. ASCII (0x20..0x7E)
 * are rendered with the currently active ASCII font. */
void render_text_jpn_halfwidth(uint16_t x, uint16_t y, const uint8_t *text);

/* UTF-8 aware rendering: accepts UTF-8 encoded C string. Fullwidth
 * katakana (e.g. U+30A1 'ァ') and halfwidth katakana (U+FF61..U+FF9F)
 * are converted to the half-width table and rendered. */
void render_text_utf8_jpn(uint16_t x, uint16_t y, const char *utf8);

#endif /* _JPN_FONT_H */
