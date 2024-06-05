#pragma once

#include_next <lv_conf.h>

#undef LV_USE_IMG
#define LV_USE_IMG 1

#undef LV_USE_GIF
#define LV_USE_GIF 1

#undef LV_USE_FS_LITTLEFS
#define LV_USE_FS_LITTLEFS 1

#undef LV_FS_LITTLEFS_LETTER
#define LV_FS_LITTLEFS_LETTER 'L'

#undef LV_FS_LITTLEFS_CACHE_SIZE
#define LV_FS_LITTLEFS_CACHE_SIZE 512

#undef LV_MEM_SIZE
#define LV_MEM_SIZE (32U * 1024U)

#undef LV_USE_USER_DATA
#define LV_USE_USER_DATA 1
