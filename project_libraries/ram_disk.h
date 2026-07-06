#pragma once

#include <cstdint>
#include <cstring>

#include "hardware/flash.h"
#include "pico/sync.h"
#include "hardware/sync.h"

#include "disk_config.h"

extern uint8_t ram_disk[DISK_SIZE_BYTES];

void ram_disk_format_fat12();
void ram_disk_load_from_flash();
void ram_disk_flush_to_flash();
bool ram_disk_is_dirty();
void ram_disk_mark_dirty();
void ram_disk_clear_dirty();
