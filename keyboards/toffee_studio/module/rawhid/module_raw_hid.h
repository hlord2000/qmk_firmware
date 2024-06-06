#ifndef MODULE_RAW_HID_H__
#define MODULE_RAW_HID_H__

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

#define IMG_NAME_LEN 16

enum module_command_id {
    id_module_base                  = 0x50,
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
    id_module_end                   = 0x59,
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

int module_raw_hid_parse_packet(uint8_t *data, uint8_t length);

#endif
