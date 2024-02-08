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

#ifdef VIA_ENABLE

/* Custom VIA commands for image, clock management */

#define IMG_NAME_LEN 54

/* Magnum command IDs. Must be first byte of the packet */

/*
*  Image commands
*  N.B.:
*  - Image names are limited to 54 characters, excluding the null terminator.
*  - Image data must be received in proper packet_id order.
*    If packets are not sent in order the image will be deleted.
*
* To create a new image:
* 1. Send a create image or create image animated command
*    a. Populate the image name, width, and height
*    b. For create image animated, populate the frame count and frame delay
* 2. Send a number of write image commands
* 3. When complete, send a close image command
*
* To delete an image:
* 1. Send a delete image command
*    a. Populate the image name
*
* To choose an image:
* 1. Send a choose image command
*    a. Populate the image name
*
* To get the remaining flash space:
* 1. Send a flash remaining command
*
* To set the time:
* 1. Send a set time command
*   a. Populate time as a Unix timestamp
*
*/

enum magnum_command_id {
    id_magnum_create_image          = 0x50,
    id_magnum_create_image_animated = 0x51,
    id_magnum_open_image            = 0x52
    id_magnum_write_image           = 0x53,
    id_magnum_close_image           = 0x54,
    id_magnum_delete_image          = 0x55,
    id_magnum_choose_image          = 0x56,
    id_magnum_flash_remaining       = 0x57,
    id_magnum_set_time              = 0x58,
};

#define MAGNUM_RET_SUCCESS                0xE0
#define MAGNUM_RET_IMAGE_ALREADY_EXISTS   0xE1
#define MAGNUM_RET_IMAGE_FLASH_FULL       0xE2
#define MAGNUM_RET_IMAGE_W_OOB            0xE3
#define MAGNUM_RET_IMAGE_H_OOB            0xE4
#define MAGNUM_RET_IMAGE_NAME_IN_USE      0xE5
#define MAGNUM_RET_IMAGE_NOT_FOUND        0xE6
#define MAGNUM_RET_IMAGE_NOT_OPEN         0xE7
#define MAGNUM_RET_IMAGE_PACKET_ID_ERR    0xE8
#define MAGNUM_RET_FLASH_REMAINING        0xE9
#define MAGNUM_RET_INVALID_COMMAND        0xEF

struct packet_header {
    uint8_t  command_id;
    uint32_t packet_id;
};

struct img_create_packet {
    struct packet_header header;
    uint8_t  width;
    uint8_t  height;
    uint8_t  image_name[IMG_NAME_LEN];
};

struct img_create_animated_packet {
    struct packet_header header;
    uint8_t  width;
    uint8_t  height;
    uint8_t  image_name[IMG_NAME_LEN];
    uint8_t  frame_count;
    uint8_t  frame_delay;
};

struct img_open_packet {
    struct packet_header header;
    uint8_t  image_name[IMG_NAME_LEN];
};

struct img_write_packet {
    struct packet_header header;
    uint8_t  packet_data[57];
};

struct img_close_packet {
    struct packet_header header;
};

struct img_delete_packet {
    struct packet_header header;
    uint8_t  image_name[IMG_NAME_LEN];
};

struct img_choose_packet {
    struct packet_header header;
    uint8_t  image_name[IMG_NAME_LEN];
};

struct img_flash_remaining_packet {
    struct packet_header header;
};

struct img_set_time_packet {
    struct packet_header header;
    uint32_t  time;
};

/* TODO: Implement basic response to these commands */

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);
    uint8_t *command_data = &(data[1]);

    switch (*command_id) {
        case id_magnum_create_image:
            break;
        case id_magnum_create_image_animated:
            break;
        case id_magnum_open_image:
            break;
        case id_magnum_write_image:
            break;
        case id_magnum_close_image:
            break;
        case id_magnum_delete_image:
            break;
        case id_magnum_choose_image:
            break;
        case id_magnum_flash_remaining:
            break;
        case id_magnum_set_time:
            break;
        default:
            break;
    }
}
#endif
