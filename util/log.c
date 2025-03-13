#include <pico/time.h>
#include <pico/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <hardware/clocks.h>
#include <display/display.h>
#include <display/font.h>
#include <util/time.h>
#include <util/log.h>

internal int display_character_width() {
    v2 res = display_get_resolution();

    return (res.x / FONT_CHARACTER_WIDTH);
}

void __log_impl(
    const char *prefix,
    const char *function,
    const char *file,
    int line,
    const char *format,
    ...
) {
    va_list args;

    va_start(args, format);

    __logv_impl(prefix, function, file, line, format, args);

    va_end(args);
}

void __logv_impl(
    const char *prefix,
    const char *function,
    const char *file,
    int line,
    const char *format,
    va_list args
) {
    char buffer[display_character_width()];

    printf(
        "[%16.08f] ",
        to_us_since_boot(get_absolute_time()) / (ONE_SECOND_IN_MICROSECONDS * 1.)
    );

    printf("%s: ", prefix);
    vprintf(format, args);

    printf("\n");
}
