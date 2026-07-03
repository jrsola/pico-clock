#include <cstring>
#include <cstdio>
#include "filesystem.h"
#include "disk_config.h"
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

FRESULT mountfs() {
    // if already mounted, just return OK
    if (fs_mounted) return FR_OK;

    // mount the filesystem
    FRESULT res = f_mount(&fs, "", 1);

    // if it went well, make mounted true
    if (res == FR_OK) fs_mounted = true;

    return res;
}

FRESULT unmountfs() {
    // if already unmounted, just return OK
    if(!fs_mounted) return FR_OK;

    // unmount the filesystem
    FRESULT res = f_mount(nullptr, "", 0);

    // if it went well, make mounted false
    if (res == FR_OK) fs_mounted = false;

    return res;
}

// formats the disk created in diskio.cpp
FRESULT format_disk(){
    
    BYTE work[DISK_SECTOR_SIZE];

    MKFS_PARM opt ={
        .fmt = FM_FAT,
        .n_fat = 1,
        .align = 0,
        .n_root = 64,
        .au_size = DISK_SECTOR_SIZE
    };

    FRESULT res = f_mkfs("0:", &opt, work, sizeof(work));

    // if the operation was succesful, flag fs as not mounted (yet)
    if (res == FR_OK) fs_mounted = false;
 
    return res;
}

// checks if a file exists 
// returns FR_OK if the file exists and FR_NO_FILE otherwise
FRESULT file_exists(const char* path){
    // try to mount the filesystem first or report error
    if (mountfs() != FR_OK) return res;
    
    FILINFO finfo;
    return f_stat(path, &finfo);
}

// looks for a key in the configuration file and returns its value
// if the key can't be found it returns the value "key_not_found"
std::string read_config(const std::string& filename, const std::string& key) {
    // if we did not pass a filename or key, return an error
    if (filename.empty() || key.empty()) return "invalid_parameters";

    // if config file does not exist, report error
    if (file_exists(filename) != FR_OK) return "config_file_missing";

    // config file exists and key is not empty, proceed
    FIL file;
    res = f_open(&file, filename.c_str(), FA_READ);

    if (res != FR_OK) return "error_opening_file";

    FSIZE_t file_size = f_size(&file);

    if (file_size == 0) {
        f_close(&file);
        return "key_not_found";
    }

    std::string content;
    content.resize(file_size);

    UINT bytes_read = 0;
    res = f_read(&file, content.data(), file_size, &bytes_read);

    f_close(&file);

    if (res != FR_OK) {
        return "key_not_found";
    }

    content.resize(bytes_read);

    // Normalitzem salts de línia: eliminem '\r'
    std::string normalized;
    normalized.reserve(content.size());

    for (char c : content) {
        if (c != '\r') {
            normalized.push_back(c);
        }
    }

    std::string wanted_key = "[" + key + "]";

    size_t pos = 0;

    while (pos < normalized.size()) {
        size_t line_end = normalized.find('\n', pos);

        if (line_end == std::string::npos) {
            line_end = normalized.size();
        }

        std::string line = normalized.substr(pos, line_end - pos);

        if (line == wanted_key) {
            // Hem trobat [CLAU].
            // Ara busquem la primera línia no buida després de la clau.
            pos = line_end + 1;

            while (pos < normalized.size()) {
                size_t value_end = normalized.find('\n', pos);

                if (value_end == std::string::npos) {
                    value_end = normalized.size();
                }

                std::string value = normalized.substr(pos, value_end - pos);

                if (!value.empty()) {
                    return value;
                }

                pos = value_end + 1;
            }

            return "key_not_found";
        }

        pos = line_end + 1;
    }

    return "key_not_found";
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

bool write_file(const char *path, const char *content) {
    if (!mountfs()) return false;

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

