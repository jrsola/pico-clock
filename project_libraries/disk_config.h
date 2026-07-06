#define FLASH_SIZE_BYTES    (2 * 1024 * 1024)
// first parameter is the KB for the drive
#define DISK_SIZE_BYTES     (64 * 1024)
#define DISK_SECTOR_SIZE    512
#define DISK_SECTOR_COUNT   (DISK_SIZE_BYTES / DISK_SECTOR_SIZE)
#define DISK_FLASH_OFFSET   (FLASH_SIZE_BYTES - DISK_SIZE_BYTES)
