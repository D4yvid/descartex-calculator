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
#define ST7789V_SPI_BAUDRATE        (6625 * 1000)
#define ST7789V_READING_BAUDRATE    (6666 * 1000)
#define ST7789V_WRITING_BAUDRATE    (15151 * 1000)

#define ST7789V_PIN_MISO        16
#define ST7789V_PIN_CS          17
#define ST7789V_PIN_SCK         18
#define ST7789V_PIN_MOSI        19
#define ST7789V_PIN_DC          20

/********************* DISPLAY CONSTANTS **************************/
#define ST7789V_DISPLAY_ID      0x858552
#define ST7789V_DISPLAY_WIDTH   319
#define ST7789V_DISPLAY_HEIGHT  239

typedef enum st7789v_error_t
{
    ENODISPLAYCONNECTED = 0x01,
    EDISPLAYBUSY        = 0x02,
    EINVALIDSTATE       = 0x03,
    ENODMAAVAILABLE     = 0x04,
    ENOTINRANGE         = 0x05
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
typedef enum st7789v_command_t: uint8_t
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
    COMMAND_PARTIAL_DISPLAY_MODE_ON                        = 0x12,
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
 * The color order for pixels sent
 */
typedef enum st7789v_color_order_t: uint8_t
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
typedef enum st7789v_pixel_format_t: uint8_t
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
typedef enum st7789v_rgb_interface_format: uint8_t
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
typedef enum st7789v_tearing_effect_mode_t: uint8_t
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
typedef enum st7789v_gamma_curve_t: uint8_t
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
 * The almost full display status (is missing some data you can query
 * separately)
 *
 * Read this structure from the display by using `st7789v_display_read_status`.
 */
typedef union st7789v_display_status_t
{
    uint32_t raw_value;

    struct {
        uint8_t                                                     : 5;

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
        uint8_t                                                     : 2;

        /**
         * If color inversion is applied in the display's pixels
         */
        bool                            color_inversion             : 1;
        uint8_t                                                     : 2;

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

        uint8_t                                                     : 2;

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
    uint8_t raw_value;

    struct {
        uint8_t                                 : 2;
        
        /**
         * If the display memory is being used from right to left
         */
        bool        horizontal_order_rtl        : 1;

        /**
         * If the display memory is being used with BGR order instead of RGB order
         */
        bool        bgr_pixels                  : 1;

        /**
         * If the display memory is being read with the scan address incrementing
         */
        bool        scan_address_increment      : 1;
                
        /**
         * If the display memory is being read with the column and row addresses
         * exchanged
         */
        bool        row_column_exchange         : 1;

        /**
         * If the display memory is being read with the column address decrementing
         */
        bool        column_address_decrement    : 1;
                
        /**
         * If the display memory is being read with the row address decrementing
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
    uint8_t raw_value;

    struct {
        uint8_t                             : 2;
        
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
    uint8_t raw_value;

    struct {
        uint8_t                                         : 1;

        /**
         * How many colors the display is using to show the image
         */
        st7789v_rgb_interface_format_t  rgb_format      : 3;

        uint8_t                                         : 1;

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
    uint8_t raw_value;

    struct {
        /**
         * The gamma curve the display is using
         */
        st7789v_gamma_curve_t   gamma_curve         : 3;
        uint8_t                                     : 2;

        /**
         * If the display has color inversion enabled
         */
        bool                    color_inversion     : 1;
        uint8_t                                     : 1;
        
        /**
         * If vertical scrolling is being used by the display
         */
        bool                    vertical_scrolling  : 1;
    } __attribute__((packed));
} st7789v_image_mode_t;

/**
 *
 */
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
external error_t st7789v_begin_comm();

/**
 * End a communication with the display (set CS to LOW)
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external error_t st7789v_end_comm();

/**
 * Begin the writing of an command, if you're sending a Command, PLEASE use this at the beginning
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external void st7789v_begin_command();

/**
 * End the writing of an command, if you're not sending a command, for example, sending pixels or
 * parameters for the command, call this when the writing of the command ends
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external void st7789v_end_command();

/**
 * Write synchronously into the display, this is useful to send commands,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`
 * 
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 */
external error_t st7789v_write_sync(byte *buffer, size_t length);

/**
 * Read synchronously into the display, this is useful to receive command outputs,
 * as all command routines need to be timed with `st7789v_begin_command` and `st7789v_end_command`,
 * you can precisely control when to read data
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
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
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
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 */
external error_t st7789v_sync_dma_operation(void);

/**
 * Send an command synchronously to the display
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
 * - diag: a pointer to an `st7789v_display_self_diagnostic_t` structure where this function
 *         will store the diagnostic data.
 *
 * RETURN VALUE
 * - ENODISPLAYCONNECTED: if the display is not plugged in, or unavailable
 * - EDISPLAYBUSY: if the display is busy with an DMA or reset operation
 * - The raw display self diagnostic value
 */
external byte st7789v_display_read_self_diagnostic(st7789v_display_self_diagnostic_t *diag);

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
external error_t st7789v_display_set_row_window(uint16_t start, uint16_t end);

/**
 * Deinitializes this display
 *
 * RETURN VALUE
 * - This function always returns zero.
 */
external error_t st7789v_deinit(void);

#endif /** DRIVERS_ST7789V_H */
