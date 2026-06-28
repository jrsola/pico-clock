#include "filesystem.h"

static bool fs_mounted = false;

bool mountfs() {
    if (fs_mounted) 
        return true;

    FRESULT res = f_mount(&fs, "", 1);
    if (res == FR_OK) 
        fs_mounted = true;

    return fs_mounted;
}

bool read_file(const char* path, char* buffer, size_t bufsize) {
    FIL file;
    FRESULT res = f_open(&file, path, FA_READ);
    if (res != FR_OK) {
        return false;
    }

    UINT bytes_read;
    res = f_read(&file, buffer, bufsize - 1, &bytes_read);
    if (res != FR_OK) {
        f_close(&file);
        return false;
    }

    buffer[bytes_read] = '\0'; // Null-terminate
    f_close(&file);
    return true;
}

bool write_file(const char* path, const char* content) {
    FIL file;
    FRESULT res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        return false;
    }

    UINT bytes_written;
    res = f_write(&file, content, strlen(content), &bytes_written);
    if (res != FR_OK || bytes_written != strlen(content)) {
        f_close(&file);
        return false;
    }

    f_close(&file);
    return true;
} 