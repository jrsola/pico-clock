#include <cstring>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/sync.h"
#include "ff.h"
#include "diskio.h"

#define FLASH_SIZE_BYTES    (2 * 1024 * 1024)
#define DISK_SIZE_BYTES     (128 * 1024)
#define DISK_SECTOR_SIZE    512
#define DISK_SECTOR_COUNT   (DISK_SIZE_BYTES / DISK_SECTOR_SIZE)
#define DISK_FLASH_OFFSET   (FLASH_SIZE_BYTES - DISK_SIZE_BYTES)

DSTATUS disk_initialize(BYTE pdrv) {
    return (pdrv == 0) ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
    return (pdrv == 0) ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || sector + count > DISK_SECTOR_COUNT) {
        return RES_PARERR;
    }

    const uint8_t* flash_base =
        (const uint8_t*)(XIP_BASE + DISK_FLASH_OFFSET + sector * DISK_SECTOR_SIZE);

    memcpy(buff, flash_base, count * DISK_SECTOR_SIZE);

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || sector + count > DISK_SECTOR_COUNT) {
        return RES_PARERR;
    }

    uint8_t block_buffer[FLASH_SECTOR_SIZE];

    uint32_t bytes_remaining = count * DISK_SECTOR_SIZE;
    uint32_t src_offset = 0;
    uint32_t write_addr = DISK_FLASH_OFFSET + sector * DISK_SECTOR_SIZE;

    while (bytes_remaining > 0) {
        uint32_t block_start = write_addr & ~(FLASH_SECTOR_SIZE - 1);
        uint32_t offset_in_block = write_addr - block_start;

        uint32_t bytes_this_block = FLASH_SECTOR_SIZE - offset_in_block;

        if (bytes_this_block > bytes_remaining) {
            bytes_this_block = bytes_remaining;
        }

        const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + block_start);

        memcpy(block_buffer, flash_ptr, FLASH_SECTOR_SIZE);

        memcpy(block_buffer + offset_in_block,
               buff + src_offset,
               bytes_this_block);

        uint32_t ints = save_and_disable_interrupts();

        flash_range_erase(block_start, FLASH_SECTOR_SIZE);
        flash_range_program(block_start, block_buffer, FLASH_SECTOR_SIZE);

        restore_interrupts(ints);

        write_addr += bytes_this_block;
        src_offset += bytes_this_block;
        bytes_remaining -= bytes_this_block;
    }

    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0) {
        return RES_PARERR;
    }

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = DISK_SECTOR_SIZE;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = FLASH_SECTOR_SIZE / DISK_SECTOR_SIZE;
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD*)buff = DISK_SECTOR_COUNT;
            return RES_OK;
    }

    return RES_PARERR;
}