#include "print.h"
#include "usb_descriptor.h"
#include "raw_hid.h"
#include "file_system.h"
#include "lfs.h"
#include "module.h"
#include "module_raw_hid.h"
#include "lvgl.h"

#define DIRECTORY_MAX 64

lfs_file_t current_file;
char current_directory[DIRECTORY_MAX];

// Helper function to open a file
static int open_file(lfs_t *lfs, lfs_file_t *file, const char *path, int flags) {
    int err = lfs_file_open(lfs, file, path, flags);
    if (err < 0) {
        uprintf("Error opening file %s: %d\n", path, err);
    }
    return err;
}

// Helper function to close a file
static int close_file(lfs_t *lfs, lfs_file_t *file) {
    int err = lfs_file_sync(lfs, file);
    if (err < 0) {
        uprintf("Error syncing file: %d\n", err);
    }

    err = lfs_file_close(lfs, file);
    if (err < 0) {
        uprintf("Error closing file: %d\n", err);
    }
    return err;
}

static int parse_ls(uint8_t *data, uint8_t length) {
    uprintf("List files\n");

    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, ".");
    if (err < 0) {
        uprintf("Error opening directory: %d\n", err);
        uint8_t return_buf[RAW_EPSIZE] = {0};
        return_buf[0] = module_ret_invalid_command;
        raw_hid_send(return_buf, RAW_EPSIZE);
        return err;
    }

    struct lfs_info info;
    uint8_t return_buf[RAW_EPSIZE] = {0};
    int offset = 1;  // Start at index 1 to leave room for the return code
    return_buf[0] = module_ret_success;  // Set the initial return code

    while (true) {
        int res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            uprintf("Error reading directory: %d\n", res);
            return_buf[0] = module_ret_invalid_command;
            raw_hid_send(return_buf, RAW_EPSIZE);
            lfs_dir_close(&lfs, &dir);
            return res;
        }
        if (res == 0) {
            break;
        }

        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
            continue;
        }

        int name_len = strlen(info.name);
        if (offset + name_len + 2 > RAW_EPSIZE) {
            raw_hid_send(return_buf, RAW_EPSIZE);
            memset(return_buf + 1, 0, RAW_EPSIZE - 1);  // Clear all but the first byte
            offset = 1;
        }

        strncpy((char *)return_buf + offset, info.name, name_len);
        offset += name_len;
        return_buf[offset++] = info.type == LFS_TYPE_DIR ? '/' : ' ';
        return_buf[offset++] = '\n';
    }

    lfs_dir_close(&lfs, &dir);

    if (offset > 1) {  // If we have any data (more than just the return code)
        raw_hid_send(return_buf, RAW_EPSIZE);
    }

    return module_ret_success;
}

static int parse_cd(uint8_t *data, uint8_t length) {
    uprintf("Change directory\n");

    struct cd_packet *cd_data = (struct cd_packet *)data;
    char *new_directory = (char *)cd_data->header.packet_id;

    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, new_directory);
    if (err < 0) {
        uprintf("Error opening directory: %d\n", err);
        return module_ret_invalid_command;
    }

    lfs_dir_close(&lfs, &dir);

    // Update current directory (assuming you have a global variable for this)
    // strncpy(current_directory, new_directory, LFS_NAME_MAX);

    uprintf("Changed to directory: %s\n", new_directory);
    return module_ret_success;
}

static int parse_pwd(uint8_t *data, uint8_t length) {
    uprintf("Print working directory\n");

    // Assuming you have a global variable for current directory
    // uprintf("Current directory: %s\n", current_directory);

    uint8_t return_buf[RAW_EPSIZE] = {0};
    // strncpy((char *)return_buf, current_directory, RAW_EPSIZE - 1);
    raw_hid_send(return_buf, RAW_EPSIZE);

    return module_ret_success;
}

static int parse_rm(uint8_t *data, uint8_t length) {
    uprintf("Remove file/directory\n");

    struct rm_packet *rm_data = (struct rm_packet *)data;
    char *path = (char *)rm_data->directory;

    int err = lfs_remove(&lfs, path);
    if (err < 0) {
        uprintf("Error removing file/directory: %d\n", err);
        return err;
    }

    return module_ret_success;
}

static int parse_mkdir(uint8_t *data, uint8_t length) {
    uprintf("Make directory\n");

    struct mkdir_packet *mkdir_data = (struct mkdir_packet *)data;
    char *path = (char *)mkdir_data->directory;

    int err = lfs_mkdir(&lfs, path);
    if (err < 0) {
        uprintf("Error creating directory: %d\n", err);
        return err;
    }

    return module_ret_success;
}

static int parse_touch(uint8_t *data, uint8_t length) {
    uprintf("Create empty file\n");

    struct touch_packet *touch_data = (struct touch_packet *)data;
    char *path = (char *)touch_data->directory;

    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT);
    if (err < 0) {
        return err;
    }

    err = lfs_file_close(&lfs, &file);
    if (err < 0) {
        return err;
    }

    return module_ret_success;
}

static int parse_cat(uint8_t *data, uint8_t length) {
    uprintf("Read file contents\n");

    struct cat_packet *cat_data = (struct cat_packet *)data;
    char *path = (char *)cat_data->directory;

    lfs_file_t file;
    int err = open_file(&lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        return err;
    }

    uint8_t return_buf[RAW_EPSIZE] = {0};
    lfs_ssize_t bytes_read;
    while ((bytes_read = lfs_file_read(&lfs, &file, return_buf + 1, RAW_EPSIZE - 1)) > 0) {
        uprintf("Read %d bytes\n", bytes_read);
        uprintf("File %s\n", path);
        for (int i = 0; i < RAW_EPSIZE; i++) {
            uprintf("%x ", return_buf[i]);
        }
        uprintf("\n");
        return_buf[0] = 0;
        raw_hid_send(return_buf, RAW_EPSIZE);
    }

    if (bytes_read < 0) {
        uprintf("Error reading file: %d\n", bytes_read);
        close_file(&lfs, &file);
        return bytes_read;
    }

    err = close_file(&lfs, &file);
    if (err < 0) {
        return err;
    }

    return module_ret_success;
}

static int parse_open(uint8_t *data, uint8_t length) {
    uprintf("Open file\n");

    struct open_packet *open_data = (struct open_packet *)data;
    char *path = (char *)open_data->directory;

    int err = open_file(&lfs, &current_file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err < 0) {
        return err;
    }

    return module_ret_success;
}

static int parse_write(uint8_t *data, uint8_t length) {
    uprintf("Write to file\n");

    struct write_packet *write_data = (struct write_packet *)data;

     lfs_ssize_t written = lfs_file_write(&lfs, &current_file, write_data->data, length - sizeof(struct packet_header));
     if (written < 0) {
         uprintf("Error writing to file: %d\n", written);
         return written;
     }

    uint8_t return_buf[RAW_EPSIZE] = {0};
    raw_hid_send(return_buf, RAW_EPSIZE);

    return module_ret_success;
}

static int parse_close(uint8_t *data, uint8_t length) {
    uprintf("Close current file\n");

    int err = close_file(&lfs, &current_file);
    if (err < 0) {
        return err;
    }

    return module_ret_success;
}

static int parse_format_filesystem(uint8_t *data, uint8_t length) {
    uprintf("Format filesystem\n");
    int err = rp2040_format_lfs(&lfs);
    if (err < 0) {
        uprintf("Error formatting filesystem: %d\n", err);
        return err;
    }
    err = rp2040_mount_lfs(&lfs);
    if (err < 0) {
        uprintf("Error mounting filesystem: %d\n", err);
        return err;
    }
    return module_ret_success;
}

static int parse_flash_remaining(uint8_t *data, uint8_t length) {
    uprintf("Flash remaining\n");
    lfs_ssize_t size = lfs_fs_size(&lfs);

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[1] = 128 - size;
    raw_hid_send(return_buf, RAW_EPSIZE);

    uprintf("Size: %li\n", 128 - size);
    return module_ret_success;
}

uint8_t current_image_map[(128*128) * LV_COLOR_DEPTH / 8];

lv_img_dsc_t current_image = {
    .header.always_zero = 0,
    .header.w = 128,
    .header.h = 128,
    .data_size = (128 * 128) * LV_COLOR_SIZE / 8,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = current_image_map,
};

LV_IMG_DELCARE(current_image);

static int parse_choose_image(uint8_t *data, uint8_t length) {
    uprintf("Choose image\n");
    struct choose_image_packet *choose_data = (struct choose_image_packet *)data;
    char *path = (char *)choose_data->directory;

    char full_path[256];  // Adjust the size as needed
    snprintf(full_path, sizeof(full_path), "L:%s", path);
    uprintf("File path: %s\n", full_path);

    lfs_file_t file;
    int err = open_file(&lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        uprintf("Error opening image file: %d\n", err);
        return err;
    }

    lfs_ssize_t bytes_read = lfs_file_read(&lfs, &file, current_image_map, sizeof(current_image_map));
    if (bytes_read < 0) {
        uprintf("Error reading image file: %d\n", bytes_read);
        close_file(&lfs, &file);
        return bytes_read;
    }

    if (bytes_read != sizeof(current_image_map)) {
        uprintf("Warning: Read %d bytes, expected %d bytes\n", bytes_read, sizeof(current_image_map));
    }

    err = close_file(&lfs, &file);
    if (err < 0) {
        uprintf("Error closing image file: %d\n", err);
        return err;
    }

    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &current_image);

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[0] = module_ret_success;
    raw_hid_send(return_buf, RAW_EPSIZE);
    return module_ret_success;
}

uint32_t image_write_pointer = 0;
static int parse_write_display(uint8_t *data, uint8_t length) {
    uprintf("Write to display\n");
    struct write_display_packet *write_data = (struct write_display_packet *)data;

    // Calculate the number of bytes to write
    uint32_t bytes_to_write = length - sizeof(struct packet_header);

    // Write the data to the current_image_map buffer
    for (uint32_t i = 0; i < bytes_to_write; i++) {
        current_image_map[image_write_pointer] = write_data->data[i];
        image_write_pointer++;

        // Check if we've reached the end of the buffer and roll over if necessary
        if (image_write_pointer >= sizeof(current_image_map)) {
            image_write_pointer = 0;
        }
    }

    // Update the display
    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &current_image);

    uprintf("Wrote %u bytes to display buffer\n", bytes_to_write);

    return module_ret_success;
}

static int parse_set_time(uint8_t *data, uint8_t length) {
    uprintf("Set time\n");

    struct set_time_packet *time_data = (struct set_time_packet *)data;

    // Implement logic to set the system time
    // This might involve updating an RTC or internal time counter

    uprintf("Time set to: %02d:%02d:%02d\n", time_data->hour, time_data->minute, time_data->second);
    return module_ret_success;
}

static module_raw_hid_parse_t* parse_packet_funcs[] = {
    parse_ls,
    parse_cd,
    parse_pwd,
    parse_rm,
    parse_mkdir,
    parse_touch,
    parse_cat,
    parse_open,
    parse_write,
    parse_close,
    parse_format_filesystem,
    parse_flash_remaining,
    parse_choose_image,
    parse_write_display,
    parse_set_time,
};

int module_raw_hid_parse_packet(uint8_t *data, uint8_t length) {
    int err;
    uint8_t return_buf[RAW_EPSIZE] = {0};

    uprintf("Received packet. Parsing command.\r\n");

    uprintf("Buffer contents: ");
    for (int i = 0; i < length; i++) {
        uprintf("%02X ", data[i]);
    }
    uprintf("\n");

    if (length < sizeof(struct packet_header) || length > RAW_EPSIZE) {
        uprintf("Invalid packet length\n");
        return -1;
    }

    struct packet_header *header = (struct packet_header *)(data);
    uint8_t command_id = header->command_id - id_module_cmd_base;
    uprintf("Command ID: %d\n", command_id);
    if (command_id >= id_module_cmd_end - id_module_cmd_base) {
        uprintf("Invalid command ID\n");
        return -1;
    }

    err = parse_packet_funcs[command_id](data, length);
    if (err < 0) {
        uprintf("Error parsing packet: %d\n", err);
        return_buf[0] = err;
    }
    return_buf[0] = 0;
    raw_hid_send(return_buf, RAW_EPSIZE);

    return err;
}
