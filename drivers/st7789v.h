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

/******************** CONNECTION SETTINGS *************************/

// Work-around for:
// "warning: 'xxxxx' is narrower than values of its type"
#if __GNUC__ >=3
#   pragma GCC system_header
#endif

#define ST7789V_SPI_PORT            spi0
#define ST7789V_SPI_BAUDRATE        (62500000)
#define ST7789V_READING_BAUDRATE    (6666666)
#define ST7789V_WRITING_BAUDRATE    (62500000)

#define ST7789V_PIN_MISO        16
#define ST7789V_PIN_CS          17
#define ST7789V_PIN_SCK         18
#define ST7789V_PIN_MOSI        19
#define ST7789V_PIN_DC          20

/********************* DISPLAY CONSTANTS **************************/
#define ST7789V_DISPLAY_ID      0x858552
#define ST7789V_DISPLAY_WIDTH   240
#define ST7789V_DISPLAY_HEIGHT  320

typedef enum st7789v_error_t
{
    ENODISPLAYCONNECTED = 0x01,
    EDISPLAYBUSY        = 0x02,
    EINVALIDSTATE       = 0x03,
    ENODMAAVAILABLE     = 0x04,
    ENOTINRANGE         = 0x05,
    EUNAVAILABLE        = 0x06
} st7789v_error_t;

/**
 * All commands defined in the datasheet for the ST7789V Display Adapter,
 * these only include the first system command table, as the second can be
 * disabled for serial connections, i don't want to implement it here. I can
 * implement it in another .c file, as it contains really specific code, and i
 * don't want to mix it with this accessible command table.
 *
 * Please refer to the datasheet to see descriptions for each of these commands,
 * but the implementation for these is going to be documented.
 *
 * - Datasheet: https://newhavendisplay.com/pt/content/datasheets/ST7789V.pdf
 */
typedef enum st7789v_command_t: byte
{
    /** implemented in st7789v_display_no_operation */
    COMMAND_NO_OPERATION                                        = 0x00,
    /** implemented in st7789v_display_software_reset */
    COMMAND_SOFTWARE_RESET                                      = 0x01,
    /** implemented in st7789v_display_read_id */
    COMMAND_READ_DISPLAY_ID                                     = 0x04,
    /** implemented in st7789v_display_read_status */
    COMMAND_READ_DISPLAY_STATUS                                 = 0x09,
    /** implemented in st7789v_display_read_power_mode */
    COMMAND_READ_DISPLAY_POWER                                  = 0x0A,
    /** implemented in st7789v_display_read_memory_access_control */
    COMMAND_READ_DISPLAY_MEMORY_ACCESS_CONTROL                  = 0x0B,
    /** implemented in st7789v_display_read_pixel_format */
    COMMAND_READ_DISPLAY_COLOR_PIXEL_FORMAT                     = 0x0C,
    /** implemented in st7789v_display_read_image_mode */
    COMMAND_READ_DISPLAY_IMAGE_MODE                             = 0x0D,
    /** implemented in st7789v_display_read_signal_mode */
    COMMAND_READ_DISPLAY_SIGNAL_MODE                            = 0x0E,
    /** implemented in st7789v_display_read_self_diagnostic */
    COMMAND_READ_DISPLAY_SELF_DIAGNOSTIC                        = 0x0F,
    /** implemented in st7789v_display_sleep_in */
    COMMAND_SLEEP_IN                                            = 0x10,
    /** implemented in st7789v_display_sleep_out */
    COMMAND_SLEEP_OUT                                           = 0x11,
    /** implemented in st7789v_display_set_normal_mode_state(false) */
    COMMAND_PARTIAL_DISPLAY_MODE_ON                             = 0x12,
    /** implemented in st7789v_display_set_normal_mode_state(true) */
    COMMAND_NORMAL_DISPLAY_MODE_ON                              = 0x13,
    /** implemented in st7789v_display_enable_inversion(false) */
    COMMAND_DISPLAY_INVERSION_OFF                               = 0x20,
    /** implemented in st7789v_display_enable_inversion(true) */
    COMMAND_DISPLAY_INVERSION_ON                                = 0x21,
    /** implemented in st7789v_display_set_gamma_correction_curve */
    COMMAND_GAMMA_SET                                           = 0x26,
    /** implemented in st7789v_display_turn_off */
    COMMAND_DISPLAY_OFF                                         = 0x28,
    /** implemented in st7789v_display_turn_on */
    COMMAND_DISPLAY_ON                                          = 0x29,
    /** implemented in st7789v_display_set_column_address_window */
    COMMAND_COLUMN_ADDRESS_SET                                  = 0x2A,
    /** implemented in st7789v_display_set_row_address_window */
    COMMAND_ROW_ADDRESS_SET                                     = 0x2B,
    /** implemented in st7789v_display_memory_write_sync & st7789v_display_memory_write_async */
    COMMAND_MEMORY_WRITE                                        = 0x2C,
    /** implemented in st7789v_display_memory_read_sync */
    COMMAND_MEMORY_READ                                         = 0x2E,
    /** implemented in st7789v_display_set_partial_area */
    COMMAND_PARTIAL_AREA                                        = 0x30,
    /** implemented in st7789v_display_set_vertical_scrolling_parameters */
    COMMAND_VERTICAL_SCROLLING_DEFINITION                       = 0x33,
    /** implemented in st7789v_display_set_tearing_line_effect_enabled(false) */
    COMMAND_TEARING_EFFECT_LINE_OFF                             = 0x34,
    /** implemented in st7789v_display_set_tearing_line_effect_enabled(true) */
    COMMAND_TEARING_EFFECT_LINE_ON                              = 0x35,
    /** implemented in st7789v_display_set_memory_access_control */
    COMMAND_MEMORY_ACCESS_CONTROL                               = 0x36,
    /** implemented in st7789v_display_set_vertical_scrolling_start_address */
    COMMAND_VERTICAL_SCROLL_START_ADDRESS                       = 0x37,
    /** implemented in st7789v_display_set_idle(false) */
    COMMAND_IDLE_MODE_OFF                                       = 0x38,
    /** implemented in st7789v_display_set_idle(true) */
    COMMAND_IDLE_MODE_ON                                        = 0x39,
    /** implemented in st7789v_display_set_pixel_format */
    COMMAND_COLOR_PIXEL_FORMAT                                  = 0x3A,
    /** implemented in st7789v_display_memory_write_sync & st7789v_display_memory_write_async */
    COMMAND_MEMORY_WRITE_CONTINUE                               = 0x3C,
    /** implemented in st7789v_display_memory_read_sync */
    COMMAND_MEMORY_READ_CONTINUE                                = 0x3E,
    /** implemented in st7789v_display_set_tear_scanline */
    COMMAND_SET_TEAR_SCANLINE                                   = 0x44,
    /** implemented in st7789v_display_read_scanline */
    COMMAND_GET_SCANLINE                                        = 0x45,
    /** implemented in st7789v_display_set_display_brightness */
    COMMAND_WRITE_DISPLAY_BRIGHTNESS                            = 0x51,
    /** implemented in st7789v_display_read_display_brightness */
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
 * The color order for pixels sent
 */
typedef enum st7789v_color_order_t: byte
{
    /**
     * Use RGB order for sent pixels, for example:
     * - Bits: RRRRRGGGGGBGBBBBB (in 16 BPP pixel format)
     */
    COLOR_ORDER_RGB = 0x00,

    /**
     * Use BGR order for sent pixels, for example:
     * - Bits: BBBBBGGGGGGRRRRR (in 16 BPP pixel format)
     */
    COLOR_ORDER_BGR = 0x01
} st7789v_color_order_t;

/**
 * The pixel format (or depth) the display is using.
 *
 * This define how we need to send bits for the display, otherwise
 * the display can interpret the data wrong
 */
typedef enum st7789v_pixel_format_t: byte
{
    /**
     * Use 12 bits per pixel, 4 bits for R, G and B
     */
    COLOR_FORMAT_12BPP          = 0b011,

    /**
     * Use 16 bits per pixel, 5 bits for R, 6 for G and 5 for B
     */
    COLOR_FORMAT_16BPP          = 0b101,
    
    /**
     * Use 18 bits per pixel, 6 bits for R, G and B
     */
    COLOR_FORMAT_18BPP          = 0b110,

    /**
     * Use 18 bits per pixel, 6 bits for R, G and B, and truncate the rest bits
     */
    COLOR_FORMAT_16M_TRUNCATED  = 0b111
} st7789v_pixel_format_t;

/**
 * The configured bit depth of the display
 */
typedef enum st7789v_rgb_interface_format: byte
{
    /**
     * If the display is using 65K colors to display the image data
     */
    RGB_INTERFACE_FORMAT_65K    = 0b101,
    
    /**
     * If the display is using 262K colors to display the image data
     */
    RGB_INTERFACE_FORMAT_262K   = 0b110
} st7789v_rgb_interface_format_t;

/**
 * This command will control how the display will tear its contents
 */
typedef enum st7789v_tearing_effect_mode_t: byte
{
    /** If the display only should tear when the vertical blank is at maximum */
    TEARING_EFFECT_MODE_VBLANK_ONLY       = 0x00,

    /** If the display can tear even when the horizontal blank is at maximum */
    TEARING_EFFECT_MODE_VBLANK_AND_HBLANK = 0x01,
} st7789v_tearing_effect_mode_t;

/**
 * The gamma curve used for gamma correction in the display, to understand
 * how this works, take a look here: https://en.wikipedia.org/wiki/Gamma_correction
 */
typedef enum st7789v_gamma_curve_t: byte
{
    /** 2.2 curve */
    GAMMA_CURVE_2_DOT_2 = 0x01,

    /** 1.8 curve */
    GAMMA_CURVE_1_DOT_8 = 0x02,

    /** 1.5 curve */
    GAMMA_CURVE_2_DOT_5 = 0x04,

    /** 1.0 curve */
    GAMMA_CURVE_1_DOT_0 = 0x08
} st7789v_gamma_curve_t;

/**
 * The content adaptive image type the display is using
 */
typedef enum st7789v_content_adaptive_brightness_t: byte {
    CONTENT_ADAPTIVE_OFF                    = 0b00,
    CONTENT_ADAPTIVE_USER_INTERFACE         = 0b01,
    CONTENT_ADAPTIVE_STILL_PICTURE          = 0b10,
    CONTENT_ADAPTIVE_MOVING_IMAGE           = 0b11
} st7789v_content_adaptive_brightness_t;

/**
 * The color enhancement mode the display is using, if available
 */
typedef enum st7789v_color_enhancement_type_t: byte {
    COLOR_ENHANCEMENT_LOW           = 0b00,
    COLOR_ENHANCEMENT_MEDIUM        = 0b01,
    COLOR_ENHANCEMENT_HIGH          = 0b11
} st7789v_color_enhancement_type_t;

/**
 * The almost full display status (is missing some data you can query
 * separately)
 *
 * Read this structure from the display by using `st7789v_display_read_status`.
 */
typedef union st7789v_display_status_t
{
    uint32_t raw_value;

    struct {
        byte                                                     : 5;

        /** The tearing effect mode the display is in */
        st7789v_tearing_effect_mode_t   tearing_effect_mode         : 1;

        /** The gamma curve used for gamma correction applied */
        st7789v_gamma_curve_t           gamma_curve                 : 3;
        
        /**
         * If the tearing effect line is enabled (if this is false,
         * the `tearing_effect_mode` doesn't change anything)
         */
        bool                            tearing_effect_line         : 1;

        /**
         * If the display is on, and showing the pixel data.
         */
        bool                            display_on                  : 1;
        byte                                                     : 2;

        /**
         * If color inversion is applied in the display's pixels
         */
        bool                            color_inversion             : 1;
        byte                                                     : 2;

        /**
         * If the display is on normal operation, if this is false,
         * the display is going to be on the `partial` display.
         *
         * Refer to the datasheet for more information about this display mode.
         */
        bool                            display_normal_mode         : 1;
        
        /**
         * If the display is on the `sleep_out` mode (not sleeping)
         */
        bool                            sleep_out                   : 1;

        /**
         * If the display is in partial mode.
         *
         * Refer to the datasheet for more information about this mode.
         */
        bool                            partial_mode                : 1;

        /**
         * If the display is in idle mode. (8-bit color depth)
         *
         * Refer to the datasheet for more information about this mode.
         */
        bool                            idle_mode                   : 1;

        /**
         * The current pixel format the display is using.
         */
        st7789v_pixel_format_t          pixel_format                : 3;

        byte                                                     : 2;

        /**
         * If the display memory is being used from right to left
         */
        bool                            horizontal_order_rtl        : 1;

        /**
         * If the display memory is being used with BGR order instead of RGB order
         */
        bool                            bgr_pixels                  : 1;
        
        /**
         * If the display memory is being read with the scan address incrementing
         */
        bool                            scan_address_increment      : 1;
        
        /**
         * If the display memory is being read with the column and row addresses
         * exchanged
         */
        bool                            row_column_exchange         : 1;

        /**
         * If the display memory is being read with the column address decrementing
         */
        bool                            column_address_decrement    : 1;
        
        /**
         * If the display memory is being read with the row address decrementing
         */
        bool                            row_address_decrement       : 1;
        
        /**
         * If the voltage booster is enabled
         */
        bool                            voltage_booster_enabled     : 1;
    } __attribute__((packed));
} st7789v_display_status_t;

/**
 * The configuration that the display is using to access it's internal pixel memory 
 *
 * Read this structure from the display by using `st7789v_display_read_memory_access_control`.
 */
typedef union st7789v_memory_access_control_t {
    byte raw_value;

    struct {
        byte                                 : 2;
        
        /**
         * If the display memory is being used from right to left (MH)
         */
        bool        horizontal_order_rtl        : 1;

        /**
         * If the display memory is being used with BGR order instead of RGB order (RGB)
         */
        bool        bgr_pixels                  : 1;

        /**
         * If the display memory is being read with the scan address incrementing (ML)
         */
        bool        scan_address_increment      : 1;
                
        /**
         * If the display memory is being read with the column and row addresses
         * exchanged (MV)
         */
        bool        row_column_exchange         : 1;

        /**
         * If the display memory is being read with the column address decrementing (MX)
         */
        bool        column_address_decrement    : 1;
                
        /**
         * If the display memory is being read with the row address decrementing (MY)
         */
        bool        row_address_decrement       : 1;
    } __attribute__((packed));
} st7789v_memory_access_control_t;

/**
 * The current display power mode status
 *
 * Read this structure from the display by using `st7789v_display_read_power_mode`.
 */
typedef union st7789v_power_mode_t {
    byte raw_value;

    struct {
        byte                             : 2;
        
        /**
         * If the display is on, and showing the pixel data.
         */
        bool        display_on              : 1;

        /**
         * If the display is on normal operation, if this is false,
         * the display is going to be on the `partial` display.
         *
         * Refer to the datasheet for more information about this display mode.
         */
        bool        display_normal_mode     : 1;

        /**
         * If the display is on the `sleep_out` mode (not sleeping)
         */
        bool        sleep_out               : 1;

        /**
         * If the display is in partial mode.
         *
         * Refer to the datasheet for more information about this mode.
         */
        bool        partial_mode            : 1;

        /**
         * If the display is in idle mode. (8-bit color depth)
         *
         * Refer to the datasheet for more information about this mode.
         */
        bool        idle_mode               : 1;
      
        /**
         * If the voltage booster is enabled
         */
        bool        voltage_booster_enabled : 1;
    } __attribute__((packed));
} st7789v_power_mode_t;

/**
 * The pixel format and rgb format which the display uses to receive pixels.
 * 
 * Read this structure from the display by using `st7789v_display_read_pixel_format`.
 */
typedef union st7789v_interface_pixel_format_t {
    byte raw_value;

    struct {
        byte                                         : 1;

        /**
         * How many colors the display is using to show the image
         */
        st7789v_rgb_interface_format_t  rgb_format      : 3;

        byte                                         : 1;

        /**
         * The pixel format being used by the display (or depth)
         */
        st7789v_pixel_format_t          pixel_format    : 3;
    } __attribute__((packed));
} st7789v_interface_pixel_format_t;

/**
 * The image mode which the display is using
 *
 * Read this structure from the display by using `st7789v_display_read_image_mode`
 */
typedef union st7789v_image_mode_t {
    byte raw_value;

    struct {
        /**
         * The gamma curve the display is using
         */
        st7789v_gamma_curve_t   gamma_curve         : 3;
        byte                                     : 2;

        /**
         * If the display has color inversion enabled
         */
        bool                    color_inversion     : 1;
        byte                                     : 1;
        
        /**
         * If vertical scrolling is being used by the display
         */
        bool                    vertical_scrolling  : 1;
    } __attribute__((packed));
} st7789v_image_mode_t;

/**
 * The current signal mode used by the display
 *
 * Read this structure from the display by using `st7789v_display_read_signal_mode`
 */
typedef union st7789v_signal_mode_t {
    byte raw_value;

    struct {
        byte                                             : 6;

        /**
         * If the tearing effect line is enabled (if this is false,
         * the `tearing_effect_mode` doesn't change anything)
         */
        st7789v_tearing_effect_mode_t   tearing_effect_mode : 1;

        /**
         * If the tearing effect line is enabled (if this is false,
         * the `tearing_effect_mode` doesn't change anything)
         */
        bool                            tearing_effect_line : 1;
    } __attribute__((packed));
} st7789v_signal_mode_t;

/**
 * Display self diagnostic information
 *
 * Read this structure from the display by using `st7789v_display_read_self_diagnostic` & `st7789v_display_read_adaptive_brightness_control_self_diagnostic`
 */
typedef union st7789v_self_diagnostic_t {
    byte raw_value;

    struct {
        byte                             : 6;
        bool    register_loading            : 1;
        bool    functionality_detection     : 1;
    } __attribute__((packed));
} st7789v_self_diagnostic_t;

/**
 * Current display control register
 *
 * Read this structure from the display by using `st7789v_self_diagnostic_t`
 */
typedef union st7789v_display_ctrl_t {
    byte raw_value;

    struct {
        byte                             : 2;
        /**
         * If backlight control is enabled on this display
         */
        bool        backlight_control       : 1;

        /**
         * If display dimming is enabled on this display
         */
        bool        display_dimming         : 1;
        byte                             : 1;

        /**
         * If brightness control is enabled on this display
         */
        bool        brightness_control      : 1;
        byte                             : 2;
    } __attribute__((packed));
} st7789v_display_ctrl_t;

/**
 * The display content adaptive brightness and color enhancement settings
 *
 * Used for setting values to the display only.
 */
typedef union st7789v_adaptive_brightness_color_enhancement_t {
    byte raw_value;

    struct {
        /**
         * If color enhancement is enabled on the display
         */
        bool                                    color_enhancement       : 1;
        byte                                                         : 1;

        /**
         * The color enhancement type the display is using
         */
        st7789v_color_enhancement_type_t        color_enhancement_type  : 2;
        byte                                                         : 2;

        /**
         * The adaptive content type this display is using
         */
        st7789v_content_adaptive_brightness_t   content_type            : 2;
    } __attribute__((packed));
} st7789v_adaptive_brightness_color_enhancement_t;

/**
 * Initialize the display on the pins defined above, using pico's Serial interface and
 * DMA hardware to get the best performance possible.
 *
 * RETURN VALUE
 * - ENODMAAVAILABLE: if there's not an DMA channel available
 * - ENODISPLAYCONNECTED: if the display doesn't match the specified id
 */
external error_t st7789v_init(void);

/**
 * Begin a communication with the display (set CS to LOW)
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external error_t st7789v_begin_comm(void);

/**
 * End a communication with the display (set CS to HIGH)
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external error_t st7789v_end_comm(void);

/**
 * Begin the writing of an command, if you're sending a Command, PLEASE use this at the beginning
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external void st7789v_begin_command(void);

/**
 * End the writing of an command, if you're not sending a command, for example, sending pixels or
 * parameters for the command, call this when the writing of the command ends
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external void st7789v_end_command(void);

/**
 * Write synchronously into the display, this is useful to send commands,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`
 *
 * PARAMETERS
 * - buffer: the buffer to write
 * - size: the size of the buffer to write
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_write_sync(byte *buffer, size_t size);

/**
 * Read synchronously into the display, this is useful to receive command outputs,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`,
 * you can precisely control when to read data
 *
 * PARAMETERS
 * - buffer: the buffer to read into
 * - size: how many bytes should be read from the device
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
error_t st7789v_read_sync(byte *buffer, size_t size);

/**
 * This function tries to write asynchronously into the display, this is useful to large buffers,
 * like pixels, etc.
 *
 * PARAMETERS
 * - buffer: the buffer to write
 * - size: the size of the buffer to write
 * - data_size: the size of the buffer data, use DMA_SIZE_8 for bytes
 * - completion_signal: the semaphore to be released when the transaction finishes
 * - close_comm_when_finish: if the dma irq handler should call `st7789v_end_comm(void)` when finishes writing
 *
 * NOTES
 * - the `data_size` and `size` parameters are related, if you put DMA_SIZE_16, you'll need to provide
 *   the size of the buffer (`size`) into 16-bit values, and not the byte size of the buffer.
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_dma_write(
    byte *buffer,
    size_t size,
    enum dma_channel_transfer_size data_size,
    semaphore_t *completion_signal,
    bool close_comm_when_finish
);

/**
 * Waits for an DMA operation to finish on this display
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external error_t st7789v_sync_dma_operation(void);

/**
 * Send an command synchronously to the display
 *
 * PARAMETERS
 * - command: the command to send
 * - parameters: the parameters this command receive
 * - parameter_count: how many parameters has in the `paramaters` pointer
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
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
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_no_operation(void);

/**
 * Reset the display
 *
 * PARAMETERS
 * - sync_delay: if you want to wait the 5ms delay for the display to reset properly.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_software_reset(bool sync_delay);

/**
 * Read the display id (it should match with ST7789V_DISPLAY_ID)
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The display id
 */
external uint32_t st7789v_display_read_id(void);

/**
 * Read the display status
 *
 * PARAMETERS
 * - status: a pointer to an `st7789v_display_status_t` structure where
 *           this function will store the current display status.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display status
 */
external int32_t st7789v_display_read_status(st7789v_display_status_t *status);

/**
 * Read the display power mode
 *
 * PARAMETERS
 * - pwrmode: a pointer to an `st7789v_power_mode_t` structure where
 *            this function will store the power mode.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display power status value
 */
external byte st7789v_display_read_power_mode(st7789v_power_mode_t *pwrmode);

/**
 * Read the display memory access control
 *
 * PARAMETERS
 * - madctl: a pointer to an `st7789v_memory_access_control_t` structure where
 *           this function will store the current memory access control rules.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display memory access control value
 */
external byte st7789v_display_read_memory_access_control(st7789v_memory_access_control_t *madctl);

/**
 * Read the display memory access control
 *
 * PARAMETERS
 * - pixfmt: a pointer to an `st7789v_interface_pixel_format_t` structure where
 *           this function will store the pixel format.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display pixel format value
 */
external byte st7789v_display_read_pixel_format(st7789v_interface_pixel_format_t *pixfmt);

/**
 * Read the display image mode
 *
 * PARAMETERS
 * - img_mode: a pointer to an `st7789v_image_mode_t` structure where this function
 *             will store the current image mode.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display image mode value
 */
external byte st7789v_display_read_image_mode(st7789v_image_mode_t *img_mode);

/**
 * Read the display signal mode
 *
 * PARAMETERS
 * - signal_mode: a pointer to an `st7789v_signal_mode_t` structure where this function
 *                will store the current signal mode.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display signal mode value
 */
external byte st7789v_display_read_signal_mode(st7789v_signal_mode_t *signal_mode);

/**
 * Read the display self diagnostic
 *
 * PARAMETERS
 * - diag: a pointer to an `st7789v_self_diagnostic_t` structure where this function
 *         will store the diagnostic data.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display self diagnostic value
 */
external byte st7789v_display_read_self_diagnostic(st7789v_self_diagnostic_t *diag);

/**
 * Enter in the sleep mode of the display (sleep in)
 *
 * PARAMETERS
 * - sync_delay: if you want to wait the 120ms delay to send `sleep out` command
 *               synchronously, or you code after this will take this time to send
 *               other commands later.
 *
 * NOTES
 * - When you call this function, an `sleep_change_lock` is set in the driver,
 *   to prevent you from calling `sleep_out` before the display is ready. If
 *   you use `sync_delay`, this lock will be set and sleep the 120ms to unset it.
 *   The driver creates an interrupt-driven timer to unlock the `sleep_change_lock`
 *   state.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_sleep_in(bool sync_delay);

/**
 * Enter out of the sleep mode of the display (sleep out)
 *
 * PARAMETERS
 * - sync_delay: if you want to wait the 120ms delay to send `sleep in` command
 *               synchronously, or you code after this will take this time to send
 *               other commands later.
 *
 * NOTES
 * - When you call this function, an `sleep_change_lock` is set in the driver,
 *   to prevent you from calling `sleep_out` before the display is ready. If
 *   you use `sync_delay`, this lock will be set and sleep the 120ms to unset it.
 *   The driver creates an interrupt-driven timer to unlock the `sleep_change_lock`
 *   state.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_sleep_out(bool sync_delay);

/**
 * Set the display into normal mode, or partial mode, depending on the `enable` parameter.
 *
 * PARAMETERS
 * - enable: enable the normal mode state, if this is `false`, partial display mode
 *           is going to be enabled.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_normal_mode_state(bool enable);

/**
 * Enable display color inversion
 *
 * PARAMETERS
 * - enable: enable color inversion
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_enable_inversion(bool enable);

/**
 * Set display gamma correction curve
 *
 * PARAMETERS
 * - gamma_curve: the gamma curve to use
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_gamma_correction_curve(st7789v_gamma_curve_t gamma_curve);

/**
 * Turn on the display
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_turn_on(void);

/**
 * Turn off the display
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_turn_off(void);

/**
 * Set display column address window
 *
 * PARAMETERS
 * - start: the starting column address the display will use
 * - end: the ending column address the display will use
 *
 * NOTES
 * - The range for these parameters is:
 *   0 <= start < end <= 239  if st7789v_memory_access_control_t.row_column_exchange is true
 *   0 <= start < end <= 320  if st7789v_memory_access_control_t.row_column_exchange is false
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - ENOTINRANGE: if the values doesn't match the allowed ranges for addresses
 */
external error_t st7789v_display_set_column_address_window(uint16_t start, uint16_t end);

/**
 * Set display row address window
 *
 * PARAMETERS
 * - start: the starting row address the display will use
 * - end: the ending row address the display will use
 *
 * NOTES
 * - The range for these parameters is:
 *   0 <= start < end <= 239  if st7789v_memory_access_control_t.row_column_exchange is false
 *   0 <= start < end <= 320  if st7789v_memory_access_control_t.row_column_exchange is true
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - ENOTINRANGE: if the values doesn't match the allowed ranges for addresses
 */
external error_t st7789v_display_set_row_address_window(uint16_t start, uint16_t end);

/**
 * Write into the display memory synchronously
 *
 * PARAMETERS
 * - buffer: the buffer to write into the display's memory
 * - size: the size of the buffer
 * - continue_writing: if you want to continue an old write operation (using COMMAND_MEMORY_WRITE_CONTINUE)
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_memory_write_sync(byte *buffer, size_t size, bool continue_writing);

/**
 * Write into the display memory asynchronously using DMA operations
 *
 * PARAMETERS
 * - buffer: the buffer to write into the display's memory
 * - size: the size of the buffer
 * - completion_signal: the semaphore to release when the memory finishes writing
 * - continue_writing: if you want to continue an old write operation (using COMMAND_MEMORY_WRITE_CONTINUE)
 *
 * NOTES
 * - the DMA operation is called with `close_comm_when_finish` as true, so the communication will
 *   end as soon the operation finishes. DO NOT TRY SENDING COMMANDS WHILE THIS OPERATION IS ONGOING.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_memory_write_async(
    byte *buffer,
    size_t size,
    semaphore_t *completion_signal,
    bool continue_writing
);

/**
 * Read from the display memory synchronously
 *
 * PARAMETERS
 * - buffer: the buffer to write the data from the display's memory
 * - size: the size of the buffer
 * - continue_reading: if you want to continue an old read operation (using COMMAND_MEMORY_READ_CONTINUE)
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_memory_read_sync(byte *buffer, size_t size, bool continue_reading);

/**
 * Set display partial area
 *
 * PARAMETERS
 * - start: the starting line of the partial area
 * - end: the ending line of the partial area
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_partial_area(uint16_t start, uint16_t end);

/**
 * Set vertical scrolling parameters for display
 *
 * NOTES
 * - top_fixed_area + vertical_scrolling_area + bottom_fixed_area needs to be 320
 * - display's memory access control configuration should have `row_column_exchange` set to false.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - ENOTINRANGE: if the sum of all parameters is not 320
 * - EUNAVAILABLE: if this function is not available because of the configuration of the display
 */
external error_t st7789v_display_set_vertical_scrolling_parameters(
    uint16_t top_fixed_area,
    uint16_t vertical_scrolling_area,
    uint16_t bottom_fixed_area
);

/**
 * Enable or disable tearing line effect
 *
 * PARAMETERS
 * - enable: if you want to enable the tearing line effect
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_tearing_line_effect_enabled(bool enable);

/**
 * Configure display's memory access control
 *
 * PARAMETERS
 * - madctl: the new memory access control configuration to use
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_memory_access_control(st7789v_memory_access_control_t madctl);

/**
 * Set vertical scroll start address of RAM
 *
 * PARAMETERS
 * - address: the address where the vertical scroll will start
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_vertical_scrolling_start_address(uint16_t address);

/**
 * Set display idle mode
 *
 * PARAMETERS
 * - enable: if you want to enable idle mode
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_idle(bool enable);

/**
 * Set display interface pixel format
 *
 * PARAMETERS
 * - colmod: the new interface pixel format to be used in the display
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_pixel_format(st7789v_interface_pixel_format_t colmod);

/**
 * Set display tearing scanline
 *
 * PARAMETERS
 * - scanline_number: the new tear scanline number
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_tear_scanline(uint16_t scanline_number);

/**
 * Get display scanline
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The current scanline number
 */
external uint16_t st7789v_display_read_scanline(void);

/**
 * Set the display brightness
 *
 * PARAMETERS
 * - value: the display brightness value, from 0 to 255, where 255 is the brightest, and 0 is lowest.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_display_brightness(byte value);

/**
 * Get the display brightness
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The current scanline number
 */
external byte st7789v_display_read_display_brightness(void);

/**
 * Set the display control register
 *
 * PARAMETERS
 * - ctrl: the new options for the control register of this display
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_ctrl_register(st7789v_display_ctrl_t ctrl);

/**
 * Get the display control register
 *
 * PARAMETERS
 * - ctrl: a pointer to where this will store the new ctrl register values
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - the raw display control register value
 */
external uint32_t st7789v_display_read_ctrl_register(st7789v_display_ctrl_t *ctrl);

/**
 * Write the display's content adaptive brightness and color enhancement
 *
 * PARAMETERS
 * - coca: the new content adaptive brightness and color enhancement settings for the display
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_adaptive_brightness_color_enhancement(
    st7789v_adaptive_brightness_color_enhancement_t coca
);

/**
 * Read the display's content adaptive brightness
 *
 * PARAMETERS
 * - brightness: a pointer to where this will store the brightness values
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - the raw content adaptive brightness and color enhancement value
 */
external byte st7789v_display_read_content_adaptive_brightness(
    st7789v_content_adaptive_brightness_t *brightness
);

/**
 * Write the display's minimum content adaptive brightness
 *
 * PARAMETERS
 * - value: the new minimum brightness value for the display
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_display_set_content_adaptive_minimum_brightness(byte value);

/**
 * Read the display's minimum content adaptive brightness
 *
 * PARAMETERS
 * - value: the new minimum brightness value for the display
 *
 * RETURN VALUE:
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external byte st7789v_display_read_content_adaptive_minimum_brightness(void);

/**
 * Read the display self diagnostic
 *
 * PARAMETERS
 * - diag: a pointer to an `st7789v_self_diagnostic_t` structure where this function
 *         will store the diagnostic data.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display adaptive brightness self diagnostic value
 */
external byte st7789v_display_read_adaptive_brightness_control_self_diagnostic(
    st7789v_self_diagnostic_t *diag
);

/**
 * Read the display id 1
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The display id 1
 */
external byte st7789v_display_read_id_1(void);

/**
 * Read the display id 2
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The display id 2
 */
external byte st7789v_display_read_id_2(void);

/**
 * Read the display id 3
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The display id 3
 */
external byte st7789v_display_read_id_3(void);

/**
 * Deinitializes this display
 *
 * RETURN VALUE
 * - This function always returns zero.
 */
external error_t st7789v_deinit(void);

#endif /** DRIVERS_ST7789V_H */
