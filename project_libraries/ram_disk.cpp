#include "ram_disk.h"

uint8_t ram_disk[DISK_SIZE_BYTES];

static volatile bool dirty = false;

void ram_disk_mark_dirty(){
   dirty= true;
}

void ram_disk_clear_dirty(){
   dirty = false;
}

void ram_disk_load_from_flash(){
   const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + DISK_FLASH_OFFSET);
   memcpy(ram_disk, flash_ptr, DISK_SIZE_BYTES);
   dirty = false;
}

void ram_disk_flush_to_flash(){
   if (!dirty) return;

   uint32_t ints = save_and_disable_interrupts();

   flash_range_erase(DISK_FLASH_OFFSET, DISK_SIZE_BYTES);
   flash_range_program(DISK_FLASH_OFFSET, ram_disk, DISK_SIZE_BYTES);

   restore_interrupts(ints);

   dirty = false;
}

static void put_u16_le(uint8_t* p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

static void put_u32_le(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}

void ram_disk_format_fat12() {
    memset(ram_disk, 0x00, DISK_SIZE_BYTES);

    uint8_t* bs = ram_disk;

    // Jump instruction + OEM name
    bs[0] = 0xEB;
    bs[1] = 0x3C;
    bs[2] = 0x90;

    memcpy(bs + 3, "MSDOS5.0", 8);

    // BIOS Parameter Block
    put_u16_le(bs + 11, DISK_SECTOR_SIZE);       // Bytes per sector = 512
    bs[13] = 1;                                  // Sectors per cluster
    put_u16_le(bs + 14, 1);                      // Reserved sectors
    bs[16] = 2;                                  // Number of FATs
    put_u16_le(bs + 17, 32);                     // Root directory entries
    put_u16_le(bs + 19, DISK_SECTOR_COUNT);      // Total sectors, 16-bit
    bs[21] = 0xF8;                               // Media descriptor
    put_u16_le(bs + 22, 1);                      // Sectors per FAT
    put_u16_le(bs + 24, 1);                      // Sectors per track
    put_u16_le(bs + 26, 1);                      // Number of heads
    put_u32_le(bs + 28, 0);                      // Hidden sectors
    put_u32_le(bs + 32, 0);                      // Total sectors, 32-bit unused

    // Extended boot record
    bs[36] = 0x80;                               // Drive number
    bs[37] = 0x00;
    bs[38] = 0x29;                               // Extended boot signature
    put_u32_le(bs + 39, 0x12345678);             // Volume ID

    memcpy(bs + 43, "PICO DISK  ", 11);          // Volume label, exactly 11 chars
    memcpy(bs + 54, "FAT12   ", 8);              // Filesystem type

    // Boot sector signature
    bs[510] = 0x55;
    bs[511] = 0xAA;

    // FAT #1 starts at sector 1
    uint8_t* fat1 = ram_disk + (1 * DISK_SECTOR_SIZE);

    fat1[0] = 0xF8;
    fat1[1] = 0xFF;
    fat1[2] = 0xFF;

    // FAT #2 starts at sector 2
    uint8_t* fat2 = ram_disk + (2 * DISK_SECTOR_SIZE);

    fat2[0] = 0xF8;
    fat2[1] = 0xFF;
    fat2[2] = 0xFF;

    ram_disk_mark_dirty();
}