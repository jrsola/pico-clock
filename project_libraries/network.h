#pragma once
#include <string>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/http_client.h"
#include "lwip/altcp_tls.h"

static volatile bool time_synced = false;

struct HttpRequest {
    std::string body;
    bool finished = false;
    bool success = false;
};

bool wifi_init();
bool wifi_connect(const std::string& ssid, const std::string& password, uint32_t timeout_ms = 30000);
void sntp_start(std::string sntp_server = "pool.ntp.org");
bool network_time_is_synced();
bool https_get(const std::string& host, const std::string& path, std::string& body);