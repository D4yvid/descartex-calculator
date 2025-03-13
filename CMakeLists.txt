cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD                11)
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)

set(HAL_SOURCES                     "")
set(USE_ST7789V_DISPLAY             TRUE)

include(external/pico_sdk_import.cmake)

project(descartex C CXX ASM)

pico_sdk_init()

aux_source_directory(drivers DRIVERS_SOURCES)
aux_source_directory(app APP_SOURCES)
aux_source_directory(init INIT_SOURCES)
aux_source_directory(util UTIL_SOURCES)
aux_source_directory(display DISPLAY_SOURCES)

list(APPEND COMPILE_FLAGS "-fms-extensions")
list(APPEND COMPILE_FLAGS "-Wno-packed-bitfield-compat")

if (USE_ST7789V_DISPLAY)
    list(APPEND COMPILE_FLAGS "-DHAL_USE_ST7789V")
    list(APPEND HAL_SOURCES   "hal/display_st7789v.c")
endif ()

add_executable(descartex
    ${INIT_SOURCES}
    ${DISPLAY_SOURCES}
    ${UTIL_SOURCES}
    ${DRIVERS_SOURCES}
    ${APP_SOURCES}
    ${HAL_SOURCES}
)

pico_enable_stdio_usb(descartex ENABLED)
pico_enable_stdio_uart(descartex DISABLED)

target_link_libraries(descartex pico_stdlib hardware_spi hardware_dma)
target_compile_options(descartex PUBLIC ${COMPILE_FLAGS})

target_include_directories(descartex PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(descartex)
