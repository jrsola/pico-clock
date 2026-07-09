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

// loads the content of a config file into a string
FRESULT load_config(const std::string& filename, std::string* content){
    
    // if filename is empty or pointer to content is null, return error
    if (filename.empty() || content == nullptr) return FR_INVALID_NAME;

    // if file does not exist, return error
    if (file_exists(filename.c_str()) != FR_OK) return res;

    // empty content
    content->clear();

    FIL file;
    FRESULT res = f_open(&file, filename.c_str(), FA_READ);

    // if we could not open the file for reading, error
    if (res != FR_OK) return res;

    FSIZE_t file_size = f_size(&file);
    
    // if file is empty, return OK, but content will be null
    if (file_size == 0){
        f_close(&file);
        return FR_OK;
    }
    
    content->resize(file_size);

    // read the content 
    UINT bytes_read = 0;
    res = f_read(&file, content->data(), file_size, &bytes_read);
    f_close(&file);

    // if there was any error reading the file, error
    if (res != FR_OK) {
        content->clear();
        return res;
    }

    // resize content to the actual bytes read
    content->resize(bytes_read);

    normalize_text(content);

    return FR_OK;
}

// normalizes the line endings in a text, so all becomes \n only.
void normalize_text(std::string* text){
    if (text == nullptr) return;
    // prepare the normalized string with same size as the original one
    std::string normalized;
    normalized.reserve(text->size());

    // run thru the text, replacing \r\n -> \n
    for (size_t i = 0; i < text->size(); i++){
        char c = (*text)[i];

        if (c == '\r'){
            c = '\n';
            // if next char is a \n skip it next
             if (i + 1 < text->size() && (*text)[i + 1] == '\n') i++;
        } 
        normalized.push_back(c);
    }

    // if the end of the file does not end in \n, add one.
    if (!normalized.empty() && normalized.back() != '\n') {
        normalized.push_back('\n');
    }
    *text = normalized;
} 


// looks for a key in the configuration file and returns its value
// if the key can't be found it returns the value "key_not_found"
std::string read_key(const std::string& filename, const std::string& key) {

    // if we did not pass a filename or key, return an empty string
    if (filename.empty() || key.empty()) return "";

    std::string content;

    FRESULT res = load_config(filename, &content);

    // if errors were found or config file is  empty, return empty string
    if (res != FR_OK || content.empty()) return "";
    
    // look for the key in the configuration
    std::string lookup_key = "[" + key + "]\n";
    size_t key_pos = content.find(lookup_key);

    // if the key could not be found, return empty string
    if (key_pos == std::string::npos) return "";

    // start from end of key
    size_t value_start = key_pos + lookup_key.length();

    // locate the next \n and the value should be between the two
    size_t value_end = content.find('\n', value_start); 

    // if there is no other \n, take the end of the file
    if (value_end == std::string::npos) value_end = content.size(); 

    return content.substr(value_start, value_end - value_start);
}

FRESULT write_key(const std::string& filename, const std::string& key, const std::string& value) {

    // if no filename or key, then return error
    if (filename.empty() || key.empty()) return FR_INVALID_NAME;

    std::string content;

    FRESULT res = load_config(filename, &content);

    // if we could not load the config file, return error
    if (res != FR_OK) return res;

    std::string current_value = read_key(filename, key);

    // key exists, replace value
    if (current_value != ""){
        size_t key_pos = content.find("["+key+"]");
        size_t value_start = key_pos + key.length()+3; // 2 for [] + \n
        size_t value_end = content.find('\n', value_start);

        if(value_end == std::string::npos) value_end = content.size();

        content.replace(value_start, value_end - value_start, value);
    } else {
        // key does not exist, create key/value at the end
        if (!content.empty() && content.back() != '\n') {
            content.push_back('\n');
        }

        content += "["+key+"]";
        content += "\n";
        content += value;
        content += "\n";
    }

    // Overwrite the updated config file
    FIL file;
    res = f_open(&file, filename.c_str(), FA_WRITE | FA_CREATE_ALWAYS);

    if (res != FR_OK) return res;

    UINT bytes_written = 0;

    res = f_write(&file, content.c_str(), content.size(), &bytes_written);

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    res = f_sync(&file);

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    res = f_close(&file);

    if (res == FR_OK) {
        ram_disk_mark_dirty();
        ram_disk_flush_to_flash();
    }

    return res;
}
