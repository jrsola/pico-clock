#include "msc_disk.h"

#define MSC_FLASH_OFFSET  (PICO_FLASH_SIZE_BYTES - 128 * 1024)
#define MSC_FLASH_SIZE    (128 * 1024)
#define MSC_SECTOR_SIZE   512
#define MSC_SECTOR_COUNT  (MSC_FLASH_SIZE / MSC_SECTOR_SIZE)

extern const uint8_t _binary_fat12_disk_start[]; // Linker symbols

static bool ejected = false;

void usb_msc_init() {
    tusb_init();
}

// Called when the device is mounted by the host
void tud_mount_cb() {
    ejected = false;
}

// Called when the device is unmounted by the host
void tud_umount_cb() {
    ejected = true;
}

extern "C" {

// READ10 Callback
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void* buffer, uint32_t bufsize) {
    const uint8_t* flash = (const uint8_t*)(XIP_BASE + MSC_FLASH_OFFSET + lba * MSC_SECTOR_SIZE + offset);
    memcpy(buffer, flash, bufsize);
    return bufsize;
}

// WRITE10 Callback
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t* buffer, uint32_t bufsize) {
    uint32_t flash_offset = MSC_FLASH_OFFSET + lba * MSC_SECTOR_SIZE + offset;
    flash_offset &= ~0xFFF;

    if (flash_offset + bufsize > PICO_FLASH_SIZE_BYTES) return -1;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offset, 4096);  // align to 4K sectors
    flash_range_program(flash_offset, buffer, bufsize);
    restore_interrupts(ints);

    return bufsize;
}

// SCSI Callback
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                        void* buffer, uint16_t bufsize) {
    uint16_t reply_len = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_INQUIRY:
            if (bufsize < 36) return -1;
            memcpy(buffer,
                   (uint8_t[36]){
                       0x00, 0x80, 0x00, 0x01, 36 - 5, 0, 0, 0,
                       'R','P','I','-','P','I','C','O',
                       'U','S','B',' ','D','I','S','K',
                       '1','.','0','0'
                   },
                   36);
            reply_len = 36;
            break;

        case SCSI_CMD_READ_CAPACITY_10:
            if (bufsize < 8) return -1;
            ((uint8_t*)buffer)[0] = ((MSC_SECTOR_COUNT - 1) >> 24) & 0xFF;
            ((uint8_t*)buffer)[1] = ((MSC_SECTOR_COUNT - 1) >> 16) & 0xFF;
            ((uint8_t*)buffer)[2] = ((MSC_SECTOR_COUNT - 1) >> 8) & 0xFF;
            ((uint8_t*)buffer)[3] = ((MSC_SECTOR_COUNT - 1) >> 0) & 0xFF;
            ((uint8_t*)buffer)[4] = (MSC_SECTOR_SIZE >> 24) & 0xFF;
            ((uint8_t*)buffer)[5] = (MSC_SECTOR_SIZE >> 16) & 0xFF;
            ((uint8_t*)buffer)[6] = (MSC_SECTOR_SIZE >> 8) & 0xFF;
            ((uint8_t*)buffer)[7] = (MSC_SECTOR_SIZE >> 0) & 0xFF;
            reply_len = 8;
            break;

        case SCSI_CMD_TEST_UNIT_READY:
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            reply_len = 0;
            break;

        default:
            return -1; // error
    }

    return reply_len;
}

// Capacitat: retorna nombre de sectors i mida de sector
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    *block_count = MSC_SECTOR_COUNT;  // 128 KB / 512
    *block_size  = MSC_SECTOR_SIZE;   // 512 bytes
}

// Informació del dispositiu USB (marca, model, versió)
void tud_msc_inquiry_cb(uint8_t lun,
                        uint8_t vendor_id[8],
                        uint8_t product_id[16],
                        uint8_t product_rev[4]) {
    const char vid[] = "RPI-PICO";
    const char pid[] = "USB DISK";
    const char rev[] = "1.0";

    memcpy(vendor_id,  vid, sizeof(vendor_id[0]) * 8);
    memcpy(product_id, pid, sizeof(product_id[0]) * 16);
    memcpy(product_rev, rev, sizeof(product_rev[0]) * 4);
}

// Indica si el disc està llest per operar
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return true;  // Sempre disponible
}


} // extern "C"
