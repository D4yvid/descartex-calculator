#ifndef DISPLAY_FONT_H
#define DISPLAY_FONT_H

#pragma once

#include <util/math.h>

#define FONT_CHARACTER_WIDTH 8

// FONT FLAGS:
// S: glyph width:
//   - 00 -> 8 bits
//   - 01 -> 6 bits
//   - 10 -> 4 bits
//   - 11 -> 2 bits
// F: glyph height
//   - 00 -> 8 bits
//   - 01 -> 6 bits
//   - 10 -> 4 bits
//   - 11 -> 2 bits
// D: downwards offset (the letter g is offset by N pixels down)
//   - 00 -> 0 pixels
//   - 01 -> 2 pixels
//   - 10 -> 4 pixels
//   - 11 -> 6 pixels
// U: upwards offset (the symbol ^ is offset by N pixels up)
//   - 00 -> 0 pixels
//   - 01 -> 2 pixels
//   - 10 -> 4 pixels
//   - 11 -> 6 pixels
// SSFFDDUU

/// USED LATER: #define FONT_GLYPH_WIDTH_8 0b00
/// USED LATER: #define FONT_GLYPH_WIDTH_6 0b01
/// USED LATER: #define FONT_GLYPH_WIDTH_4 0b10
/// USED LATER: #define FONT_GLYPH_WIDTH_2 0b11
/// USED LATER: 
/// USED LATER: #define FONT_GLYPH_HEIGHT_8 0b00
/// USED LATER: #define FONT_GLYPH_HEIGHT_6 0b01
/// USED LATER: #define FONT_GLYPH_HEIGHT_4 0b10
/// USED LATER: #define FONT_GLYPH_HEIGHT_2 0b11
/// USED LATER: 
/// USED LATER: #define FONT_GLYPH_DOWN_0_PIXELS 0b00
/// USED LATER: #define FONT_GLYPH_DOWN_2_PIXELS 0b01
/// USED LATER: #define FONT_GLYPH_DOWN_4_PIXELS 0b10
/// USED LATER: #define FONT_GLYPH_DOWN_6_PIXELS 0b11
/// USED LATER: 
/// USED LATER: #define FONT_GLYPH_UP_0_PIXELS 0b00
/// USED LATER: #define FONT_GLYPH_UP_2_PIXELS 0b01
/// USED LATER: #define FONT_GLYPH_UP_4_PIXELS 0b10
/// USED LATER: #define FONT_GLYPH_UP_6_PIXELS 0b11
/// USED LATER: 
/// USED LATER: #define FONT_GLYPH_WIDTH(n)     ((n & 0x3) << 6)
/// USED LATER: #define FONT_GLYPH_HEIGHT(n)    ((n & 0x3) << 4)
/// USED LATER: #define FONT_GLYPH_DOWNWARDS(n) ((n & 0x3) << 2)
/// USED LATER: #define FONT_GLYPH_UPWARDS(n)   ((n & 0x3))
/// USED LATER: 
/// USED LATER: #define FONT_GET_GLPYH_WIDTH(v)             (((v) >> 6) & 0x3)
/// USED LATER: #define FONT_GET_GLPYH_HEIGHT(v)            (((v) >> 4) & 0x3)
/// USED LATER: #define FONT_GET_GLPYH_DOWNWARDS_OFFSET(v)  (((v) >> 2) & 0x3)
/// USED LATER: #define FONT_GET_GLPYH_UPWARDS_OFFSET(v)    ((v) & 0x3)

/**
 * Basic monospace ASCII font for displaying text
 */
external byte DISPLAY_BASIC_FONT[0xFF][8];

/**
 * Draw character `c` at position `pos` on the display
 *
 * PARAMETERS
 * - c: the character to draw
 * - pos: the position where to draw the character
 * - color: the color to use for the pixel
 *
 * RETURN VALUE
 * - true if successful
 */
external bool display_font_put_char(char c, v2 position, uint32_t color);

#endif /** DISPLAY_FONT_H */
