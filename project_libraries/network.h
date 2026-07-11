#pragma once
#include <string>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"

static volatile bool time_synced = false;

bool wifi_init();
bool wifi_connect(const std::string& ssid, const std::string& password, uint32_t timeout_ms = 30000);
void sntp_start(std::string sntp_server = "pool.ntp.org");
bool sync_time(const char* sntp_server, uint32_t timeout_ms = 10000);
bool network_time_is_synced();