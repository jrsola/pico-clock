cmake_minimum_required(VERSION 3.13)

# Set the project name from the external file
set(PROJECT_NAME_FILE "${CMAKE_CURRENT_SOURCE_DIR}/project_name.txt")
if(EXISTS ${PROJECT_NAME_FILE})
    file(READ ${PROJECT_NAME_FILE} FILE_CONTENT)
    list(GET FILE_CONTENT 0 NAME)
    message(STATUS "PROJECT NAME: ${NAME}")
else()
    message(STATUS "WARNING: PROJECT NAME IS MISSING, USING DEFAULT")
    set(NAME "default")
endif()

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

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(${NAME} 
    pico_stdlib 
    pico_cyw43_arch_none
    )

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${NAME})

# Move the generated .uf2 from build -> ..
add_custom_command(TARGET ${NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2"
    "/workspace/${NAME}.uf2"
)