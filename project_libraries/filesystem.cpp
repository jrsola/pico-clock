#include <cstring>
#include <cstdio>
#include "filesystem.h"
#include "disk_config.h"
#include "my_screen.h"
#include "ram_disk.h"

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
    if (mountfs() != FR_OK) return res;
    
    FILINFO finfo;
    return f_stat(path, &finfo);
}

// private function to write a text file
static FRESULT write_text_file(const std::string& filename, const std::string& content)
{
    FRESULT res = mountfs();

    if (res != FR_OK) {
        return res;
    }

    FIL file;

    res = f_open(&file, filename.c_str(), FA_WRITE | FA_CREATE_ALWAYS);

    if (res != FR_OK) {
        return res;
    }

    UINT bytes_written = 0;
    UINT bytes_to_write = content.size();

    res = f_write(&file, content.c_str(), bytes_to_write, &bytes_written);

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    if (bytes_written != bytes_to_write) {
        f_close(&file);
        return FR_DISK_ERR;
    }

    res = f_sync(&file);

    if (res != FR_OK) {
        f_close(&file);
        return res;
    }

    return f_close(&file);
}


// looks for a key in the configuration file and returns its value
// if the key can't be found it returns the value "key_not_found"
std::string read_config(const std::string& filename, const std::string& key) {
    // if we did not pass a filename or key, return an error
    if (filename.empty() || key.empty()) return "invalid_parameters";

    // if config file does not exist, report error
    if (file_exists(filename.c_str()) != FR_OK) return "config_file_missing";

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

FRESULT write_config(const std::string& filename,
                     const std::string& key,
                     const std::string& value)
{
    if (filename.empty() || key.empty()) {
        return FR_INVALID_PARAMETER;
    }

    // Evitem claus que trenquin el format [CLAU]
    if (key.find('\r') != std::string::npos ||
        key.find('\n') != std::string::npos ||
        key.find('[')  != std::string::npos ||
        key.find(']')  != std::string::npos) {
        return FR_INVALID_NAME;
    }

    // Primer mirem si la clau ja existeix
    std::string current_value = read_config(filename, key);

    // Cas 1: el fitxer no existeix encara
    if (current_value == "config_file_missing") {
        std::string new_content;

        new_content += "[" + key + "]\r\n";
        new_content += "\r\n";
        new_content += value + "\r\n";

        return write_text_file(filename, new_content);
    }

    // Cas 2: error obrint el fitxer
    if (current_value == "error_opening_file") {
        return FR_DISK_ERR;
    }

    // Cas 3: paràmetres invàlids
    if (current_value == "invalid_parameters") {
        return FR_INVALID_PARAMETER;
    }

    // Ara sabem que el fitxer existeix.
    // Llegim tot el contingut per poder modificar-lo.
    FIL file;
    FRESULT res = f_open(&file, filename.c_str(), FA_READ);

    if (res != FR_OK) {
        return res;
    }

    FSIZE_t file_size = f_size(&file);

    std::string content;
    content.resize(file_size);

    UINT bytes_read = 0;

    if (file_size > 0) {
        res = f_read(&file, content.data(), file_size, &bytes_read);

        if (res != FR_OK) {
            f_close(&file);
            return res;
        }

        content.resize(bytes_read);
    }

    f_close(&file);

    // Normalitzem salts de línia: traiem '\r' i treballem amb '\n'
    std::string normalized;
    normalized.reserve(content.size());

    for (char c : content) {
        if (c != '\r') {
            normalized.push_back(c);
        }
    }

    std::string wanted_key = "[" + key + "]";

    // Cas 4: el fitxer existeix però la clau no hi és
    if (current_value == "key_not_found") {
        if (!normalized.empty() && normalized.back() != '\n') {
            normalized += "\n";
        }

        if (!normalized.empty()) {
            normalized += "\n";
        }

        normalized += wanted_key + "\n";
        normalized += "\n";
        normalized += value + "\n";
    }

    // Cas 5: la clau existeix i cal actualitzar el valor
    else {
        size_t pos = 0;
        bool updated = false;

        while (pos < normalized.size()) {
            size_t line_end = normalized.find('\n', pos);

            if (line_end == std::string::npos) {
                line_end = normalized.size();
            }

            std::string line = normalized.substr(pos, line_end - pos);

            if (line == wanted_key) {
                // Hem trobat [CLAU].
                // Ara busquem la primera línia no buida després de la clau.
                size_t value_pos = line_end + 1;

                while (value_pos < normalized.size()) {
                    size_t value_end = normalized.find('\n', value_pos);

                    if (value_end == std::string::npos) {
                        value_end = normalized.size();
                    }

                    std::string existing_value =
                        normalized.substr(value_pos, value_end - value_pos);

                    if (!existing_value.empty()) {
                        // Substituïm el valor existent pel nou valor
                        normalized.replace(value_pos,
                                           value_end - value_pos,
                                           value);
                        updated = true;
                        break;
                    }

                    value_pos = value_end + 1;
                }

                // La clau existia però no tenia cap valor sota.
                if (!updated) {
                    if (!normalized.empty() && normalized.back() != '\n') {
                        normalized += "\n";
                    }

                    normalized += "\n";
                    normalized += value;
                    normalized += "\n";

                    updated = true;
                }

                break;
            }

            pos = line_end + 1;
        }

        // Per seguretat: si read_config havia trobat valor però aquí no hem pogut actualitzar,
        // afegim la clau al final.
        if (!updated) {
            if (!normalized.empty() && normalized.back() != '\n') {
                normalized += "\n";
            }

            normalized += "\n";
            normalized += wanted_key + "\n";
            normalized += "\n";
            normalized += value + "\n";
        }
    }

    // Tornem a convertir a CRLF per Windows
    std::string output;
    output.reserve(normalized.size() + 32);

    for (char c : normalized) {
        if (c == '\n') {
            output += "\r\n";
        } else {
            output += c;
        }
    }

    return write_text_file(filename, output);
}
