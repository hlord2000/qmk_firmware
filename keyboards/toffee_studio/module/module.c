#if 1
#include <stdio.h>
#include <string.h>
#include "print.h"
#include "quantum.h"
#include "file_system.h"
#include "lfs.h"
#include "qp.h"
#include "qp_st7735.h"
#include "graphics/thintel15.qff.c"
#include "qp_lvgl.h"
#include "lvgl.h"

#ifdef VIA_ENABLE

/* Custom VIA commands for image, clock management */

#define INIT_PATH_LEN 16

#define IMG_NAME_LEN 26

/* Module command IDs. Must be first byte of the packet */

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

enum module_command_id {
    id_module_create_image          = 0x50,
    id_module_create_image_animated = 0x51,
    id_module_open_image            = 0x52,
    id_module_write_image           = 0x53,
    id_module_close_image           = 0x54,
    id_module_delete_image          = 0x55,
    id_module_choose_image          = 0x56,
    id_module_flash_remaining       = 0x57,
    id_module_format_filesystem     = 0x58,
    id_module_set_time              = 0x59,
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

static lfs_t lfs;

void board_init(void) {
    rp2040_mount_lfs(&lfs);

    char path[INIT_PATH_LEN] = {0};
    path[0] = LV_FS_LITTLEFS_LETTER;
    path[1] = ':';

    int err = lfs_mkdir(&lfs, path);
    if (err != 0 || err != LFS_ERR_EXIST) {
        uprintf("Error creating directory: %d\n", err);
    }

    strlcat(path, "/img", INIT_PATH_LEN);

    err = lfs_mkdir(&lfs, path);
    if (err != 0 || err != LFS_ERR_EXIST) {
        uprintf("Error creating directory: %d\n", err);
    }

    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;
    debug_mouse    = true;
}

static painter_device_t      oled;
static painter_font_handle_t font;

#include "qp_comms.h"
#include "qp_st7735_opcodes.h"

__attribute__((weak)) void ui_init(void) {
    oled = qp_st7735_make_spi_device(128, 160, 0xFF, OLED_DC_PIN, 0xFF, 8, 0);
    font = qp_load_font_mem(font_thintel15);

    qp_init(oled, QP_ROTATION_180);

    qp_comms_start(oled);
    qp_comms_command(oled, ST7735_SET_INVERSION_CTL);
    qp_comms_stop(oled);

    qp_power(oled, true);

    if (qp_lvgl_attach(oled)) {
        volatile lv_fs_drv_t *result;

        result = lv_fs_littlefs_set_driver(LV_FS_LITTLEFS_LETTER, &lfs);
        if (result == NULL) {
            uprintf("Error mounting LFS");
        }

        lv_obj_t *background = lv_obj_create(lv_scr_act());
        lv_obj_set_size(background, 128, 160);
        lv_obj_set_style_bg_color(background, lv_color_hex(0xFF0000), 0);
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
}
#endif //QUANTUM_PAINTER_ENABLE

static lv_fs_file_t lvgl_current_file;
static lfs_file_t lfs_current_file;

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[1]);
    uint8_t *command_data = (uint8_t *)(&(data[2]));

    uprintf("Data length: %d\n", length);
    uprintf("Sequence ID: %d\n", data[0]);
    uprintf("Command ID: %d\n", *command_id);

    int err;
    lv_fs_res_t res;
    switch (*command_id) {
    case id_module_create_image:
        struct img_create_packet *create_packet = (struct img_create_packet *)command_data;

        err = lfs_file_open(&lfs, &lfs_current_file, create_packet->image_name, \
                LFS_O_RDWR | LFS_O_CREAT);
        if (err < 0) {
            uprintf("Error creating file, littlefs: %d\n", err);
        }

        err = lfs_file_close(&lfs, &lfs_current_file);
        if (err < 0) {
            uprintf("Error closing file, littlefs: %d\n", err);
        }

        uprintf("Success\n");

        break;
    case id_module_create_image_animated:
        struct img_create_animated_packet *create_animated_packet = \
            (struct img_create_animated_packet *)command_data;
        break;
    case id_module_open_image:
        uprintf("Open image\n");
        break;
    case id_module_write_image:
        uprintf("Write image\n");
        break;
    case id_module_close_image:
        uprintf("Close image\n");
        break;
    case id_module_delete_image:
        uprintf("Delete image\n");
        break;
    case id_module_choose_image:
        uprintf("Choose image\n");
        break;
    case id_module_flash_remaining:
        uprintf("Flash remaining\n");
        break;
    case id_module_format_filesystem:
        uprintf("Format filesystem\n");
        err = rp2040_format_lfs(&lfs);
        if (err < 0) {
            uprintf("Error formatting filesystem: %d\n", err);
        }
        err = rp2040_mount_lfs(&lfs);
        if (err < 0) {
            uprintf("Error mounting filesystem: %d\n", err);
        }
        break;
    case id_module_set_time:
        uprintf("Set time\n");
        break;
    default:
        uprintf("Invalid command\n");
        break;
    }
    uprintf("Data: %s\n", command_data);
    *command_id = 0xFF;
}
#endif
#endif


