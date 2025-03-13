#include <assert.h>
#include <pico.h>
#include <stdbool.h>

#include <app/entry.h>
#include <stdio.h>
#include <util/time.h>
#include <util/log.h>

#include <display/display.h>

#include <pico/bootrom.h>

#include <pico/time.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>

int main(void)
{
    if (0 != display_init()) {
        for (;;);
    }

    stdio_usb_init();

    LOG("init", "starting up application...");

    bool restart = false;

    for (;;)
    {
        restart = app_main();

        if (!restart) {
            LOG("init", "exiting out of application...");

            break;
        }

        LOG("init", "application asked to restart, restarting...");
    }

    LOG("init", "deinitializing HALs...");

    display_deinit();

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
