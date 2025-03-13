#ifndef CONSOLE_H
#define CONSOLE_H

#include <display/display.h>
#include <display/font.h>
#include <errno.h>

typedef enum console_error_t {
    ENODISPLAYCONNECTED     = 0x01
} console_error_t;

/**
 * Initialize the console interface
 *
 * RETURN VALUE
 * - ENODISPLAYFOUND: if no display is connected/found
 */
external error_t console_init(void);

/**
 * Disable the console on the display, it doesn't log anything back on the screen.
 */
external bool console_disable(void);

/**
 * Enable the console on the display
 */
external bool console_enable(void);

external bool console_set_color(uint32_t color);


#endif /** CONSOLE_H */

