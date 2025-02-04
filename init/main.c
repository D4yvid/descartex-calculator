#include <assert.h>
#include <pico.h>
#include <stdbool.h>

#include <app/entry.h>
#include <drivers/st7789v.h>
#include <stdio.h>
#include <util/time.h>
#include <util/log.h>

#include <pico/bootrom.h>

#include <pico/time.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>

int main(void)
{
    assert(stdio_usb_init());

    bool restart = false;

    char c = 0x00;

    while (c == 0x00) {
        // This will only return falsy when the Serial Terminal connects and sends an character
        // so all logs are shown
        c = getchar_timeout_us(1000 * ONE_SECOND_IN_MICROSECONDS);
    }

    // Clear terminal on the OS side
    printf("\033[H\033[J\033[2J");
    fflush(stdout);

    LOG("init", "starting up...");
    LOG("init", "loading drivers...");

    st7789v_init();

    LOG("init", "loading HALs...");
    LOG("init", "todo: really loading HALs...");

    LOG("init", "starting up application...");

    for (;;)
    {
        restart = app_main();

        if (!restart) {
            LOG("init", "exiting out of application...");

            break;
        }

        LOG("init", "application asked to restart, restarting...");
    }

    LOG("init", "deinitializing drivers");

    st7789v_deinit();

#ifndef DO_NOT_REBOOT_IN_BOOTSEL
    LOG("init", "rebooting into BOOTSEL mode");

    reset_usb_boot(0, 0);
#endif

    LOG("init", "halting CPU");

    for (;;) {
        sleep_ms(1000 * 1000);
        tight_loop_contents();
    }
}
