# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD                11)
set(CMAKE_CXX_STANDARD              17)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)

include(external/pico_sdk_import.cmake)

project(st7789v_drv C CXX ASM)

pico_sdk_init()

aux_source_directory(drivers DRIVERS_SOURCES)
aux_source_directory(app APP_SOURCES)
aux_source_directory(hal HAL_SOURCES)
aux_source_directory(init INIT_SOURCES)
aux_source_directory(util UTIL_SOURCES)

add_executable(scicalculator ${INIT_SOURCES} ${UTIL_SOURCES} ${DRIVERS_SOURCES} ${APP_SOURCES} ${HAL_SOURCES})

pico_enable_stdio_uart(scicalculator DISABLED)
pico_enable_stdio_usb(scicalculator ENABLED)

target_link_libraries(scicalculator pico_stdlib hardware_spi hardware_dma)

target_compile_options(scicalculator PUBLIC "-fms-extensions" "-Wno-microsoft-anon-tag")

target_include_directories(scicalculator PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(scicalculator)

