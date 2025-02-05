#ifndef DRIVERS_ST7789V_H
#define DRIVERS_ST7789V_H

#include <hardware/spi.h>
#include <hardware/dma.h>
#include <pico/sem.h>
#include <stdbool.h>
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

typedef enum st7789v_command_t: uint8_t
{
    COMMAND_NO_OPERATION                                        = 0x00,
    COMMAND_SOFTWARE_RESET                                      = 0x01,
    COMMAND_READ_DISPLAY_ID                                     = 0x04,
    COMMAND_READ_DISPLAY_STATUS                                 = 0x09,
    COMMAND_READ_DISPLAY_POWER                                  = 0x0A,
    COMMAND_READ_DISPLAY_MEMORY_ACCESS_CONTROL                  = 0x0B,
    COMMAND_READ_DISPLAY_COLOR_PIXEL_FORMAT                     = 0x0C,
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

typedef enum st7789v_color_order_t: uint8_t
{
    COLOR_ORDER_RGB = 0x00,
    COLOR_ORDER_BGR = 0x01
} st7789v_color_order_t;

typedef enum st7789v_pixel_format_t: uint8_t
{
    COLOR_FORMAT_12BPP          = 0b011,
    COLOR_FORMAT_16BPP          = 0b101,
    COLOR_FORMAT_18BPP          = 0b110,
    COLOR_FORMAT_16M_TRUNCATED  = 0b111
} st7789v_pixel_format_t;

typedef enum st7789v_rgb_interface_format: uint8_t
{
    RGB_INTERFACE_FORMAT_65K    = 0b101,
    RGB_INTERFACE_FORMAT_262K   = 0b110
} st7789v_rgb_interface_format_t;

typedef enum st7789v_tearing_effect_mode_t: uint8_t
{
    TEARING_EFFECT_MODE_VBLANK_ONLY       = 0x00,
    TEARING_EFFECT_MODE_VBLANK_AND_HBLANK = 0x01,
} st7789v_tearing_effect_mode_t;

typedef enum st7789v_gamma_curve_t: uint8_t
{
    GAMMA_CURVE_2_DOT_2 = 0x01,
    GAMMA_CURVE_1_DOT_8 = 0x02,
    GAMMA_CURVE_2_DOT_5 = 0x04,
    GAMMA_CURVE_1_DOT_0 = 0x08
} st7789v_gamma_curve_t;

typedef union st7789v_display_status_t
{
    uint32_t raw_value;

    struct {
        uint8_t                                                     : 5;
        st7789v_tearing_effect_mode_t   tearing_effect_mode         : 1;
        st7789v_gamma_curve_t           gamma_curve                 : 3;
        bool                            tearing_effect_line         : 1;
        bool                            display_on                  : 1;
        uint8_t                                                     : 2;
        bool                            color_inversion             : 1;
        uint8_t                                                     : 2;
        bool                            display_normal_mode         : 1;
        bool                            sleep_out                   : 1;
        bool                            partial_mode                : 1;
        bool                            idle_mode                   : 1;
        st7789v_pixel_format_t          pixel_format                : 3;
        uint8_t                                                     : 2;
        bool                            horizontal_order_rtl        : 1;
        bool                            bgr_pixels                  : 1;
        bool                            scan_address_increment      : 1;
        bool                            row_column_exchange         : 1;
        bool                            column_address_decrement    : 1;
        bool                            row_address_decrement       : 1;
        bool                            voltage_booster_enabled     : 1;
    } __attribute__((packed));
} st7789v_display_status_t;

typedef union st7789v_memory_access_control_t {
    uint8_t raw_value;

    struct {
        uint8_t                                 : 2;
        bool        horizontal_order_rtl        : 1;
        bool        bgr_pixels                  : 1;
        bool        scan_address_increment      : 1;
        bool        row_column_exchange         : 1;
        bool        column_address_decrement    : 1;
        bool        row_address_decrement       : 1;
    } __attribute__((packed));
} st7789v_memory_access_control_t;

typedef union st7789v_power_mode_t {
    uint8_t raw_value;

    struct {
        uint8_t                             : 2;
        bool        display_on              : 1;
        bool        display_normal_mode     : 1;
        bool        sleep_out               : 1;
        bool        partial_mode            : 1;
        bool        idle_mode               : 1;
        bool        voltage_booster_enabled : 1;
    } __attribute__((packed));
} st7789v_power_mode_t;

typedef union st7789v_interface_pixel_format {
    uint8_t raw_value;

    struct {
        uint8_t                                         : 1;
        st7789v_rgb_interface_format_t  rgb_format      : 3;
        uint8_t                                         : 1;
        st7789v_pixel_format_t          pixel_format    : 3;
    } __attribute__((packed));
} st7789v_interface_pixel_format;


typedef union st7789v_image_mode_t {
    uint8_t raw_value;

    struct {
        st7789v_gamma_curve_t   gamma_curve         : 3;
        uint8_t                                     : 2;
        bool                    color_inversion     : 1;
        uint8_t                                     : 1;
        bool                    vertical_scrolling  : 1;
    } __attribute__((packed));
} st7789v_image_mode_t;

typedef union st7789v_signal_mode_t {
    uint8_t raw_value;

    struct {
        uint8_t                                             : 6;
        st7789v_tearing_effect_mode_t   tearing_effect_mode : 1;
        bool                            tearing_effect_line : 1;
    } __attribute__((packed));
} st7789v_signal_mode_t;

typedef union st7789v_display_self_diagnostic_t {
    uint8_t raw_value;

    struct {
        uint8_t                             : 6;
        bool    register_loading            : 1;
        bool    functionality_detection     : 1;
    } __attribute__((packed));
} st7789v_display_self_diagnostic_t;

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
external error_t st7789v_display_no_operation(void);

/**
 * Reset the display
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 */
external error_t st7789v_display_software_reset(void);

/**
 * Read the display id (it should match with ST7789V_DISPLAY_ID)
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The display id
 */
external uint32_t st7789v_display_read_id(void);

/**
 * Read the display status
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display status
 */
external uint32_t st7789v_display_read_status(st7789v_display_status_t *status);

/**
 * Read the display power mode
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display power status value
 */
external byte st7789v_display_read_power_mode(st7789v_power_mode_t *pwrmode);

/**
 * Read the display memory access control
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display memory access control value
 */
external byte st7789v_display_read_memory_access_control(st7789v_memory_access_control_t *madctl);

/**
 * Read the display memory access control
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display pixel format value
 */
external byte st7789v_display_read_pixel_format(st7789v_interface_pixel_format *pixfmt);

/**
 * Read the display image mode
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display image mode value
 */
external byte st7789v_display_read_image_mode(st7789v_image_mode_t *img_mode);

/**
 * Read the display signal mode
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display signal mode value
 */
external byte st7789v_display_read_signal_mode(st7789v_signal_mode_t *signal_mode);

/**
 * Read the display self diagnostic
 *
 * RETURN VALUE
 * - ENODEV: if the display is not plugged in, or unavailable
 * - EBUSY: if there is an operation ongoing in this display
 * - The raw display self diagnostic value
 */
external byte st7789v_display_read_self_diagnostic(st7789v_display_self_diagnostic_t *diag);

/**
 * Deinitializes this display
 *
 * RETURN VALUE
 * - This function always returns zero.
 */
external error_t st7789v_deinit(void);

#endif /** DRIVERS_ST7789V_H */
