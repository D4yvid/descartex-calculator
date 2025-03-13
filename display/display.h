#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#pragma once

#ifdef HAL_USE_ST7789V
#   include <drivers/st7789v.h>
#else
#   error   "No driver selected for display HAL!"
#endif

#include <util/types.h>
#include <util/util.h>
#include <util/math.h>

typedef enum display_error_t {
    ENOTCONNECTED           = 0x01
} display_error_t;

/**
 * Initializes the selected display driver
 *
 * RETURN VALUE
 * - ENOTCONNECTED: if the display is not connected, or the driver couldn't initialize it.
 * - zero.
 */
external error_t display_init(void);

/**
 * Return true if a display is plugged
 */
external bool display_plugged(void);

/**
 * Get the current display resolution
 *
 * RETURN VALUE
 * - The resolution of this display
 */
external v2 display_get_resolution(void);

/**
 * Fill the display with the specified color
 *
 * PARAMETERS
 * - color: the color to fill the display with
 *
 * RETURN VALUE
 * - ENOTCONNECTED: if the display is not connected, or the driver couldn't initialize it.
 */
external error_t display_fill_color(uint16_t color);

/**
 * Draw the specified buffer into the position `pos`
 *
 * PARAMETERS
 * - pos: the position where to put the buffer
 * - size: the resolution of the pixel data
 * - buffer: the pixel buffer to use
 *
 * RETURN VALUE
 * - ENOTCONNECTED: if the display is not connected, or the driver couldn't initialize it.
 */
external error_t display_draw_pixel_buffer(v2 pos, v2 size, uint8_t *buffer);

/**
 * De-initializes the display
 *
 * RETURN VALUE
 * - ENOTCONNECTED: if the display is not connected, or the driver couldn't initialize it.
 * - zero.
 */
external error_t display_deinit(void);

#endif /** HAL_DISPLAY_H */
