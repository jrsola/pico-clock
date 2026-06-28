#include "tusb.h"

#define EPNUM_MSC_OUT  0x01
#define EPNUM_MSC_IN   0x81

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = 0x4000,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 1
};

extern "C" uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device;
}

// Configuration descriptor
#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power (mA)
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Interface number, string index, EP OUT & IN address, EP size
    TUD_MSC_DESCRIPTOR(0, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)
};

extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t) {
    return desc_configuration;
}

// String descriptors

char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },     // 0: English (0x0409)
    "Raspberry Pi",                    // 1: Manufacturer
    "Pico FAT Drive",                  // 2: Product
    "123456",                          // 3: Serial number
};

static uint16_t desc_string[32];

extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        desc_string[1] = 0x0409; // English
        chr_count = 1;
    } else {
        const char* str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; ++i) {
            desc_string[1 + i] = str[i];
        }
    }

    desc_string[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return desc_string;
}
