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

#include "button.hpp"

#include "project_libraries/color.h"
#include "project_libraries/my_screen.h"
#include "project_libraries/my_led.h"
#include "project_libraries/bootsel.h"
#include "project_libraries/buttonmgr.h"
#include "project_libraries/msc_disk.h"
#include "project_libraries/filesystem.h"


using namespace pimoroni;

// Instantiate Screen
myScreen screen;

// Instantiate buttons & button manager
ButtonManager buttonmgr;

// Instantiate LED
myLED led;

void init_screen(){
    screen.set_brightness(200);
    screen.set_pen("light blue");
    screen.clear();
    screen.update();
    sleep_ms(1000);
    screen.set_pen("dark blue");
    screen.rectangle(10,10,screen.get_width()-20, screen.get_height()-20);
    screen.update(); 
    screen.writeln("SCREEN: OK","white");
}

// Initialize WiFi chipset
void init_wifi(){
    if(cyw43_arch_init()) {
        screen.writeln("ERROR INITIALIZING WIFI CHIPSET", "red");
    } else {
        screen.writeln("WIFI CHIPSET: OK", "white");
    }

    // Enable chipset operation
    cyw43_arch_enable_sta_mode();

    // Connect to WiFi network
    for(int attempt = 0; attempt < 3; attempt++){
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            std::string msg = "ERROR IN WIFI NETWORK #" + std::to_string(attempt+1);
            screen.writeln(msg, "red");
        } else {
            screen.writeln("WIFI NETWORK: OK", "white");
            break;
        }
    }
}

void init_sntp(std::string sntp_server) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // Poll mode for periodic updates
    sntp_setservername(0, sntp_server.c_str());   // Set NTP server
    sntp_init();  // Start SNTP service

    screen.writeln("SNTP SERVER FOUND: OK", "pink");
    led.set_rgb("pink");
}

bool time_synched = false;
void sntp_callback(time_t sec, suseconds_t us) {
    time_synched = true;
    led.set_rgb("orange");
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

void init_filesystem(){
    disk_initialize(0);
    if (!mountfs()) {
        screen.writeln("ERROR INITIALIZING FATFS", "red");
    } else {
        screen.writeln("FATFS DISK INITIALIZED", "white");
        
    };
    // configuration files, each holds a value. 
    // FAT12 (8.3) format for filenames
    std::string textu = "AMIGARULEZ";
     if (!write_file("/CONFIG.TXT","WIFI_NAME\nWIFI_PASSWORD"))
        screen.writeln("ERROR WRITING CONFIG FILE", "red");
    else 
        screen.writeln("CONFIG FILE WRITTEN", "green");
    return;
}

int main() {

    // Initialize Pico
    stdio_init_all();

    // Initialize screen 
    init_screen();
 
    // Initialize FATFS & file system 
    init_filesystem();
    usb_msc_init();

    // Mount or format LittleFS partition
    //std::string config_file = "BOOTCNT.TXT"; // FAT12 (8.3)
    //std::string boot_count;
    /* boot_count = readfilestr(config_file);
    boot_count = "1";
    if (boot_count == "ERROR") {
        boot_count = "0";
        int err = writefilestr(config_file, boot_count);
        if  (err != FR_OK) screen.writeln("I DID NOT WRITE WELL");
    } */
    //createconfig(config_file, "MIMAMAMEMIMA");
    //boot_count = std::to_string(std::stoi(boot_count)+1); // Add 1 to bootcount
    //writefilestr(config_file, boot_count);
    //boot_count = readfilestr(config_file);
    //screen.writeln("NUMBER OF BOOTUPS: " + boot_count,"white");

    //boot_count = std::to_string(std::stoi(boot_count) + 1);
    //write2file("boot_count.cfg",boot_count);
    //screen.writeln("NUMBER OF BOOTUPS: " + boot_count,"white");
    std::string board_id = info_board();
    screen.writeln("BOARD ID: " + board_id,"green");
    std::string voltage_id = info_voltage();
    //screen.writeln("VOLTAGE: " + voltage_id + "V","pink");


    init_wifi();
    init_sntp("pool.ntp.org");

    screen.writeln("SYNCRONIZING TIME","white");
    while (!time_synched) {
        sleep_ms(50); // wait for sntp to sync clock
    }
    screen.writeln("TIME SYCHRONIZED","green");
    screen.writeln(); // empty line

    screen.writeln("TIME IS: ", "orange");

    std::string time_string;
    led.set_rgb("light blue");
    time_string = get_time();

    while(true) {
/*         // detect if the A button is pressed (could be A, B, X, or Y)
        if(button_a.raw() && !button_y.raw()) {
            led.new_blink(5,500,"blue");
        } 
        if(button_a.raw() && button_y.raw()) {
            reset_usb_boot(0, 0);
        } 

        if(get_bootsel_button()) {
            led.new_blink(5,500,"green");
        }

        led.blink_update();
        if(get_time() != time_string){
            screen.writexy(120,150,time_string, "dark blue");
            time_string = get_time();
            screen.writexy(120,150,time_string, "white");
        }; */
        tud_task();
        buttonmgr.update();
        if (buttonmgr.is_a()) led.new_blink(5,500,"blue");
        if (buttonmgr.get_bootsel_event()== BootselEvent::DoublePress) reset_usb_boot(0,0);
        led.blink_update();
    } 

}