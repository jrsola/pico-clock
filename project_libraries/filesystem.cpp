#include <cstring>
#include <cstdio>
#include "filesystem.h"
#include "disk_config.h"
#include "my_screen.h"
#include "ram_disk.h"

static bool fs_mounted = false;
static myScreen* debug_screen = nullptr;

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

    f_mount(nullptr, "", 0);
    fs_mounted = false;
    
    BYTE work[DISK_SECTOR_SIZE];

    MKFS_PARM opt ={
        .fmt = FM_FAT,
        .n_fat = 1,
        .align = 0,
        .n_root = 32,
        .au_size = DISK_SECTOR_SIZE
    };

    FRESULT res = f_mkfs("", &opt, work, sizeof(work));

    // if the operation was succesful, flag fs as not mounted (yet)
    if (res == FR_OK) {
        ram_disk_mark_dirty();
    }
    
    fs_mounted = false;
    
    return res;
}

// checks if a file exists 
// returns FR_OK if the file exists and FR_NO_FILE otherwise
FRESULT file_exists(const char* path){
    // try to mount the filesystem first or report error
    if (!fs_mounted) return FR_NO_FILE;
    
    FILINFO finfo;
    return f_stat(path, &finfo);
}


// looks for a key in the configuration file and returns its value
// if the key can't be found it returns the value "key_not_found"
std::string read_config(const std::string& filename, const std::string& key) {

    // if we did not pass a filename or key, return an empty string
    if (filename.empty() || key.empty()) return "";

    // if config file does not exist, return empty string
    if (file_exists(filename.c_str()) != FR_OK) return "";

    // file exists
    FIL file;
    res = f_open(&file, filename.c_str(), FA_READ);

    // file exists, but it's empty, return empty string 
    if (f_size(&file) == 0) {
        f_close(&file);
        return "";
    }

    // read file contents in a string, resized to match the filesize.
    std::string content;
    content.resize(f_size(&file));

    UINT bytes_read = 0;
    FRESULT res = f_read(&file, content.data(), f_size(&file), &bytes_read);
    f_close(&file);

    content.resize(bytes_read);

    std::string lookup_key = "[" + key + "]";
    size_t key_pos = content.find(lookup_key);

    // npos = no position = lookup key not found, return empty string
    if (key_pos == std::string::npos) return "";

    // place pointer just after [key] + \r\n (2)
    size_t value_start = key_pos + lookup_key.length() + 2;
    // find until next \r\n
    size_t value_end = content.find("\r\n", value_start);

    // if a final \r\n can't be found, assume it's until the end of the file
    if (value_end == std::string::npos) value_end = content.size();

    return content.substr(value_start, value_end - value_start);
}

FRESULT write_config(const std::string& filename, const std::string& key, const std::string& value) {

    if (filename.empty() || key.empty()) return FR_INVALID_NAME;

    if (file_exists(filename.c_str()) != FR_OK) return FR_NO_FILE;

    // config file exists
    FIL file;
    FRESULT res = f_open(&file, filename.c_str(), FA_READ);
    
    if (res != FR_OK) {
        return res;
    }

    FSIZE_t file_size = f_size(&file);

    std::string content;
    content.resize(file_size);

    UINT bytes_read = 0;
    res = f_read(&file, content.data(), file_size, &bytes_read);

    f_close(&file);

    if (res != FR_OK) {
        return res;
    }

    content.resize(bytes_read);

    // Busquem la clau
    std::string lookup_key = key;
    size_t key_pos = content.find(lookup_key);

    if (key_pos == std::string::npos) {
        // La clau no existeix: l'afegim al final

        if (!content.empty()) {
            if (content.size() < 2 ||
                content.substr(content.size() - 2) != "\r\n") {
                content += "\r\n";
            }
        }

        content += lookup_key;
        content += "\r\n";
        content += value;
        content += "\r\n";
    } else {
        // La clau existeix: actualitzem el valor

        size_t value_start = key_pos + lookup_key.length() + 2;

        if (value_start > content.size()) {
            return FR_INVALID_OBJECT;
        }

        size_t value_end = content.find("\r\n", value_start);

        if (value_end == std::string::npos) {
            value_end = content.size();
        }

        content.replace(value_start, value_end - value_start, value);
    }

    // Ara sobreescrivim el fitxer complet amb el contingut actualitzat
    res = f_open(&file, filename.c_str(), FA_WRITE | FA_CREATE_ALWAYS);

    if (res != FR_OK) {
        return res;
    }

    UINT bytes_written = 0;

    res = f_write(
        &file,
        content.c_str(),
        content.size(),
        &bytes_written
    );

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    if (bytes_written != content.size()) {
        f_close(&file);
        return FR_DISK_ERR;
    }

    res = f_sync(&file);

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    res = f_close(&file);

    if (res == FR_OK) {
        ram_disk_mark_dirty();
    }

    return res;
}
