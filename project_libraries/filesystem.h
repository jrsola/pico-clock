#include <string>
#include <cstring>
#include <cstdio>
#include "ff.h"


static FATFS fs;
static FIL file;

static constexpr size_t WORKBUFFER_SIZE = 4096;
static uint8_t work_buffer[WORKBUFFER_SIZE];
static FRESULT res;


bool mountfs();
bool format_disk();
bool file_exists(const char* path);
bool read_file(const char* path, char* buffer, size_t bufsize);
bool write_file(const char* path, const char* content);