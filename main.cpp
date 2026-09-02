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
#include "project_libraries/buttonmgr.h"
#include "project_libraries/msc_disk.h"
#include "project_libraries/filesystem.h"
#include "project_libraries/ram_disk.h"
#include "project_libraries/network.h"

const int CLOCK_SIZE = 7;

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

void show_info(){
    screen.clear_buttonhint_all();
    screen.clear();
    screen.set_buttonhint('y', Icons::BACK, "orange");
    screen.draw_buttonhints();
    // this needs to be moved to a function button screeen 
    std::string board_id = info_board();
    screen.writeln("BOARD ID: " + board_id,"green");
    std::string voltage_id = info_voltage();
    screen.writeln("VOLTAGE: " + voltage_id + "V","pink");
    std::string body;
    https_get("ipapi.co", "/json/", body);
    screen.writeln(body);

    screen.update();

       while(buttonmgr.update()==Action::None) {
        sleep_ms(10);
       }

    // restore main screen buttons here
    screen.clear();
    screen.draw_clock_time(
        get_time(std::stoi(read_key("CONFIG.TXT", "TIMEZONE"))),
        "white",
        CLOCK_SIZE,
        true
    );
    screen.default_buttonhints();
    return;
}

// boot up process
void bootup(){
    //booting process should be moved here
}

void draw_clock(int tz_offset) {
    static absolute_time_t last_activity = get_absolute_time();
    static absolute_time_t last_move = get_absolute_time();

    static bool screensaver_mode = false;

    static int clock_x = 0;
    static int clock_y = 30;

    static int dx = 2;
    static int dy = 1;

    static int last_drawn_second = -1;

    const int CLOCK_SIZE = 7;
    const int BUTTONHINT_TIMEOUT_SECONDS = 30;
    const int CLOCK_MOVE_INTERVAL_MS = 5000;

    const int screen_width = screen.get_width();
    const int screen_height = screen.get_height();

    const int size = CLOCK_SIZE;
    const int thickness = size;
    const int horizontal_length = size * 5;
    const int vertical_length = (horizontal_length * 7) / 4;

    const int digit_width =
        thickness * 2 +
        horizontal_length;

    const int digit_height =
        thickness * 3 +
        vertical_length * 2;

    const int digit_gap = size * 2;
    const int colon_width = size;
    const int colon_gap = size * 2;

    const int clock_width =
        digit_width * 4 +
        digit_gap * 2 +
        colon_gap * 2 +
        colon_width;

    const int clock_height = digit_height;

    absolute_time_t now = get_absolute_time();

    const int current_second =
        static_cast<int>(::time(nullptr) % 60);

    buttonmgr.wait_for_any_button();

    last_activity = now;

    /*
     * Sortida del mode screensaver.
     *
     * Netegem la pantalla, recuperem els button hints
     * i redibuixem immediatament tot el rellotge.
     */
    if (screensaver_mode) {
        screensaver_mode = false;

        screen.clear();
        screen.default_buttonhints();

        screen.draw_clock_time(
            get_time(tz_offset),
            "white",
            CLOCK_SIZE,
            true
        );

        last_drawn_second = current_second;

        return;
    }

    const int64_t inactive_ms =
        absolute_time_diff_us(last_activity, now) / 1000;

    /*
     * Entrada al mode screensaver.
     */
    if (!screensaver_mode &&
        inactive_ms >= BUTTONHINT_TIMEOUT_SECONDS * 1000) {

        screensaver_mode = true;

        screen.clear();

        clock_x = (screen_width - clock_width) / 2;
        clock_y = 30;

        last_move = now;

        screen.draw_clock_time(
            clock_x,
            clock_y,
            get_time(tz_offset),
            "white",
            CLOCK_SIZE,
            true
        );

        last_drawn_second = current_second;

        return;
    }

    /*
     * Mode screensaver.
     */
    if (screensaver_mode) {
        const int64_t move_elapsed_ms =
            absolute_time_diff_us(last_move, now) / 1000;

        bool clock_moved = false;

        if (move_elapsed_ms >= CLOCK_MOVE_INTERVAL_MS) {
            last_move = now;

            clock_x += dx;
            clock_y += dy;

            if (clock_x <= 0) {
                clock_x = 0;
                dx = -dx;
            }

            if (clock_x + clock_width >= screen_width) {
                clock_x = screen_width - clock_width;
                dx = -dx;
            }

            if (clock_y <= 0) {
                clock_y = 0;
                dy = -dy;
            }

            if (clock_y + clock_height >= screen_height) {
                clock_y = screen_height - clock_height;
                dy = -dy;
            }

            clock_moved = true;
        }

        /*
         * Si el rellotge s'ha mogut:
         * esborrem i redibuixem tots els dígits.
         */
        if (clock_moved) {
            screen.clear();

            screen.draw_clock_time(
                clock_x,
                clock_y,
                get_time(tz_offset),
                "white",
                CLOCK_SIZE,
                true
            );

            last_drawn_second = current_second;

            return;
        }

        /*
         * Si només ha canviat el segon,
         * draw_clock_time actualitzarà els dos punts.
         */
        if (current_second != last_drawn_second) {
            screen.draw_clock_time(
                clock_x,
                clock_y,
                get_time(tz_offset),
                "white",
                CLOCK_SIZE,
                false
            );

            last_drawn_second = current_second;
        }

        return;
    }

    /*
     * Mode normal.
     *
     * Només actualitzem una vegada per segon.
     */
    if (current_second != last_drawn_second) {
        screen.draw_clock_time(
            get_time(tz_offset),
            "white",
            CLOCK_SIZE,
            false
        );

        last_drawn_second = current_second;
    }
}



int main() {

    // initialize Pico
    stdio_init_all();

    // 1. show logo and app name
    screen.clear("dark blue",5);
    screen.draw_logo("PICO CLOCK");
    screen.show_boot_message("SCREEN INITIALIZED", "green");

    // 2 & 3. tinyFS file system 
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
    // and show message to configure it from computer
    if (file_exists("CONFIG.TXT") != FR_OK){
        screen.show_boot_message("CREATING CONFIG FILE", "orange");
        sleep_ms(3000);
        write_key("CONFIG.TXT", "WIFI_NAME", "WRITE WIFI NAME HERE");
        write_key("CONFIG.TXT", "WIFI_PASSWORD", "WRITE WIFI PASSWORD HERE");
        screen.show_boot_message("WIFI CONFIG NEEDED. ANY BUTTON TO CONTIUNE.");
        buttonmgr.wait_for_any_button();
    }
    // 4. filesystem initialized
    screen.show_boot_message("FILESYSTEM INITIALIZED");

    // 5 & 6 wifi and network initialization
    if(wifi_init()) {
        screen.show_boot_message("WIFI CHIPSET OK", "green");
        screen.show_boot_message("WIFI ARCH OK", "green");
    } else {
        screen.show_boot_message("ERROR INITIALIZING WIFI CHIPSET", "red");
        reboot();
    }

    // 7 & 8 connect to WiFi network
    screen.show_boot_message("CONNECTING TO WIFI NETWORK");
    if (wifi_connect(read_key("CONFIG.TXT", "WIFI_NAME"), read_key("CONFIG.TXT", "WIFI_PASSWORD"))){
        screen.show_boot_message("CONNECTED TO WIFI NETWORK", "green");
    } else {
        screen.show_boot_message("ERROR CONNECTING TO WIFI", "red");
        sleep_ms(3000);
        screen.show_boot_message("CHECK NETWORK CONFIG", "orange");
        buttonmgr.wait_for_any_button();
    }

    // 9 & 10 SNTP client 
    screen.show_boot_message("STARTING SNTP CLIENT");
    sntp_start();
    screen.show_boot_message("SNTP CLIENT STARTED", "green");


    // 11-12 sync time
    screen.show_boot_message("SYNCHRONIZING TIME");

    absolute_time_t start = get_absolute_time();

   // wait 10 seconds for the time to get acquired or show error
    while (!network_time_is_synced()) {
        if (absolute_time_diff_us(start, get_absolute_time()) >= 10 * 1000 * 1000) break;
    }

    if (network_time_is_synced()){
        screen.show_boot_message("TIME IS SYNCHRONIZED", "green");
    } else {
        screen.show_boot_message("TIME SYNC ERROR", "red");
        buttonmgr.wait_for_any_button();
    }

    // 13 read timezone
    if (read_key("CONFIG.TXT", "TIMEZONE").empty()){
        write_key("CONFIG.TXT", "TIMEZONE", "0");
        screen.show_boot_message("CONFIGURE TZ IN CONFIG FILE", "red");
        buttonmgr.wait_for_any_button();
    }
    int tz_offset = std::stoi(read_key("CONFIG.TXT", "TIMEZONE"));
    screen.show_boot_message("TZ LOADED", "green");

    std::string time_string;

    screen.clear("black",20);

    //draw initial buttonhints aka corners
    screen.default_buttonhints();

    // main loop
    while(true) {
        Action action = buttonmgr.update();

        switch (action) {
            case Action::UsbBoot:
                reset_usb_boot(0, 0);
                break;

            case Action::Reboot:
                reboot();
                break;

            case Action::ExposeDisk:
                expose_drive();
                break;

            case Action::ShowInfo:
                show_info();
                break;

            case Action::None:
                break;

            default:
                break;
    }

        draw_clock(tz_offset);
        led.blink_update();
        screen.update();
    }
}