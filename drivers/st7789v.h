#ifndef DRIVERS_ST7789V_H
#define DRIVERS_ST7789V_H

#include <hardware/spi.h>
#include <hardware/dma.h>
#include <pico/sem.h>
#include <stddef.h>
#include <stdint.h>
#include <util/util.h>
#include <util/types.h>
#include <errno.h>

/******************** CONNECTION SETTINGS ********************/

#define ST7789V_SPI_PORT            spi0
#define ST7789V_SPI_BAUDRATE        (6625 * 1000)
#define ST7789V_READING_BAUDRATE    (6666 * 1000)
#define ST7789V_WRITING_BAUDRATE    (15151 * 1000)

#define ST7789V_PIN_MISO        16
#define ST7789V_PIN_CS          17
#define ST7789V_PIN_SCK         18
#define ST7789V_PIN_MOSI        19
#define ST7789V_PIN_DC          20

#define ST7789V_DISPLAY_ID      0x858552

/******************** DISPLAY_COMMANDS ********************/

typedef enum st7789v_command_t
{
    COMMAND_NO_OPERATION                                        = 0x00,
    COMMAND_SOFTWARE_RESET                                      = 0x01,
    COMMAND_READ_DISPLAY_ID                                     = 0x04,
    COMMAND_READ_DISPLAY_STATUS                                 = 0x09,
    COMMAND_READ_DISPLAY_POWER                                  = 0x0A,
    COMMAND_READ_MEMORY_ACCESS_CONTROL                          = 0x0B,
    COMMAND_READ_COLOR_PIXEL_FORMAT                             = 0x0C,
    COMMAND_READ_DISPLAY_IMAGE_MODE                             = 0x0D,
    COMMAND_READ_DISPLAY_SIGNAL_MODE                            = 0x0E,
    COMMAND_READ_DISPLAY_SELF_DIAGNOSTIC                        = 0x0F,
    COMMAND_SLEEP_IN                                            = 0x10,
    COMMAND_SLEEP_OUT                                           = 0x11,
    COMMAND_PARTIAL_DISPLAY_MODE_ON                             = 0x12,
    COMMAND_NORMAL_DISPLAY_MODE_ON                              = 0x13,
    COMMAND_DISPLAY_INVERSION_OFF                               = 0x20,
    COMMAND_DISPLAY_INVERSION_ON                                = 0x21,
    COMMAND_GAMMA_SET                                           = 0x26,
    COMMAND_DISPLAY_OFF                                         = 0x28,
    COMMAND_DISPLAY_ON                                          = 0x29,
    COMMAND_COLUMN_ADDRESS_SET                                  = 0x2A,
    COMMAND_ROW_ADDRESS_SET                                     = 0x2B,
    COMMAND_MEMORY_WRITE                                        = 0x2C,
    COMMAND_MEMORY_READ                                         = 0x2E,
    COMMAND_PARTIAL_AREA                                        = 0x30,
    COMMAND_VERTICAL_SCROLLING_DEFINITION                       = 0x33,
    COMMAND_TEARING_EFFECT_LINE_OFF                             = 0x34,
    COMMAND_TEARING_EFFECT_LINE_ON                              = 0x35,
    COMMAND_MEMORY_ACCESS_CONTROL                               = 0x36,
    COMMAND_VERTICAL_SCROLL_START_ADDRESS                       = 0x37,
    COMMAND_IDLE_MODE_OFF                                       = 0x38,
    COMMAND_IDLE_MODE_ON                                        = 0x39,
    COMMAND_COLOR_PIXEL_FORMAT                                  = 0x3A,
    COMMAND_MEMORY_WRITE_CONTINUE                               = 0x3C,
    COMMAND_MEMORY_READ_CONTINUE                                = 0x3E,
    COMMAND_SET_TEAR_SCANLINE                                   = 0x44,
    COMMAND_GET_SCANLINE                                        = 0x45,
    COMMAND_WRITE_DISPLAY_BRIGHTNESS                            = 0x51,
    COMMAND_READ_DISPLAY_BRIGHTNESS                             = 0x52,
    COMMAND_WRITE_CTRL_DISPLAY                                  = 0x53,
    COMMAND_READ_CTRL_DISPLAY                                   = 0x54,
    COMMAND_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_COLOR_ENHANCEMENT = 0x55,
    COMMAND_READ_CONTENT_ADAPTIVE_BRIGHTNESS                    = 0x56,
    COMMAND_WRITE_CONTENT_ADAPTIVE_MINIMUM_BRIGHTNESS           = 0x5E,
    COMMAND_READ_CONTENT_ADAPTIVE_MINIMUM_BRIGHTNESS            = 0x5F,
    COMMAND_READ_AUTOMATIC_BRIGHTNESS_SELF_DIAGNOSTIC           = 0x68,
    COMMAND_READ_ID_1                                           = 0xDA,
    COMMAND_READ_ID_2                                           = 0xDB,
    COMMAND_READ_ID_3                                           = 0xDC
} st7789v_command_t;

/**
 * Initialize the display on the pins defined above, using pico's Serial interface and
 * DMA hardware to get the best performance possible.
 *
 * RETURN VALUE
 * - EBUSY: if there's not an DMA channel available
 * - ENODEV: if the display doesn't match the specified id
 */
external error_t st7789v_init(void);

/**
 * Begin a communication with the display (set CS to LOW)
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 */
external error_t st7789v_begin_comm();

/**
 * End a communication with the display (set CS to LOW)
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 */
external error_t st7789v_end_comm();

/**
 * Begin the writing of an command, if you're sending a Command, PLEASE use this at the beginning
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 */
external void st7789v_begin_command();

/**
 * End the writing of an command, if you're not sending a command, for example, sending pixels or
 * parameters for the command, call this when the writing of the command ends
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 */
external void st7789v_end_command();

/**
 * Write synchronously into the display, this is useful to send commands,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
external error_t st7789v_write_sync(byte *buffer, size_t length);

/**
 * Read synchronously into the display, this is useful to receive command outputs,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`,
 * you can precisely control when to read data
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
error_t st7789v_read_sync(byte *buffer, size_t size);

/**
 * This function tries to write asynchronously into the display, this is useful to large buffers,
 * like pixels, etc.
 * 
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
external error_t st7789v_dma_write(
    byte *buffer,
    size_t size,
    enum dma_channel_transfer_size data_size,
    semaphore_t *completion_signal
);

/**
 * Waits for an DMA operation to finish on this display
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 */
external error_t st7789v_sync_dma_operation(void);

/**
 * Send an command synchronously to the display
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
external error_t st7789v_send_command_sync(
    enum st7789v_command_t command,
    byte *parameters,
    size_t parameter_count
);

/**
 * No operation (command NOOP)
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
error_t st7789v_display_no_operation(void);

/**
 * Reset the display
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
error_t st7789v_display_software_reset(void);

/**
 * Read the display id (it should match with ST7789V_DISPLAY_ID)
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The display id
 */
uint32_t st7789v_display_read_id(void);

/**
 * Deinitializes this display
 *
 * RETURN VALUE
 * - This function always returns zero.
 */
external error_t st7789v_deinit(void);

#endif /** DRIVERS_ST7789V_H */
