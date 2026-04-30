cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(ferrit_test C CXX ASM)

pico_sdk_init()

add_executable(ferrit_test)

# Generate ferrit_spi.pio.h from ferrit_spi.pio
pico_generate_pio_header(ferrit_test
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit_spi.pio
)

target_sources(ferrit_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit_pio_spi.c
)

target_include_directories(ferrit_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

pico_enable_stdio_usb(ferrit_test 1)
pico_enable_stdio_uart(ferrit_test 0)

target_compile_definitions(ferrit_test PUBLIC
    PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
)

target_link_libraries(ferrit_test PRIVATE
    pico_stdlib
    pico_stdio_usb
    hardware_resets
    hardware_irq
    hardware_pio
)

pico_add_extra_outputs(ferrit_test)
