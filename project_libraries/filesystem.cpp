#include <cstring>
#include <cstdio>
#include "filesystem.h"
#include "my_screen.h"

static bool fs_mounted = false;
static myScreen* debug_screen = nullptr;

void filesystem_screen(myScreen* screen){
    debug_screen = screen;
}

static void fs_debug(const char* text, const char* color = "") {
    if (debug_screen) {
        debug_screen->writeln(text, color);
    }
}

bool mountfs() {
    if (fs_mounted) {
        fs_debug("already mounted", "green");
        return true;
    }

    FRESULT res = f_mount(&fs, "", 1);

    if (res == FR_OK) {
        fs_mounted = true;
        fs_debug("mountfs OK", "green");
        return true;
    }

    fs_debug("mountfs failed", "red");

    if (res == FR_NO_FILESYSTEM) {
        fs_debug("no FAT volume", "yellow");
    } else if (res == FR_DISK_ERR) {
        fs_debug("disk err mount", "red");
    } else if (res == FR_NOT_READY) {
        fs_debug("not ready", "red");
    }

    return false;
}

bool unmountfs() {
    FRESULT res = f_mount(nullptr, "", 0);

    if (res == FR_OK) {
        fs_mounted = false;
        return true;
    }

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
    fs_debug("WRITE START", "white");

    if (!mountfs()) {
        fs_debug("WRITE: MOUNT FAIL", "red");
        return false;
    }

    fs_debug("WRITE: MOUNT OK", "green");

    FIL file;
    FRESULT res = f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS);

    if (res != FR_OK) {
        fs_debug("WRITE: OPEN FAIL", "red");

        if (res == FR_INVALID_NAME) {
            fs_debug("ERR INVALID NAME", "red");
        } else if (res == FR_DENIED) {
            fs_debug("ERR DENIED", "red");
        } else if (res == FR_DISK_ERR) {
            fs_debug("ERR DISK", "red");
        } else if (res == FR_NOT_READY) {
            fs_debug("ERR NOT READY", "red");
        } else if (res == FR_NO_FILESYSTEM) {
            fs_debug("ERR NO FS", "red");
        } else if (res == FR_NOT_ENABLED) {
            fs_debug("ERR NOT ENABLED", "red");
        } else {
            fs_debug("ERR OTHER OPEN", "red");
        }

        return false;
    }

    fs_debug("WRITE: OPEN OK", "green");

    UINT bytes_written = 0;
    UINT bytes_to_write = strlen(content);

    res = f_write(&file, content, bytes_to_write, &bytes_written);

    if (res != FR_OK) {
        fs_debug("WRITE: WRITE FAIL", "red");

        if (res == FR_DISK_ERR) {
            fs_debug("ERR DISK WRITE", "red");
        } else if (res == FR_DENIED) {
            fs_debug("ERR DENIED WRITE", "red");
        } else {
            fs_debug("ERR OTHER WRITE", "red");
        }

        f_close(&file);
        return false;
    }

    if (bytes_written != bytes_to_write) {
        fs_debug("WRITE: SIZE MISMATCH", "red");
        f_close(&file);
        return false;
    }

    fs_debug("WRITE: WRITE OK", "green");

    res = f_sync(&file);

    if (res != FR_OK) {
        fs_debug("WRITE: SYNC FAIL", "red");
        f_close(&file);
        return false;
    }

    fs_debug("WRITE: SYNC OK", "green");

    res = f_close(&file);

    if (res != FR_OK) {
        fs_debug("WRITE: CLOSE FAIL", "red");
        return false;
    }

    fs_debug("WRITE: CLOSE OK", "green");
    fs_debug("WRITE OK", "green");

    return true;
}