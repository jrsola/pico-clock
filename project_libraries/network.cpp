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

static err_t http_recv_callback(
    void* arg,
    struct altcp_pcb* pcb,
    struct pbuf* p,
    err_t err
) {
    HttpRequest* request = static_cast<HttpRequest*>(arg);

    if (err != ERR_OK) {
        if (p != nullptr) {
            pbuf_free(p);
        }

        return err;
    }

    if (p == nullptr) {
        return ERR_OK;
    }

    for (struct pbuf* q = p; q != nullptr; q = q->next) {
        request->body.append(
            static_cast<const char*>(q->payload),
            q->len
        );
    }

    altcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

static void http_result_callback(
    void* arg,
    httpc_result_t httpc_result,
    u32_t rx_content_len,
    u32_t srv_res,
    err_t err
) {
    HttpRequest* request = static_cast<HttpRequest*>(arg);

    request->success =
        httpc_result == HTTPC_RESULT_OK &&
        srv_res == 200 &&
        err == ERR_OK;

    request->finished = true;
}

bool https_get(
    const std::string& host,
    const std::string& path,
    std::string& body
) {
    HttpRequest request;

    struct altcp_tls_config* tls_config =
        altcp_tls_create_config_client(nullptr, 0);

    if (tls_config == nullptr) {
        return false;
    }

    altcp_allocator_t tls_allocator = {
        .alloc = altcp_tls_alloc,
        .arg = tls_config
    };

    httpc_connection_t settings = {};
    settings.altcp_allocator = &tls_allocator;
    settings.result_fn = http_result_callback;
    settings.headers_done_fn = nullptr;

    cyw43_arch_lwip_begin();

    err_t err = httpc_get_file_dns(
        host.c_str(),
        443,
        path.c_str(),
        &settings,
        http_recv_callback,
        &request,
        nullptr
    );

    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        altcp_tls_free_config(tls_config);
        return false;
    }

    absolute_time_t timeout =
        make_timeout_time_ms(10000);

    while (!request.finished) {
        if (absolute_time_diff_us(
                get_absolute_time(),
                timeout) <= 0) {

            altcp_tls_free_config(tls_config);
            return false;
        }

        sleep_ms(10);
    }

    altcp_tls_free_config(tls_config);

    if (!request.success) {
        return false;
    }

    body = request.body;

    return true;
}