#include "lfs.h"
#include <stdint.h>
#include <ch.h>
#include "hardware/flash.h"
#include "hardware/sync.h"

#define XSTR(x) STR(x)
#define STR(x) #x

extern char __flash_binary_end;
#define FLASH_BINARY_END (((uintptr_t)&__flash_binary_end + 0x1000) & ~0xFFF)
#define FILE_SYSTEM_RP2040_FLASH_BASE (FLASH_BINARY_END - XIP_BASE)

#define FS_SIZE (16 * 4096)

static int rp2040_flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    const void *addr = (void *)((FILE_SYSTEM_RP2040_FLASH_BASE + (block * c->block_size) + off) + XIP_BASE);
    memcpy(buffer, addr, size);
    return 0;
}

static int rp2040_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {

    // The RP2040 requires 256 byte aligned writes, this checks the offset of the write.
    if (off % 256 != 0) {
        return -1;
    }

    // The RP2040 requires 256 byte aligned writes, this checks the size of the write buffer.
    if (size % 256 != 0) {
        return -1;
    }

    uint32_t flash = (FILE_SYSTEM_RP2040_FLASH_BASE + (block * c->block_size) + off);

    // Lock the scheduler and disable interrupts.
    uint32_t interrupts = save_and_disable_interrupts();
    // Write the data to flash. The base address is the end of the XIP region.
    flash_range_program(flash, (uint8_t *)buffer, (size_t)size);
    restore_interrupts(interrupts);

    // Restore interrupts and unlock the scheduler.
    return 0;
}

static int rp2040_flash_erase(const struct lfs_config *c, lfs_block_t block) {

    // Make sure the block is within the range of the flash.
    if (block >= c->block_count) {
        return -1;
    }

    if (block >= FILE_SYSTEM_RP2040_FLASH_BASE / c->block_size) {
        return -1;
    }

    uint32_t flash = (FILE_SYSTEM_RP2040_FLASH_BASE + (block * c->block_size));

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(flash, c->block_size);
    restore_interrupts(interrupts);
    return 0;
}

static int rp2040_flash_sync(const struct lfs_config *c) {
    return 0;
}

uint32_t read_buffer[FLASH_SECTOR_SIZE / 4];
uint32_t prog_buffer[FLASH_SECTOR_SIZE / 4];
uint32_t lookahead_buffer[32];

const struct lfs_config cfg = {
    // block device operations
    .read  = rp2040_flash_read,
    .prog  = rp2040_flash_prog,
    .erase = rp2040_flash_erase,
    .sync  = rp2040_flash_sync,
    // block device configuration
    .read_size = 1,
    .prog_size = FLASH_PAGE_SIZE,
    .block_size = FLASH_SECTOR_SIZE,
    .block_count = 128,
    .cache_size = FLASH_SECTOR_SIZE / 4,
    .lookahead_size = 32,
    .block_cycles = 1000,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

int rp2040_mount_lfs(lfs_t *lfs) {
    int err = lfs_mount(lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(lfs, &cfg);
        lfs_mount(lfs, &cfg);
    }
    return 0;
}

int rp2040_format_lfs(lfs_t *lfs) {
    lfs_format(lfs, &cfg);
    return 0;
}

int rp2040_unmount_lfs(lfs_t *lfs) {
    lfs_unmount(lfs);
    return 0;
}
