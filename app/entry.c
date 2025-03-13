#include <pico.h>
#include <stdbool.h>
#include <assert.h>
#include <display/display.h>
#include <display/font.h>
#include <util/log.h>
#include <util/time.h>
#include <pico/time.h>

bool app_main()
{
    char *phrases[] = {
        "O Italo e viado"
    };

    int x = 0;
    int y = 0;

    for (int i = 0 ; i < sizeof(phrases) / sizeof(phrases[0]) ; i ++) {
        const char *phrase = phrases[i];

        while (*phrase) {
            display_font_put_char(*phrase++, v2(x, y), 0xFFFFFF);
            x += 8;
        }

        y += 9;
        x = 0;
    }

    for (;;) {
    }
}
