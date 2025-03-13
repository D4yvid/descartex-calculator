#include <drivers/st7789v.h>
#include <util/log.h>
#include <util/math.h>
#include <pico/sem.h>
#include <string.h>
#include <display/display.h>

#define DPY_LOG(...) LOG("display", __VA_ARGS__)

#define DISPLAY_PIXEL_COUNT (ST7789V_DISPLAY_WIDTH * ST7789V_DISPLAY_HEIGHT)
#define DISPLAY_BUFFER_SIZE (DISPLAY_PIXEL_COUNT * 3)

// If the display is initialized successfully
internal bool initialized = false;

internal semaphore_t busy_writing;

error_t display_init(void) {
    DPY_LOG("initializing display driver: st7789v");

    if (st7789v_init() != 0) {
        return -ENOTCONNECTED;
    }

    sem_init(&busy_writing, 0, 1);

    DPY_LOG("filling screen with black pixels");
    uint16_t value = 0x00;

    st7789v_display_memory_write_async_ex(
        /*                 buffer: */ (byte *) &value,
        /*                   size: */ DISPLAY_BUFFER_SIZE,
        /*      completion_signal: */ &busy_writing,
        /*       continue_writing: */ false,
        /* increment_buffer_index: */ false
    );

    sem_acquire_blocking(&busy_writing);

    DPY_LOG("st7789v: entering sleep out mode");
    st7789v_display_sleep_out(true);

    DPY_LOG("st7789v: enabling normal mode");
    st7789v_display_set_normal_mode_state(true);

    DPY_LOG("st7789v: turning on display");
    st7789v_display_turn_on();

    st7789v_display_set_column_address_window(0, 239);
    st7789v_display_set_row_address_window(0, 319);

    DPY_LOG("display initialized successfully!");

    initialized = true;

    return 0;
}

bool display_plugged(void) {
    return initialized;
}

force_inline
v2 display_get_resolution() {
    return v2(ST7789V_DISPLAY_WIDTH, ST7789V_DISPLAY_HEIGHT);
}

force_inline
error_t display_fill_color(uint16_t color) {
    st7789v_display_memory_write_async_ex(
        /*                 buffer: */ (byte *) &color,
        /*                   size: */ DISPLAY_BUFFER_SIZE,
        /*      completion_signal: */ &busy_writing,
        /*       continue_writing: */ false,
        /* increment_buffer_index: */ false
    );

    sem_acquire_blocking(&busy_writing);
}

error_t display_draw_pixel_buffer(v2 pos, v2 size, uint8_t *buffer) {
    if (size.x == 0 || size.y == 0)
        return 0;

    size_t byte_size = (size.x * size.y) * 3;

    st7789v_display_set_column_address_window(pos.x, (pos.x + size.x) - 1);
    st7789v_display_set_row_address_window(pos.y, (pos.y + size.y) - 1);

    st7789v_display_memory_write_async(
        /*                 buffer: */ (byte *) buffer,
        /*                   size: */ byte_size,
        /*      completion_signal: */ &busy_writing,
        /*       continue_writing: */ false
    );

    sem_acquire_blocking(&busy_writing);

    st7789v_display_set_column_address_window(0, ST7789V_DISPLAY_WIDTH - 1);
    st7789v_display_set_row_address_window(0, ST7789V_DISPLAY_HEIGHT - 1);
}

error_t display_deinit(void) {
    if (!initialized) {
        return -ENOTCONNECTED;
    }

    st7789v_deinit();

    return 0;
}
