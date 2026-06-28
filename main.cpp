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



int main()
{
   // Initialize Pico
   stdio_init_all();
    
   // Initialize screen 
   //init_screen();

}