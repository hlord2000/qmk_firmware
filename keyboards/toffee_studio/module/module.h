#ifndef MODULE_H__
#define MODULE_H__
#include "lfs.h"

lfs_t lfs;

#undef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES 16 * 1024 * 1024

#endif
