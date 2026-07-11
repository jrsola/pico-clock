#include "network.h"

bool wifi_init(){
    if (cyw43_arch_init() != 0) return false;
    cyw43_arch_enable_sta_mode();
    return true;
}

bool wifi_connect(const std::string& ssid, const std::string& password, uint32_t timeout_ms){
    for (int attempt = 0; attempt < 3; attempt++){
        int result = cyw43_arch_wifi_connect_timeout_ms(ssid.c_str(), password.c_str(), CYW43_AUTH_WPA2_AES_PSK, timeout_ms);
        if (result == 0) return true;
        sleep_ms(1000);
    }
    return false;
}

void sntp_callback(time_t sec, suseconds_t us) {
    struct timeval tv = {sec, us};
    settimeofday(&tv, nullptr);

    time_synced = true;
}

void sntp_start(std::string sntp_server) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL); // Poll mode for periodic updates
    sntp_setservername(0, sntp_server.c_str());   // Set NTP server
    sntp_init();  // Start SNTP service
}

bool network_time_is_synced() {
    return time_synced;
}
