#include "usb_msc.h"

#define MSC_FLASH_SIZE         (128 * 1024)                 // 128 KB
#define MSC_FLASH_OFFSET       (PICO_FLASH_SIZE_BYTES - MSC_FLASH_SIZE)
#define MSC_SECTOR_SIZE        512
#define MSC_SECTOR_COUNT       (MSC_FLASH_SIZE / MSC_SECTOR_SIZE)

static uint8_t msc_write_buffer[MSC_SECTOR_SIZE];

// Call this at startup
void usb_msc_init() {
    tusb_init();
}

// TinyUSB callback: return inquiry response
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    memcpy(vendor_id,  "RP2040  ", 8);
    memcpy(product_id, "USB FlashDisk  ", 16);
    memcpy(product_rev, "1.0", 4);
}

// Read-only for now
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    *block_count = MSC_SECTOR_COUNT;
    *block_size  = MSC_SECTOR_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + MSC_FLASH_OFFSET + lba * MSC_SECTOR_SIZE + offset);
    memcpy(buffer, flash_ptr, bufsize);
    return bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    // TinyUSB may call multiple times with offset != 0
    memcpy(msc_write_buffer + offset, buffer, bufsize);
    return bufsize;
}

void tud_msc_write10_complete_cb(uint8_t lun) {
    static uint32_t last_lba = (uint32_t)-1;

    if (last_lba != (uint32_t)-1) {
        uint32_t flash_offset = MSC_FLASH_OFFSET + last_lba * MSC_SECTOR_SIZE;

        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(flash_offset, FLASH_SECTOR_SIZE);
        flash_range_program(flash_offset, msc_write_buffer, FLASH_SECTOR_SIZE);
        restore_interrupts(ints);
    }

    last_lba = (uint32_t)-1;  // Clear after write
}

bool tud_msc_write10_ready_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint32_t size) {
    // Store LBA for completion
    if (offset == 0) last_lba = lba;
    return true;
}

void tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (load_eject) {
        // Optional: persist state or cleanup
    }
}
