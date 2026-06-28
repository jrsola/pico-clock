#include <cstring>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/sync.h"
#include "ff.h"
#include "diskio.h"

#define FLASH_SIZE_BYTES    (2 * 1024 * 1024) // 2MB FLASH IN PICO
#define DISK_SIZE_BYTES     (128 * 1024) // IF DISK SIZE CHANGES, 128 SHOULD CHANGE TOO
#define DISK_SECTOR_SIZE    512
#define DISK_SECTOR_COUNT   (DISK_SIZE_BYTES / DISK_SECTOR_SIZE)
#define DISK_FLASH_OFFSET   (FLASH_SIZE_BYTES - DISK_SIZE_BYTES) // This is where the disk starts  

DSTATUS disk_initialize(BYTE pdrv) {
    return (pdrv == 0) ? RES_OK : STA_NOINIT; 
}

DSTATUS disk_status(BYTE pdrv) {
    return (pdrv == 0) ? RES_OK : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || sector + count > DISK_SECTOR_COUNT)
        return RES_PARERR;

    const uint8_t* flash_base = (const uint8_t*)(XIP_BASE + DISK_FLASH_OFFSET + sector * DISK_SECTOR_SIZE);
    memcpy(buff, flash_base, count * DISK_SECTOR_SIZE);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || sector + count > DISK_SECTOR_COUNT)
        return RES_PARERR;

    uint32_t offset = DISK_FLASH_OFFSET + sector * DISK_SECTOR_SIZE;
    uint32_t size = count * DISK_SECTOR_SIZE;

    // Flash must be erased in 4KB sectors
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(offset, (size + 4095) / 4096 * 4096);
    flash_range_program(offset, buff, size);
    restore_interrupts(ints);

    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:        return RES_OK;
        case GET_SECTOR_SIZE:  *(WORD*)buff = DISK_SECTOR_SIZE; return RES_OK;
        case GET_BLOCK_SIZE:   *(DWORD*)buff = 1; return RES_OK; // Flash block = sector
        case GET_SECTOR_COUNT: *(DWORD*)buff = DISK_SECTOR_COUNT; return RES_OK;
    }
    return RES_PARERR;
}
