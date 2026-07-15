#include <stdio.h>
#include <string>
#include <time.h>
#include <cstdint>
#include <hardware/sync.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
//#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "tusb.h"
#include "ff.h"
#include "diskio.h"


#include "pico/unique_id.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"

#include "button.hpp"

#include "project_libraries/color.h"
#include "project_libraries/my_screen.h"
#include "project_libraries/my_led.h"
#include "project_libraries/bootsel.h"
#include "project_libraries/buttonmgr.h"
#include "project_libraries/msc_disk.h"
#include "project_libraries/filesystem.h"
#include "project_libraries/ram_disk.h"
#include "project_libraries/network.h"

using namespace pimoroni;

// Instantiate Screen
myScreen screen;

// Instantiate buttons & button manager
ButtonManager buttonmgr;

// Instantiate LED
myLED led;


// Get formatted current time as string
std::string get_time(int tz_offset) {
    time_t now = time(NULL);
    now += tz_offset * 3600;
    struct tm *timeinfo = localtime(&now);
    
    char buffer[9]; // "hh:mm:ss" format 9 chars (8+null)
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
    
    return std::string(buffer);
} 

std::string info_board(){
    char id_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(id_str, sizeof(id_str));
    return std::string(id_str);
}

std::string info_voltage(){
    adc_init();
    adc_select_input(3);

    uint16_t raw = adc_read();
    float voltage = raw * 3.3f / (1 << 12) * 3.0f;
    return std::to_string(voltage);
}

void reboot(){
    screen.show_boot_message("REBOOTING", "red");
    for (int i=0; i<=5; i++){
        sleep_ms(300);
        screen.show_boot_message();
    }
        sleep_ms(1000);
        watchdog_reboot(0,0,0);
}

void expose_drive(){
    unmountfs();
    sleep_ms(1000);
    usb_msc_init();
    sleep_ms(1000);

    absolute_time_t last_status_update = get_absolute_time();

    screen.show_boot_message("PICO IS AVAILABLE ON YOUR PC");
    while(pico_mounted()){
        tud_task();

        absolute_time_t now = get_absolute_time();

        if (absolute_time_diff_us(last_status_update, now) >= 1000000) {
            screen.show_boot_message("PICO IS AVAILABLE ON YOUR PC");
            last_status_update = now;
        } else {
            sleep_ms(1);
        }
    }

    screen.show_boot_message("PICO EJECTED", "green");
    ram_disk_flush_to_flash();
    sleep_ms(5000);

    reboot();
}

// this needs to be moved to a function button screeen 
// std::string board_id = info_board();
// screen.writeln("BOARD ID: " + board_id,"green");
// std::string voltage_id = info_voltage();
// //screen.writeln("VOLTAGE: " + voltage_id + "V","pink");



int main() {

    // initialize Pico
    stdio_init_all();

    // initialize and bootup screen 
    screen.clear("dark blue",5);
    screen.draw_logo("PICO CLOCK");
    screen.show_boot_message("SCREEN INITIALIZED", "green");

    // tinyFS file system 
    screen.show_boot_message("INITIALIZING FILESYSTEM");
    ram_disk_load_from_flash();
    FRESULT res = mountfs();
    if (res == FR_OK) {
        screen.show_boot_message("FILESYSTEM MOUNTED", "green");
    } else {
        ram_disk_format_fat12();
        ram_disk_flush_to_flash();
        screen.show_boot_message("FILESYSTEM FORMATTED", "orange");
        res = mountfs();
        if (res != FR_OK) {
            screen.show_boot_message("FILESYSTEM ERROR", "red");
            reboot();
        }
    }
    // filesystem exists, now if CONFIG file is missing, create it
    // and expose drive to PC to configure WIFI
    if (file_exists("CONFIG.TXT") != FR_OK){
        screen.show_boot_message("CREATING CONFIG FILE", "orange");
        write_key("CONFIG.TXT", "WIFI_NAME", "WRITE WIFI NAME HERE");
        screen.show_boot_message();
        write_key("CONFIG.TXT", "WIFI_PASSWORD", "WRITE WIFI PASSWORD HERE");
        screen.show_boot_message();
        screen.show_boot_message("EDIT CONFIG FILE FROM YOUR PC");
        sleep_ms(3000);
        expose_drive();
        reboot();
    }
    screen.show_boot_message("FILESYSTEM INITIALIZED");

    // wifi and network initialization
    if(wifi_init()) {
        screen.show_boot_message("WIFI CHIPSET OK", "green");
        screen.show_boot_message("WIFI ARCH OK", "green");
    } else {
        screen.show_boot_message("ERROR INITIALIZING WIFI CHIPSET", "red");
        reboot();
    }

    // connect to WiFi network
    screen.show_boot_message("CONNECTING TO WIFI NETWORK");
    if (wifi_connect(read_key("CONFIG.TXT", "WIFI_NAME"), read_key("CONFIG.TXT", "WIFI_PASSWORD"))){
        screen.show_boot_message("CONNECTED TO WIFI NETWORK", "green");
    } else {
        screen.show_boot_message("ERROR CONNECTING TO WIFI", "red");
        sleep_ms(3000);
        screen.show_boot_message("CHECK NETWORK CONFIG", "orange");
        sleep_ms(3000);
    }

    screen.show_boot_message("STARTING SNTP CLIENT");
    sntp_start();
    screen.show_boot_message("SNTP CLIENT STARTED", "green");

    screen.show_boot_message("SYNCHRONIZING TIME...");
    while (!network_time_is_synced()) {
        sleep_ms(50); // wait for sntp to sync clock
    }
    screen.show_boot_message("TIME IS SYNCHRONIZED", "green");

    if (read_key("CONFIG.TXT", "TIMEZONE").empty()){
        write_key("CONFIG.TXT", "TIMEZONE", "0");
        screen.show_boot_message("CONFIGURE TZ IN CONFIG FILE", "orange");
    }
    int tz_offset = std::stoi(read_key("CONFIG.TXT", "TIMEZONE"));
    screen.show_boot_message("TZ LOADED", "orange");

    std::string time_string;

    screen.clear("black",20);

    //draw initial buttonhints aka corners
    screen.set_buttonhint(3, Icons::HEART, "yellow");
    screen.draw_buttonhints();

    // main loop
    while(true) {
        buttonmgr.update();
        //if (buttonmgr.is_a()) led.new_blink(5,500,"blue");
        if (buttonmgr.is_bootsel_long()) reset_usb_boot(0, 0);
        if (buttonmgr.is_a())  expose_drive();
        if (buttonmgr.is_bootsel_single()) reboot();
        if (buttonmgr.is_y()) screen.set_buttonhint(3,nullptr);

        screen.draw_clock_time(get_time(tz_offset), "white", 7);
        screen.draw_buttonhints();
        led.blink_update();
        screen.update();
    }
}