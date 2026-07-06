#include <cstring>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "pico/sync.h"
#include "ff.h"
#include "diskio.h"
#include "disk_config.h"
#include "ram_disk.h"

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

    memcpy(
        buff,
        ram_disk + sector * DISK_SECTOR_SIZE,
        count * DISK_SECTOR_SIZE
    );

    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || sector + count > DISK_SECTOR_COUNT) {
        return RES_PARERR;
    }

    memcpy(
        ram_disk + sector * DISK_SECTOR_SIZE,
        buff,
        count * DISK_SECTOR_SIZE
    );

    ram_disk_mark_dirty();

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
            *(DWORD*)buff = 1;
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD*)buff = DISK_SECTOR_COUNT;
            return RES_OK;
    }

    return RES_PARERR;
}