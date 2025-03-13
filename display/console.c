#include <display/display.h>
#include <display/font.h>

/**
 * If the console is enabled to output pixels into the display
 */
internal bool is_console_enabled = false;

error_t console_init() {
    if (!display_plugged()) {
        return -ENODISPLAYCONNECTED;
    }
}

bool console_disable(void) {
    is_console_enabled = false;
}

bool console_enable(void) {
    is_console_enabled = true;
}
