// Copyright 2022 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "file_system.h"
#include "lfs.h"
#include "print.h"
#include <stdio.h>
#include "qp.h"
#include "qp_st7735.h"
#include "graphics/thintel15.qff.c"

#ifdef LFS_TESTING
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
#endif

static painter_device_t      oled;
static painter_font_handle_t font;

__attribute__((weak)) void ui_init(void) {
    oled = qp_st7735_make_spi_device(128, 160, OLED_CS_PIN, OLED_DC_PIN, OLED_RST_PIN, 8, 0);
    font = qp_load_font_mem(font_thintel15);
    qp_init(oled, QP_ROTATION_180);
    qp_rect(oled, 0, 0, 130, 162, 0, 0, 0, true);
    qp_rect(oled, 20, 20, 108, 60, 55, 55, 55, true);
    qp_rect(oled, 20, 80, 108, 120, 55, 55, 55, true);
    qp_flush(oled);
}

uint8_t layer = 0;

__attribute__((weak)) void ui_task(void) {
    static const char *text  = "Layer:";
    int16_t            width = qp_textwidth(font, text);
    qp_drawtext(oled, 20, 140, font, text);


    switch (layer) {
        case 0:
            qp_drawtext(oled, (20 + width), 140, font, "QWERTY");
            layer = 1;
            break;
        case 1:
            qp_drawtext(oled, (20 + width), 140, font, "SYMBOL");
            layer = 0;
            break;
        case 2:
            qp_drawtext(oled, (20 + width), 140, font, "NUMBER");
            break;
        default:
            qp_drawtext(oled, (20 + width), 140, font, "_PANIC_");
            break;
    }
}

#ifdef QUANTUM_PAINTER_ENABLE
void keyboard_post_init_kb(void) {
    // Init the display
    ui_init();
    keyboard_post_init_user();
}

void housekeeping_task_kb(void) {
    // Draw the display
    ui_task();
}
#endif //QUANTUM_PAINTER_ENABLE

