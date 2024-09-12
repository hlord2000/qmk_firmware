#ifndef MODULE_RAW_HID_H__
#define MODULE_RAW_HID_H__

#include <stdint.h>
#include "usb_descriptor.h"

enum module_command_id {
    id_module_cmd_base              = 0x50,
    id_module_cmd_ls                = 0x50,
    id_module_cmd_cd                = 0x51,
    id_module_cmd_pwd               = 0x52,
    id_module_cmd_rm                = 0x53,
    id_module_cmd_mkdir             = 0x54,
    id_module_cmd_touch             = 0x55,
    id_module_cmd_cat               = 0x56,
    id_module_cmd_open              = 0x57,
    id_module_cmd_write             = 0x58,
    id_module_cmd_close             = 0x59,
    id_module_cmd_format_filesystem = 0x5A,
    id_module_cmd_flash_remaining   = 0x5B,
    id_module_cmd_choose_image      = 0x5C,
    id_module_cmd_write_display     = 0x5D,
    id_module_cmd_set_time          = 0x5E,
    id_module_cmd_end               = 0xFF,
};

typedef int module_raw_hid_parse_t(uint8_t *data, uint8_t length);

enum module_return_codes {
    module_ret_success                = 0x00,
    module_ret_image_already_exists   = 0xE1,
    module_ret_image_flash_full       = 0xE2,
    module_ret_image_w_oob            = 0xE3,
    module_ret_image_h_oob            = 0xE4,
    module_ret_image_name_in_use      = 0xE5,
    module_ret_image_not_found        = 0xE6,
    module_ret_image_not_open         = 0xE7,
    module_ret_image_packet_id_err    = 0xE8,
    module_ret_flash_remaining        = 0xE9,
    module_ret_invalid_command        = 0xEF,
};

struct __attribute__((packed)) packet_header {
    uint8_t via_magic;
    uint8_t  command_id;
    uint32_t packet_id;
};

#define PACKET_DATA_SIZE (RAW_EPSIZE - sizeof(struct packet_header))

struct __attribute__((packed)) ls_packet {
    struct packet_header header;
};

struct __attribute__((packed)) cd_packet {
    struct packet_header header;
};

struct __attribute__((packed)) pwd_packet {
    struct packet_header header;
};

struct __attribute__((packed)) rm_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) mkdir_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) touch_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) cat_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) open_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) write_packet {
    struct packet_header header;
    uint8_t data[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) close_packet {
    struct packet_header header;
};

struct __attribute__((packed)) format_filesystem_packet {
    struct packet_header header;
};

struct __attribute__((packed)) flash_remaining_packet {
    struct packet_header header;
};

struct __attribute__((packed)) choose_image_packet {
    struct packet_header header;
    uint8_t directory[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) write_display_packet {
    struct packet_header header;
    uint8_t data[PACKET_DATA_SIZE];
};

struct __attribute__((packed)) set_time_packet {
    struct packet_header header;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

int module_raw_hid_parse_packet(uint8_t *data, uint8_t length);

#endif
