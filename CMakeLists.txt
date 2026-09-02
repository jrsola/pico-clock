cmake_minimum_required(VERSION 3.13)

# Set the project name
set(NAME "pico-clock")
message(STATUS "PROJECT NAME: ${NAME}")

# state the pico version here for WiFi to work
# pico/pico_w/pico2/pico2_w
set(PICO_BOARD pico_w) 

# switch to use pimoroni libraries
# yes/no
set(USE_PIMORONI yes)
message(STATUS "Pimoroni Libraries: ${USE_PIMORONI}")

# Enable lwIP and Wi-Fi support
set(PICO_CYW43_SUPPORTED 1)
set(PICO_LWIP 1)
set(PICO_LWIP_THREADS 1)  # Enable thread-safe background network

# Enable TinyUSB
set(PICO_TINYUSB_ENABLE 1)

# Initialize Pico SDK 
# Note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

if(USE_PIMORONI STREQUAL "yes")
    include($ENV{PIMORONI_PICO_PATH}/pimoroni_pico_import.cmake)
endif()

# Add the project libraries to be included
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/project_libraries)


# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# shrink usable size for programs, so a filesystem 
# can be placed safely at the end of the flash space
# size of flash in the pico
set(PICO_TOTAL_FLASH_KB 2048)
# size of filesystem in KB
set(PICO_DISK_SIZE_KB 64)

message(STATUS "Reserved disk size: ${PICO_DISK_SIZE_KB} KB")

# adjust space for programs accordingly
math(EXPR PICO_FLASH_SIZE "(${PICO_TOTAL_FLASH_KB} - ${PICO_DISK_SIZE_KB}) * 1024")

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp
    project_libraries/color.cpp
    project_libraries/my_screen.cpp
    project_libraries/my_led.cpp
    project_libraries/buttonmgr.cpp
    project_libraries/msc_disk.cpp
    project_libraries/diskio.cpp
    project_libraries/filesystem.cpp
    project_libraries/usb_descriptors.cpp
    project_libraries/ram_disk.cpp
    project_libraries/network.cpp
)

target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/project_libraries
    ${CMAKE_CURRENT_SOURCE_DIR}/project_libraries/fatfs
)

# Adding some libraries here would save you from having to use subdirectories
# in your program #INCLUDES i.e. "/drivers/st7789/st7789.hpp" vs just "st7789.hpp" 
# it's for "complex" libraries that have external dependencies
add_subdirectory(${PIMORONI_PICO_PATH}/drivers/st7789 st7789)
add_subdirectory(${PIMORONI_PICO_PATH}/libraries/pico_graphics pico_graphics)
add_subdirectory(${PIMORONI_PICO_PATH}/libraries/pico_display_2 pico_display_2)

# Include required libraries
include(common/pimoroni_bus)
include(drivers/rgbled/rgbled)
include(drivers/button/button)

# Use our own local FatFs copy, not the one from Pimoroni
set(PROJECT_FATFS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/project_libraries/fatfs")

add_library(fatfs STATIC
    ${PROJECT_FATFS_DIR}/ff.c
)

target_include_directories(fatfs PUBLIC
    ${PROJECT_FATFS_DIR}
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(${NAME} 
    pico_stdlib 
    #pico_cyw43_arch_none
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_sntp
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
    hardware_clocks
    hardware_pio
    st7789
    pico_display_2
    pico_graphics
    rgbled
    button
    pico_lwip_sntp
    hardware_flash
    hardware_adc
    hardware_watchdog
    tinyusb_device
    tinyusb_board
    fatfs
    )

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${NAME})

# Move the generated .uf2 from build -> ..
add_custom_command(TARGET ${NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2"
    "/workspace/${NAME}.uf2"
)