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
#define FS_SIZE (8 * 1024 * 1024)  // 8MB filesystem

static int rp2040_flash_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    const uint32_t addr = FILE_SYSTEM_RP2040_FLASH_BASE + (block * FLASH_SECTOR_SIZE) + off;
    memcpy(buffer, (const void *)(addr + XIP_BASE), size);
    return LFS_ERR_OK;
}

static int rp2040_flash_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = FILE_SYSTEM_RP2040_FLASH_BASE + (block * FLASH_SECTOR_SIZE) + off;

    // Check if write is aligned to page boundary
    if (off % FLASH_PAGE_SIZE != 0 || size != FLASH_PAGE_SIZE) {
        return LFS_ERR_INVAL;  // LittleFS should only send page-aligned writes
    }

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_program(addr, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(interrupts);

    return LFS_ERR_OK;
}

static int rp2040_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = FILE_SYSTEM_RP2040_FLASH_BASE + (block * FLASH_SECTOR_SIZE);

    // Verify block is within bounds
    if (block >= c->block_count) {
        return LFS_ERR_INVAL;
    }

    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(addr, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);

    return LFS_ERR_OK;
}

static int rp2040_flash_sync(const struct lfs_config *c) {
    return LFS_ERR_OK;
}

// Static allocation of buffers (must be aligned)
static uint32_t read_buffer[FLASH_PAGE_SIZE / sizeof(uint32_t)] __attribute__((aligned(4)));
static uint32_t prog_buffer[FLASH_PAGE_SIZE / sizeof(uint32_t)] __attribute__((aligned(4)));
static uint32_t lookahead_buffer[32] __attribute__((aligned(4)));  // Increased for larger filesystem

const struct lfs_config cfg = {
    // Block device operations
    .read  = rp2040_flash_read,
    .prog  = rp2040_flash_prog,
    .erase = rp2040_flash_erase,
    .sync  = rp2040_flash_sync,

    // Block device configuration
    .read_size = 1,                    // Can read any size
    .prog_size = FLASH_PAGE_SIZE,      // Must write exactly 256 bytes
    .block_size = FLASH_SECTOR_SIZE,   // Each block is a 4KB sector
    .block_count = FS_SIZE / FLASH_SECTOR_SIZE,  // 2048 blocks (8MB / 4KB)
    .cache_size = FLASH_PAGE_SIZE,     // Cache size matches program size
    .lookahead_size = 32,             // Increased lookahead for larger filesystem
    .block_cycles = 500,              // Conservative wear leveling

    // Static buffer configuration
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

int rp2040_mount_lfs(lfs_t *lfs) {
    int err = lfs_mount(lfs, &cfg);
    if (err) {
        err = lfs_format(lfs, &cfg);
        if (err) {
            return err;
        }
        err = lfs_mount(lfs, &cfg);
    }
    return err;
}

int rp2040_format_lfs(lfs_t *lfs) {
    return lfs_format(lfs, &cfg);
}

int rp2040_unmount_lfs(lfs_t *lfs) {
    return lfs_unmount(lfs);
}
