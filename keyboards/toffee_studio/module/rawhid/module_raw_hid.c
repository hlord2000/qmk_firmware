#include "print.h"
#include "usb_descriptor.h"
#include "raw_hid.h"
#include "file_system.h"
#include "lfs.h"

#include "module.h"
#include "module_raw_hid.h"

static int parse_create_image(uint8_t *data, uint8_t length) {
    int err;
    uprintf("Create image\n");

    struct img_create_packet *create_packet = (struct img_create_packet *)data;
#if 0
    err = lfs_file_open(&lfs, &lfs_current_file, create_packet->image_name, \
            LFS_O_RDWR | LFS_O_CREAT);
    if (err < 0) {
        uprintf("Error creating file, littlefs: %d\n", err);
        return err;
    }

    err = lfs_file_close(&lfs, &lfs_current_file);
    if (err < 0) {
        uprintf("Error closing file, littlefs: %d\n", err);
        return err;
    }
#endif

    return module_ret_success;
}

static int parse_create_image_animated(uint8_t *data, uint8_t length) {
    uprintf("Create image animated\n");
    return module_ret_success;
}

static int parse_open_image(uint8_t *data, uint8_t length) {
    uprintf("Open image\n");
    return module_ret_success;
}

static int parse_write_image(uint8_t *data, uint8_t length) {
    uprintf("Write image\n");
    return module_ret_success;
}

static int parse_close_image(uint8_t *data, uint8_t length) {
    uprintf("Close image\n");
    return module_ret_success;
}

static int parse_delete_image(uint8_t *data, uint8_t length) {
    uprintf("Delete image\n");
    return module_ret_success;
}

static int parse_choose_image(uint8_t *data, uint8_t length) {
    uprintf("Choose image\n");
    return module_ret_success;
}

static int parse_flash_remaining(uint8_t *data, uint8_t length) {
    uprintf("Flash remaining\n");
    return module_ret_success;
}

static int parse_format_filesystem(uint8_t *data, uint8_t length) {
    int err;

    uprintf("Format filesystem\n");
    err = rp2040_format_lfs(&lfs);
    if (err < 0) {
        uprintf("Error formatting filesystem: %d\n", err);
        return err;
    }

    err = rp2040_mount_lfs(&lfs);
    if (err < 0) {
        uprintf("Error mounting filesystem: %d\n", err);
        return err;
    }

    return err;
}

static int parse_set_time(uint8_t *data, uint8_t length) {
    uprintf("Set time\n");
    return 0;
}

static module_raw_hid_parse_t* parse_packet_funcs[] = {
    parse_create_image,
    parse_create_image_animated,
    parse_open_image,
    parse_write_image,
    parse_close_image,
    parse_delete_image,
    parse_choose_image,
    parse_flash_remaining,
    parse_format_filesystem,
    parse_set_time,
};

int module_raw_hid_parse_packet(uint8_t *data, uint8_t length) {
    int err;
    uint8_t return_buf[RAW_EPSIZE] = {0};
    memset(return_buf, 0, RAW_EPSIZE);

    uint8_t command_id = data[1] - id_module_base;
    if (command_id >= id_module_end - id_module_base) {
        uprintf("Invalid command ID\n");
        return -1;
    }

    uint8_t *command_data = (uint8_t *)(&(data[2]));

    uprintf("Data length: %d\n", length);
    uprintf("Sequence ID: %d\n", data[0]);
    uprintf("Command ID: %d\n", command_id);

    err = parse_packet_funcs[command_id](command_data, length - 2);
    if (err < 0) {
        uprintf("Error parsing packet\n");
        return_buf[0] = err;
    }

    raw_hid_send(return_buf, RAW_EPSIZE);

    return err;
}

