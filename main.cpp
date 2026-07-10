#include <stdio.h>
#include <string>
#include <time.h>
#include <cstdint>
#include <hardware/sync.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
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

using namespace pimoroni;

// Instantiate Screen
myScreen screen;

// Instantiate buttons & button manager
ButtonManager buttonmgr;

// Instantiate LED
myLED led;

// Initialize WiFi chipset
void init_wifi(){
    if(cyw43_arch_init()) {
        screen.show_boot_status("ERROR INITIALIZING WIFI CHIPSET", "red");
        sleep_ms(3000);
        watchdog_reboot(0,0,0);
    } else {
        screen.show_boot_status("WIFI CHIPSET OK", "green");
    }

    // Enable chipset operation
    cyw43_arch_enable_sta_mode();
    screen.show_boot_status("WIFI ARCH OK", "green");

    // Connect to WiFi network
    std::string wifi_name = read_key("CONFIG.TXT", "WIFI_NAME");
    std::string wifi_password = read_key("CONFIG.TXT", "WIFI_PASSWORD");
    screen.show_boot_status("WIFI CONFIG FOUND", "green");

    for(int attempt = 0; attempt < 3; attempt++){
        if (cyw43_arch_wifi_connect_timeout_ms(wifi_name.c_str(), wifi_password.c_str(), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            std::string msg = "TRYING TO CONNECT TO WIFI #" + std::to_string(attempt+1);
            screen.show_boot_status(msg, "orange");
        } else {
            screen.show_boot_status("CONNECTED TO WIFI NETWORK", "green");
            sleep_ms(2000);
            break;
        }
    }
}

void init_sntp(std::string sntp_server) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // Poll mode for periodic updates
    sntp_setservername(0, sntp_server.c_str());   // Set NTP server
    sntp_init();  // Start SNTP service

    screen.show_boot_status("SNTP SERVER FOUND", "green");

}

bool time_synched = false;
void sntp_callback(time_t sec, suseconds_t us) {
    time_synched = true;
    struct timeval tv = {sec, us};
    settimeofday(&tv, NULL);
}

// Get formatted current time as string
std::string get_time() {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    
    char buffer[9]; // "hh:mm:ss" format 9 chars (8+null)
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
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

void expose_drive(){
    unmountfs();
    sleep_ms(1000);
    usb_msc_init();
    sleep_ms(1000);

    absolute_time_t last_status_update = get_absolute_time();

    screen.show_boot_status("PICO IS AVAILABLE ON YOUR PC");
    while(pico_mounted()){
        tud_task();

        absolute_time_t now = get_absolute_time();

        if (absolute_time_diff_us(last_status_update, now) >= 1000000) {
            screen.show_boot_status("PICO IS AVAILABLE ON YOUR PC");
            last_status_update = now;
        } else {
            sleep_ms(1);
        }
    }

    screen.show_boot_status("PICO EJECTED", "green");
    ram_disk_flush_to_flash();
    sleep_ms(5000);

    screen.show_boot_status("REBOOTING...", "red");
    sleep_ms(5000);
    watchdog_reboot(0,0,0);
    // just waiting for the reboot
    while(true){
        sleep_ms(1000);
    }
}

void init_config_file(){
    screen.show_boot_status("CREATING CONFIG FILE.");
    write_key("CONFIG.TXT", "WIFI_NAME", "WRITE WIFI NAME HERE");
    screen.show_boot_status("CREATING CONFIG FILE..");
    write_key("CONFIG.TXT", "WIFI_PASSWORD", "WRITE WIFI PASSWORD HERE");
    screen.show_boot_status("CREATING CONFIG FILE...");
}

void init_filesystem(){

    ram_disk_load_from_flash();
    FRESULT res = mountfs();

    if (res == FR_OK) {
        screen.show_boot_status("FILESYSTEM MOUNTED", "green");
        // filesystem exists, but there is no CONFIG file
        if (file_exists("CONFIG.TXT") != FR_OK){
            init_config_file();
        }
        return;
    } else {
        ram_disk_format_fat12();
        ram_disk_flush_to_flash();
        screen.show_boot_status("FILESYSTEM FORMATTED", "orange");
    }

    ram_disk_load_from_flash();
    res = mountfs();

    if (res == FR_OK) {
        screen.show_boot_status("FILESYSTEM MOUNTED", "green");
    } else {
        screen.show_boot_status("FILESYSTEM ERROR", "red");
        sleep_ms(5000); // need to decide what to do here
    }

    // create the config file
    init_config_file();

    sleep_ms(3000);
    screen.show_boot_status("CONFIGURE WIFI FROM YOUR PC");
    sleep_ms(5000);
    expose_drive();
}


int main() {

    // Initialize Pico
    stdio_init_all();

    // bootup screen 
    screen.set_pen("black");
    screen.clear();
    screen.show_boot_status("INITIALIZING SCREEN...");
    screen.draw_logo("PICO CLOCK");
    screen.show_boot_status("SCREEN OK", "green");

    // FATFS & file system 
    screen.show_boot_status("INITIALIZING FILESYSTEM...");
    init_filesystem();

 
    // this needs to be moved to a function button screeen 
    // std::string board_id = info_board();
    // screen.writeln("BOARD ID: " + board_id,"green");
    // std::string voltage_id = info_voltage();
    // //screen.writeln("VOLTAGE: " + voltage_id + "V","pink");

    init_wifi();
    init_sntp("pool.ntp.org");

    screen.show_boot_status("SYNCHRONIZING TIME...");
    while (!time_synched) {
        sleep_ms(50); // wait for sntp to sync clock
    }
    screen.show_boot_status("TIME IS SYNCHRONIZED", "green");

    std::string time_string;

    while(true) {
        buttonmgr.update();
        //if (buttonmgr.is_a()) led.new_blink(5,500,"blue");
    if (buttonmgr.is_bootsel_long()) {
        reset_usb_boot(0, 0);
    } else if (buttonmgr.is_a())  {
        expose_drive();
    } else if (buttonmgr.is_bootsel_single()) {
        watchdog_reboot(0, 0, 1000);
    }
    time_string = "CURRENT TIME: " + get_time();
    screen.show_boot_status(time_string, "yellow");
    led.blink_update();
    screen.update();
    }
}