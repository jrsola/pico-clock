#include "msc_disk.h"
#include "disk_config.h"
#include "ram_disk.h"

#define MSC_DISK_SIZE     DISK_SIZE_BYTES
#define MSC_SECTOR_SIZE   DISK_SECTOR_SIZE
#define MSC_SECTOR_COUNT  DISK_SECTOR_COUNT

static volatile bool ejected = false;

// checks if the pico is mounted in Windows
// it's te opposite of ejected (if it's not ejected it's in Windows)
bool pico_mounted() {
    return !ejected;
}

void usb_msc_init()
{
    ejected = false;
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

int32_t tud_msc_read10_cb(uint8_t lun,
                          uint32_t lba,
                          uint32_t offset,
                          void* buffer,
                          uint32_t bufsize) {
    (void) lun;

    uint32_t start = lba * MSC_SECTOR_SIZE + offset;

    if (start + bufsize > MSC_DISK_SIZE) {
        return -1;
    }

    memcpy(buffer, ram_disk + start, bufsize);

    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun,
                           uint32_t lba,
                           uint32_t offset,
                           uint8_t* buffer,
                           uint32_t bufsize) {
    (void) lun;

    uint32_t start = lba * MSC_SECTOR_SIZE + offset;

    if (start + bufsize > MSC_DISK_SIZE) {
        return -1;
    }

    memcpy(ram_disk + start, buffer, bufsize);

    ram_disk_mark_dirty();

    return (int32_t)bufsize;
}

void tud_msc_write10_complete_cb(uint8_t lun) {
    (void) lun;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return true;
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    return true;
}

bool tud_msc_start_stop_cb(uint8_t lun,
                           uint8_t power_condition,
                           bool start,
                           bool load_eject) {
    (void) lun;
    (void) power_condition;

    if (load_eject && !start) {
        ram_disk_flush_to_flash();
        ejected = true;
    }

    return true;
}

void tud_msc_capacity_cb(uint8_t lun,
                         uint32_t* block_count,
                         uint16_t* block_size) {
    (void) lun;

    *block_count = MSC_SECTOR_COUNT;
    *block_size  = MSC_SECTOR_SIZE;
}

void tud_msc_inquiry_cb(uint8_t lun,
                        uint8_t vendor_id[8],
                        uint8_t product_id[16],
                        uint8_t product_rev[4]) {
    (void) lun;

    const uint8_t vid[8] = {
        'R','P','I','-','P','I','C','O'
    };

    const uint8_t pid[16] = {
        'U','S','B',' ','D','I','S','K',
        ' ',' ',' ',' ',' ',' ',' ',' '
    };

    const uint8_t rev[4] = {
        '1','.','0','0'
    };

    memcpy(vendor_id, vid, 8);
    memcpy(product_id, pid, 16);
    memcpy(product_rev, rev, 4);
}

int32_t tud_msc_scsi_cb(uint8_t lun,
                        uint8_t const scsi_cmd[16],
                        void* buffer,
                        uint16_t bufsize) {
    (void) lun;
    (void) buffer;
    (void) bufsize;

    switch (scsi_cmd[0]) {
        case 0x1B: { // START STOP UNIT
            bool start = scsi_cmd[4] & 0x01;
            bool load_eject = scsi_cmd[4] & 0x02;

            if (load_eject && !start) {
                ram_disk_flush_to_flash();
                ejected = true;
            }

            return 0;
        }

        case 0x35: // SYNCHRONIZE CACHE
            ram_disk_flush_to_flash();
            return 0;

        default:
            return 0;
    }
}

} // extern "C"