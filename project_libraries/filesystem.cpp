#include <cstring>
#include <cstdio>
#include "filesystem.h"

static bool fs_mounted = false;

bool mountfs()
{
    if (fs_mounted) {
        return true;
    }

    // Try to mount the existing filesystem
    FRESULT res = f_mount(&fs, "0:", 1);
    
    if (res == FR_OK){
        fs_mounted = true;
        return true;
    }

    // there is no filesystem
    return false;
}

bool format_disk(){
    
    BYTE work[512];

    MKFS_PARM opt ={
        .fmt = FM_FAT,
        .n_fat = 1,
        .align = 0,
        .n_root = 64,
        .au_size = 512
    };

    FRESULT res = f_mkfs("0:", &opt, work, sizeof(work));

    if (res != FR_OK){
        return false;
    }
    fs_mounted = false;
    return true;
}

// checks if a file exists 
// returns true if the file exists and false otherwise
bool file_exists(const char* path){
    if (!mountfs()) return false;
    
    FILINFO finfo;
    FRESULT res = f_stat(path, &finfo);

    return res == FR_OK;
}


bool read_file(const char *path, char *buffer, size_t bufsize)
{
    if (!mountfs()) {
        return false;
    }
    
    FIL file;
    FRESULT res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        return false;
    }

    UINT bytes_read = 0;
    res = f_read(&file, buffer, bufsize - 1, &bytes_read);
    if (res != FR_OK)
    {
        f_close(&file);
        return false;
    }

    buffer[bytes_read] = '\0'; // Null-terminate
    f_close(&file);
    return true;
}

bool write_file(const char *path, const char *content)
{
    if (!mountfs()) {
        return false;
    }
    
    FIL file;
    FRESULT res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);
    
    if (res != FR_OK)
    {
        return false;
    }

    UINT bytes_written = 0;
    UINT bytes_to_write = strlen(content);

    res = f_write(&file, content, bytes_to_write, &bytes_written);

    if (res != FR_OK || bytes_written != bytes_to_write) {
        f_close(&file);
        return false;
    }

    res = f_sync(&file);

    if (res != FR_OK)
    {
        f_close(&file);
        return false;
    }

    f_close(&file);
    return true;
}