#include "st7789v.h"
#include <errno.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/spi.h>
#include <hardware/structs/spi.h>
#include <pico.h>
#include <pico/lock_core.h>
#include <pico/mutex.h>
#include <pico/platform/compiler.h>
#include <pico/sem.h>
#include <pico/sync.h>
#include <pico/time.h>
#include <stdbool.h>
#include <util/log.h>
#include <util/types.h>
#include <hardware/dma.h>

#define DRV_LOG(...) LOG("st7789v", __VA_ARGS__)

//////////////////////////////////////////////////////////////// Serial communication variables
internal spi_inst_t *serial = ST7789V_SPI_PORT;

//////////////////////////////////////////////////////////////// DMA hardware variables
internal int dma_data_channel = -1;
internal semaphore_t *dma_operation_completion_signal = NULL;

//////////////////////////////////////////////////////////////// Driver-specific variables
internal bool is_plugged = false;

//////////////////////////////////////////////////////////////// Display state
internal bool row_and_column_exchanged = false;

/**
 * If there's an communication happening, this lock is triggered
 * when `st7789v_begin_comm()` is called, and released when `st7789v_end_comm()` is called.
 */
internal mutex_t communication_lock;

/**
 * If the driver is busy writing or reading data with `DMA` or normal read/write operations.
 */
internal mutex_t busy_lock;

/**
 * If the display is switching sleep states, this lock is released when the 5ms
 * delay of switching sleep states and be available to send new commands.
 *
 * This lock is locked by `st7789v_display_sleep_[in|out]()`
 */
internal mutex_t sleep_lock;

/**
 * If the display is switching sleep states, this lock is released when the 120ms
 * delay of changing sleep states finishes.
 *
 * This lock is locked by `st7789v_display_software_reset()` and `st7789v_display_sleep_[in|out]()`
 */
internal mutex_t sleep_switch_state_lock;

/**
 * If the display is resetting by software, this lock is released when the 5ms of the
 * reset has passed, after calling `st7789v_display_software_reset()`.
 */
internal mutex_t reset_lock;

internal void __isr st7789v_dma_irq_handler(void) {
    if (!dma_channel_get_irq0_status(dma_data_channel)) {
        // This IRQ was not caused by any of our channels
        DRV_LOG("irq0 received from unknown channel");

        return;
    }

    dma_channel_acknowledge_irq0(dma_data_channel);

    if (dma_operation_completion_signal != NULL) {
        sem_release(dma_operation_completion_signal);

        dma_operation_completion_signal = NULL;
    }

    // FIXME: if i do not read this Data Register of the SPI after the DMA operation,
    //        it's not going to write properly if the transaction is only one byte.
    uint32_t value = spi_get_hw(serial)->dr;

    mutex_exit(&busy_lock);
}

internal int64_t unlock_mutex_alarm_callback(alarm_id_t alarm_id, void *data) {
    mutex_t *mtx = data;

    mutex_exit(mtx);

    return 0;
}

internal void st7789v_dummy_cycle() {
    if (dma_channel_is_busy(dma_data_channel)) {
        return;
    }

    while (spi_is_busy(serial))
        tight_loop_contents();

    gpio_set_function(ST7789V_PIN_SCK, GPIO_FUNC_SIO);

    gpio_set_dir(ST7789V_PIN_SCK, GPIO_OUT);

    // Do a cycle
    gpio_put(ST7789V_PIN_SCK, 1);
    gpio_put(ST7789V_PIN_SCK, 0);

    gpio_set_function(ST7789V_PIN_SCK, GPIO_FUNC_SPI);
}

internal force_inline
bool st7789v_is_dma_busy() {
    return dma_channel_is_busy(dma_data_channel);
}

internal force_inline
bool st7789v_is_reset_busy() {
    return reset_lock.owner != LOCK_INVALID_OWNER_ID;
}

internal force_inline
bool st7789v_is_sleep_busy() {
    return sleep_lock.owner != LOCK_INVALID_OWNER_ID;
}

force_inline
error_t st7789v_begin_comm() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    mutex_enter_blocking(&communication_lock);

    gpio_put(ST7789V_PIN_CS, 0);

    return 0;
}

force_inline
error_t st7789v_end_comm() {
    if (!is_plugged) {
        return false;
    }

    gpio_put(ST7789V_PIN_CS, 1);

    mutex_exit(&communication_lock);

    return 0;
}

force_inline
void st7789v_begin_command() {
    gpio_put(ST7789V_PIN_DC, 0);
}

force_inline
void st7789v_end_command() {
    gpio_put(ST7789V_PIN_DC, 1);
}

error_t st7789v_write_sync(byte *buffer, size_t size) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    // If there is an ongoing DMA operation, we cannot do anything until the operation ends
    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    mutex_enter_blocking(&busy_lock);

    spi_set_baudrate(serial, ST7789V_WRITING_BAUDRATE);
    spi_write_blocking(serial, buffer, size);

    mutex_exit(&busy_lock);

    return 0;
}

error_t st7789v_read_sync(byte *buffer, size_t size) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }    

    mutex_enter_blocking(&busy_lock);

    spi_set_baudrate(serial, ST7789V_READING_BAUDRATE);
    spi_read_blocking(serial, 0xFF, buffer, size);

    mutex_exit(&busy_lock);

    return 0;
}

error_t st7789v_dma_write(
    byte *buffer,
    size_t size,
    enum dma_channel_transfer_size data_size,
    semaphore_t *completion_signal
) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    dma_channel_config config = dma_channel_get_default_config(dma_data_channel);

    channel_config_set_write_increment(&config, false);
    channel_config_set_read_increment(&config, true);

    channel_config_set_transfer_data_size(&config, data_size);

    channel_config_set_dreq(&config, spi_get_dreq(serial, /* is_tx: */ true));

    dma_operation_completion_signal = completion_signal;

    mutex_enter_blocking(&busy_lock);

    // Set the baud rate for writing data
    spi_set_baudrate(serial, ST7789V_WRITING_BAUDRATE);

    // Make the DMA request to write data
    dma_channel_configure(
        dma_data_channel,
        &config,
        /* write_addr: */ &spi_get_hw(serial)->dr,
        /* read_addr: */ buffer,
        /* transfer_count: */ size,
        /* trigger: */ true
    );

    return 0;
}

error_t st7789v_sync_dma_operation() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (!st7789v_is_dma_busy()) {
        return 0;
    }

    dma_channel_wait_for_finish_blocking(dma_data_channel);

    while (spi_is_busy(serial))
        tight_loop_contents();

    // Wait the IRQ to unlock this mutex, so we are 100% sure it's sent and the
    // IRQ handler executed and released the semaphore if provided.
    mutex_enter_blocking(&busy_lock);
    mutex_exit(&busy_lock);

    return 0;
}

error_t st7789v_init() {
    // Find a free DMA channel, if it's not found, error out
    dma_data_channel = dma_claim_unused_channel(false);

    if (dma_data_channel == -1) {
        DRV_LOG("couldn't find an available DMA channel for transmitting data");

        return -ENODMAAVAILABLE;
    }

    DRV_LOG("using DMA channel: %d", dma_data_channel);

    mutex_init(&busy_lock);
    mutex_init(&communication_lock);

    mutex_init(&sleep_switch_state_lock);
    mutex_init(&sleep_lock);
    mutex_init(&reset_lock);

    spi_init(serial, ST7789V_SPI_BAUDRATE);

    spi_set_format(serial, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Initialize this pin to be high
    gpio_put(ST7789V_PIN_CS, 1);

    gpio_set_function(ST7789V_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ST7789V_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ST7789V_PIN_SCK,  GPIO_FUNC_SPI);

    gpio_init(ST7789V_PIN_DC);
    gpio_init(ST7789V_PIN_CS);

    gpio_set_dir(ST7789V_PIN_DC, GPIO_OUT);
    gpio_set_dir(ST7789V_PIN_CS, GPIO_OUT);

    DRV_LOG("initialized SPI with frequency:");
    DRV_LOG("  reading: %d baud rate", ST7789V_READING_BAUDRATE);
    DRV_LOG("  writing: %d baud rate", ST7789V_WRITING_BAUDRATE);

    // Enable DMA interrupts
    dma_channel_set_irq0_enabled(dma_data_channel, true);

    irq_set_exclusive_handler(DMA_IRQ_0, st7789v_dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    is_plugged = true;

    st7789v_display_software_reset(true);

    uint32_t display_id = st7789v_display_read_id();

    is_plugged = false;
    
    if (display_id != ST7789V_DISPLAY_ID) {
        DRV_LOG("invalid display id received: %06x", display_id);

        return -ENODISPLAYCONNECTED;
    }

    is_plugged = true;

    DRV_LOG("found Sitronix ST7789V display controller on serial:");

    DRV_LOG(
        "  pins: tx=%d,rx=%d,cs=%d,sck=%d,dc=%d",
        ST7789V_PIN_MOSI,
        ST7789V_PIN_MISO,
        ST7789V_PIN_CS,
        ST7789V_PIN_SCK,
        ST7789V_PIN_DC
    );

    st7789v_memory_access_control_t madctl = { 0 };

    st7789v_display_read_memory_access_control(&madctl);

    row_and_column_exchanged = madctl.row_column_exchange;

    // TODO: init sequence here:
    //       - display_disable_sleep_mode
    //       - display_write_interface_pixel_format
    //       - display_write_memory_data_access_control
    //       - display_set_x_bounds
    //       - display_set_y_bounds
    //       - display_set_inversion_disabled
    //       - display_set_normal_mode
    //       - display_on

    return 0;
}

error_t st7789v_deinit(void) {
    if (dma_data_channel >= 0) {
        DRV_LOG("deinitializing DMA channel %d", dma_data_channel);

        dma_channel_set_irq0_enabled(dma_data_channel, false);

        irq_set_exclusive_handler(DMA_IRQ_0, NULL);
        irq_set_enabled(DMA_IRQ_0, false);

        dma_channel_cleanup(dma_data_channel);
        dma_channel_unclaim(dma_data_channel);
        
        dma_data_channel = -1;
    }

    DRV_LOG("deinitializing serial connection");
    spi_deinit(serial);

    DRV_LOG("deinitializing GPIO pins");
    gpio_deinit(ST7789V_PIN_CS);

    gpio_set_function(ST7789V_PIN_CS,   GPIO_FUNC_NULL);
    gpio_set_function(ST7789V_PIN_MISO, GPIO_FUNC_NULL);
    gpio_set_function(ST7789V_PIN_MOSI, GPIO_FUNC_NULL);
    gpio_set_function(ST7789V_PIN_SCK,  GPIO_FUNC_NULL);

    return 0;
}

error_t st7789v_send_command_sync(
    enum st7789v_command_t command,
    byte *parameters,
    size_t parameter_count
) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte command_byte = command & 0xFF;

    st7789v_begin_command();
        st7789v_write_sync(&command_byte, 1);
    st7789v_end_command();

    if (parameters != NULL && parameter_count > 0) {
        st7789v_write_sync(parameters, parameter_count);
    }

    return 0;
}

/******************** "HIGH" LEVEL API ********************/

error_t st7789v_display_no_operation() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_NO_OPERATION, NULL, 0);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_software_reset(bool sync_delay) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    mutex_enter_blocking(&reset_lock);
    mutex_enter_blocking(&sleep_switch_state_lock);

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_SOFTWARE_RESET, NULL, 0);

    st7789v_end_comm();

    add_alarm_in_ms(/* time_ms: */   5, unlock_mutex_alarm_callback, &reset_lock, true);
    add_alarm_in_ms(/* time_ms: */ 120, unlock_mutex_alarm_callback, &sleep_switch_state_lock, true);

    if (sync_delay) {
        sleep_ms(/* time_ms: */ 5);
    }

    return 0;
}

uint32_t st7789v_display_read_id() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    // This command is special, because it needs a dummy cycle,
    // so we're writing this directly instead of using `st7789v_send_command_sync`

    byte buffer[4] = {COMMAND_READ_DISPLAY_ID, 0x00, 0x00, 0x00};

    st7789v_begin_comm();

        st7789v_begin_command();

            st7789v_write_sync(buffer, 1);
        
        st7789v_end_command();

        st7789v_dummy_cycle();
        st7789v_read_sync(&buffer[1], 3);

    st7789v_end_comm();

    return buffer[1] << 16 | buffer[2] << 8 | buffer[3];
}

int32_t st7789v_display_read_status(st7789v_display_status_t *status) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    // This command is special, because it needs a dummy cycle,
    // so we're writing this directly instead of using `st7789v_send_command_sync`

    byte buffer[4] = { COMMAND_READ_DISPLAY_STATUS, 0x00, 0x00, 0x00 };

    st7789v_begin_comm();

        st7789v_begin_command();

            st7789v_write_sync(buffer, 1);
        
        st7789v_end_command();

        st7789v_dummy_cycle();
        st7789v_read_sync(buffer, 4);

    st7789v_end_comm();

    uint32_t raw_value = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];

    if (status != NULL) {
        status->raw_value = raw_value;
    }

    return raw_value;
}

byte st7789v_display_read_power_mode(st7789v_power_mode_t *pwrmode) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_status = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_POWER, NULL, 0);
        
        st7789v_read_sync(&raw_status, 1);

    st7789v_end_comm();

    if (pwrmode != NULL) {
        pwrmode->raw_value = raw_status;
    }

    return raw_status;
}


byte st7789v_display_read_memory_access_control(st7789v_memory_access_control_t *madctl) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_MEMORY_ACCESS_CONTROL, NULL, 0);

        st7789v_read_sync(&raw_value, 1);

    st7789v_end_comm();

    if (madctl != NULL) {
        madctl->raw_value = raw_value;
    }

    return raw_value;
}

byte st7789v_display_read_pixel_format(st7789v_interface_pixel_format *pixfmt) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_COLOR_PIXEL_FORMAT, NULL, 0);
        
        st7789v_read_sync(&raw_value, 1);

    st7789v_end_comm();

    if (pixfmt != NULL) {
        pixfmt->raw_value = raw_value;
    }

    return raw_value;
}

byte st7789v_display_read_image_mode(st7789v_image_mode_t *img_mode) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_IMAGE_MODE, NULL, 0);

        st7789v_read_sync(&raw_value, 1);

    st7789v_end_comm();

    if (img_mode) {
        img_mode->raw_value = raw_value;
    }

    return raw_value;
}

byte st7789v_display_read_signal_mode(st7789v_signal_mode_t *signal_mode) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_SIGNAL_MODE, NULL, 0);

        st7789v_read_sync(&raw_value, 1);

    st7789v_end_comm();

    if (signal_mode) {
        signal_mode->raw_value = raw_value;
    }

    return raw_value;
}

byte st7789v_display_read_self_diagnostic(st7789v_display_self_diagnostic_t *diag) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x00;

    st7789v_begin_comm();

        st7789v_send_command_sync(COMMAND_READ_DISPLAY_SELF_DIAGNOSTIC, NULL, 0);

        st7789v_read_sync(&raw_value, 1);

    st7789v_end_comm();

    if (diag) {
        diag->raw_value = raw_value;
    }

    return raw_value;
}

error_t st7789v_display_sleep_in(bool sync_delay) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    mutex_enter_blocking(&sleep_lock);
    mutex_enter_blocking(&sleep_switch_state_lock);

    st7789v_begin_comm();

    st7789v_send_command_sync(COMMAND_SLEEP_IN, NULL, 0);

    st7789v_end_comm();

    add_alarm_in_ms(/* time_ms: */   5, unlock_mutex_alarm_callback, &sleep_lock, true);
    add_alarm_in_ms(/* time_ms: */ 120, unlock_mutex_alarm_callback, &sleep_switch_state_lock, true);

    if (sync_delay) {
        sleep_ms(/* time_ms: */ 120);
    }

    return 0;
}

error_t st7789v_display_sleep_out(bool sync_delay) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    mutex_enter_blocking(&sleep_lock);
    mutex_enter_blocking(&sleep_switch_state_lock);

    st7789v_begin_comm();

    st7789v_send_command_sync(COMMAND_SLEEP_OUT, NULL, 0);

    st7789v_end_comm();

    add_alarm_in_ms(/* time_ms: */   5, unlock_mutex_alarm_callback, &sleep_lock, true);
    add_alarm_in_ms(/* time_ms: */ 120, unlock_mutex_alarm_callback, &sleep_switch_state_lock, true);

    if (sync_delay) {
        sleep_ms(/* time_ms: */ 120);
    }

    return 0;
}

error_t st7789v_display_set_normal_mode_state(bool enable) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    st7789v_command_t command = enable ? COMMAND_NORMAL_DISPLAY_MODE_ON : COMMAND_PARTIAL_DISPLAY_MODE_ON;

    st7789v_begin_comm();

    st7789v_send_command_sync(command, NULL, 0);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_enable_inversion(bool enable) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    st7789v_command_t command = enable ? COMMAND_DISPLAY_INVERSION_ON : COMMAND_DISPLAY_INVERSION_OFF;

    st7789v_begin_comm();

    st7789v_send_command_sync(command, NULL, 0);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_set_gamma_correction_curve(st7789v_gamma_curve_t gamma_curve) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    byte raw_value = 0x01;

    switch (gamma_curve)
    {
    case GAMMA_CURVE_1_DOT_0:
        raw_value = 0x08;
        break;

    case GAMMA_CURVE_2_DOT_5:
        raw_value = 0x04;
        break;

    case GAMMA_CURVE_1_DOT_8:
        raw_value = 0x02;
        break;

    /** The default value of `raw_value` is the bitmask for GAMMA_CURVE_2_DOT_2 */
    default:
        break;
    }

    st7789v_begin_comm();

    st7789v_send_command_sync(COMMAND_GAMMA_SET, NULL, 0);

    st7789v_write_sync(&raw_value, 0x01);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_turn_on() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    st7789v_begin_comm();

    st7789v_send_command_sync(COMMAND_DISPLAY_ON, NULL, 0);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_turn_off() {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    st7789v_begin_comm();

    st7789v_send_command_sync(COMMAND_DISPLAY_OFF, NULL, 0);

    st7789v_end_comm();

    return 0;
}

error_t st7789v_display_set_column_address_window(uint16_t start, uint16_t end) {
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    // The range for these parameters is:
    // 0 <= start < end <= 239  if st7789v_memory_access_control_t.row_column_exchange is true
    // 0 <= start < end <= 320  if st7789v_memory_access_control_t.row_column_exchange is false

    uint16_t maximum_boundary = row_and_column_exchanged ? ST7789V_DISPLAY_HEIGHT : ST7789V_DISPLAY_WIDTH;

    if (start < 0 || start > maximum_boundary || end < 0 || end > maximum_boundary) {
        return -ENOTINRANGE;
    }

    if (start >= end) {
        return -ENOTINRANGE;
    }

    byte paramaters[] = {
        // Start address, MSB
        (start >> 8) & 0xFF,
        start & 0xFF,

        // End address, MSB
        (end >> 8) & 0xFF,
        end & 0xFF
    };

    st7789v_begin_comm();

    st7789v_send_command_sync(
        /*         command: */ COMMAND_COLUMN_ADDRESS_SET,
        /*      parameters: */ parameters,
        /* parameter_count: */ sizeof(parameters) / sizeof(parameters[0])
    );

    st7789v_end_comm();

    return 0;
}

/**
 * NOTES
 * - The range for these parameters is:
 *   0 <= start < end <= 239  if st7789v_memory_access_control_t.row_column_exchange is false
 *   0 <= start < end <= 320  if st7789v_memory_access_control_t.row_column_exchange is true
 */
error_t st7789v_display_set_row_window(uint16_t start, uint16_t end)
{
    if (!is_plugged) {
        return -ENODISPLAYCONNECTED;
    }

    if (st7789v_is_dma_busy() || st7789v_is_reset_busy() || st7789v_is_sleep_busy()) {
        return -EDISPLAYBUSY;
    }

    // The range for these parameters is:
    // 0 <= start < end <= 239  if st7789v_memory_access_control_t.row_column_exchange is true
    // 0 <= start < end <= 320  if st7789v_memory_access_control_t.row_column_exchange is false

    uint16_t maximum_boundary = row_and_column_exchanged ? ST7789V_DISPLAY_WIDTH : ST7789V_DISPLAY_HEIGHT;

    if (start < 0 || start > maximum_boundary || end < 0 || end > maximum_boundary) {
        return -ENOTINRANGE;
    }

    if (start >= end) {
        return -ENOTINRANGE;
    }

    byte paramaters[] = {
        // Start address, MSB
        (start >> 8) & 0xFF,
        start & 0xFF,

        // End address, MSB
        (end >> 8) & 0xFF,
        end & 0xFF
    };

    st7789v_begin_comm();

    st7789v_send_command_sync(
        /*         command: */ COMMAND_ROW_ADDRESS_SET,
        /*      parameters: */ parameters,
        /* parameter_count: */ sizeof(parameters) / sizeof(parameters[0])
    );

    st7789v_end_comm();

    return 0;
}

// TODO: COMMAND_MEMORY_WRITE
// TODO: COMMAND_MEMORY_READ
// TODO: COMMAND_PARTIAL_AREA
// TODO: COMMAND_VERTICAL_SCROLLING_DEFINITION
// TODO: COMMAND_TEARING_EFFECT_LINE_OFF
// TODO: COMMAND_TEARING_EFFECT_LINE_ON
// TODO: COMMAND_MEMORY_ACCESS_CONTROL
// TODO: COMMAND_VERTICAL_SCROLL_START_ADDRESS
// TODO: COMMAND_IDLE_MODE_OFF
// TODO: COMMAND_IDLE_MODE_ON
// TODO: COMMAND_COLOR_PIXEL_FORMAT
// TODO: COMMAND_MEMORY_WRITE_CONTINUE
// TODO: COMMAND_MEMORY_READ_CONTINUE
// TODO: COMMAND_SET_TEAR_SCANLINE
// TODO: COMMAND_GET_SCANLINE
// TODO: COMMAND_WRITE_DISPLAY_BRIGHTNESS
// TODO: COMMAND_READ_DISPLAY_BRIGHTNESS
// TODO: COMMAND_WRITE_CTRL_DISPLAY
// TODO: COMMAND_READ_CTRL_DISPLAY
// TODO: COMMAND_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_COLOR_ENHANCEMENT
// TODO: COMMAND_READ_CONTENT_ADAPTIVE_BRIGHTNESS
// TODO: COMMAND_WRITE_CONTENT_ADAPTIVE_MINIMUM_BRIGHTNESS
// TODO: COMMAND_READ_CONTENT_ADAPTIVE_MINIMUM_BRIGHTNESS
// TODO: COMMAND_READ_AUTOMATIC_BRIGHTNESS_SELF_DIAGNOSTIC
// TODO: COMMAND_READ_ID_1
// TODO: COMMAND_READ_ID_2
// TODO: COMMAND_READ_ID_3
