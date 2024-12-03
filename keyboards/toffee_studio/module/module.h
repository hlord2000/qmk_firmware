#ifndef MODULE_H__
#define MODULE_H__
#include "lfs.h"

lfs_t lfs;

#undef LFS_TRACE
#define LFS_TRACE_(fmt, ...) \
    uprintf("%s:%d:trace: " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)

#endif
