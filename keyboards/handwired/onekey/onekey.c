// Copyright 2022 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "file_system.h"
#include "lfs.h"
#include "print.h"

lfs_t lfs;
uint32_t boot_count = 0;

void board_init(void) {
    rp2040_mount_lfs(&lfs);

    lfs_file_t file;

    uint32_t file_buffer[FLASH_SECTOR_SIZE / 4];

    const struct lfs_file_config cfg = {
        .buffer = file_buffer,
    };

    lfs_file_opencfg(&lfs, &file, "test", LFS_O_RDWR | LFS_O_CREAT, &cfg);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    boot_count++;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
    lfs_file_close(&lfs, &file);

    rp2040_unmount_lfs(&lfs);

    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;
    debug_mouse    = true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uprintf("Boot count: %d\n", boot_count);
}
