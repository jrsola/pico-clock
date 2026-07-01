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

# Load Default Wi-Fi values (SSID/PASSWORD) from wifi_credentials.txt
set(WIFI_CREDENTIALS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/wifi_credentials.txt")
if(EXISTS ${WIFI_CREDENTIALS_FILE})
    file(READ ${WIFI_CREDENTIALS_FILE} FILE_CONTENT)
    string(REGEX REPLACE "\n" ";" CREDENTIALS_LIST "${FILE_CONTENT}")
    list(GET CREDENTIALS_LIST 0 WIFI_SSID)
    list(GET CREDENTIALS_LIST 1 WIFI_PASSWORD)
    message(STATUS "WIFI_SSID: ${WIFI_SSID}")
    message(STATUS "WIFI_PASSWORD: ${WIFI_PASSWORD}")
    add_definitions(-DWIFI_SSID="${WIFI_SSID}")
    add_definitions(-DWIFI_PASSWORD="${WIFI_PASSWORD}")
else()
    message(STATUS "WARNING: WIFI CREDENTIALS NOT DEFINED")
endif()

# Initialize the Pico SDK 
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

# Shrink the usable size for programs in 128 KB 
# so we can safely place a file system
set(PICO_FLASH_SIZE 0x1E0000)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp
    project_libraries/color.cpp
    project_libraries/my_screen.cpp
    project_libraries/my_led.cpp
    project_libraries/bootsel.cpp
    project_libraries/buttonmgr.cpp
    project_libraries/msc_disk.cpp
    project_libraries/diskio.cpp
    project_libraries/filesystem.cpp
    project_libraries/usb_descriptors.cpp
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