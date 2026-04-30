cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ferrit C CXX ASM)

pico_sdk_init()

add_executable(ferrit)

pico_generate_pio_header(ferrit
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit_spi.pio
  )

target_sources(ferrit PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ferrit_pio_spi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/mapper.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/drv_fram.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_msc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
  )

target_include_directories(ferrit PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

target_compile_definitions(ferrit PUBLIC
  PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
  )

target_link_libraries(ferrit PRIVATE
  pico_stdlib
  hardware_resets
  hardware_irq
  hardware_pio
  tinyusb_device
  )

pico_add_extra_outputs(ferrit)
