#include <string>
#include <cstring>
#include <cstdio>
#include "ff.h"

class myScreen;

void filesystem_screen(myScreen* screen);

static FATFS fs;
static FIL file;

static constexpr size_t WORKBUFFER_SIZE = 4096;
static uint8_t work_buffer[WORKBUFFER_SIZE];
static FRESULT res;

FRESULT mountfs();
FRESULT unmountfs();
FRESULT format_disk();
FRESULT file_exists(const char* path);
std::string read_key(const std::string& filename, const std::string& key);
FRESULT write_key(const std::string& filename, const std::string& key, const std::string& value);
FRESULT load_config(const std::string& filename, std::string* content);
void normalize_text(std::string* text);