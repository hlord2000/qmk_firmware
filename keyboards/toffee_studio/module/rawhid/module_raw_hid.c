#include "print.h"
#include "usb_descriptor.h"
#include "raw_hid.h"
#include "file_system.h"
#include "lfs.h"
#include "module.h"
#include "module_raw_hid.h"
#include "lvgl.h"

#define DIRECTORY_MAX 64
#define MAX_PATH_LENGTH 256

// Function declarations
static int parse_ls(uint8_t *data, uint8_t length);
static int parse_cd(uint8_t *data, uint8_t length);
static int parse_pwd(uint8_t *data, uint8_t length);
static int parse_rm(uint8_t *data, uint8_t length);
static int parse_mkdir(uint8_t *data, uint8_t length);
static int parse_touch(uint8_t *data, uint8_t length);
static int parse_cat(uint8_t *data, uint8_t length);
static int parse_open(uint8_t *data, uint8_t length);
static int parse_write(uint8_t *data, uint8_t length);
static int parse_close(uint8_t *data, uint8_t length);
static int parse_format_filesystem(uint8_t *data, uint8_t length);
static int parse_flash_remaining(uint8_t *data, uint8_t length);
static int parse_choose_image(uint8_t *data, uint8_t length);
static int parse_write_display(uint8_t *data, uint8_t length);
static int parse_set_time(uint8_t *data, uint8_t length);

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

        // Ensure name_len does not exceed buffer size
        if (name_len > RAW_EPSIZE - offset - 2) {
            name_len = RAW_EPSIZE - offset - 2;
        }

        if (offset + name_len + 2 > RAW_EPSIZE) {
            raw_hid_send(return_buf, RAW_EPSIZE);
            memset(return_buf + 1, 0, RAW_EPSIZE - 1);  // Clear all but the first byte
            offset = 1;
        }

        strncpy((char *)return_buf + offset, info.name, name_len);
        offset += name_len;
        return_buf[offset++] = (info.type == LFS_TYPE_DIR) ? '/' : ' ';
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

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char new_directory[DIRECTORY_MAX];
    if (path_length >= DIRECTORY_MAX) {
        uprintf("Directory name too long\n");
        return module_ret_invalid_command;
    }
    memcpy(new_directory, path_data, path_length);
    new_directory[path_length] = '\0';

    lfs_dir_t dir;
    int err = lfs_dir_open(&lfs, &dir, new_directory);
    if (err < 0) {
        uprintf("Error opening directory: %d\n", err);
        return module_ret_invalid_command;
    }

    lfs_dir_close(&lfs, &dir);

    // Update current directory
    strncpy(current_directory, new_directory, DIRECTORY_MAX - 1);
    current_directory[DIRECTORY_MAX - 1] = '\0';

    uprintf("Changed to directory: %s\n", new_directory);
    return module_ret_success;
}

static int parse_pwd(uint8_t *data, uint8_t length) {
    uprintf("Print working directory\n");

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[0] = module_ret_success;

    // Ensure current_directory is null-terminated
    current_directory[DIRECTORY_MAX - 1] = '\0';
    size_t dir_length = strlen(current_directory);

    if (dir_length > RAW_EPSIZE - 1) {
        dir_length = RAW_EPSIZE - 1;
    }

    memcpy(return_buf + 1, current_directory, dir_length);

    raw_hid_send(return_buf, RAW_EPSIZE);

    return module_ret_success;
}

static int parse_rm(uint8_t *data, uint8_t length) {
    uprintf("Remove file/directory\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH) {
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    int err = lfs_remove(&lfs, path);
    if (err < 0) {
        uprintf("Error removing file/directory: %d\n", err);
        return err;
    }

    return module_ret_success;
}

static int parse_mkdir(uint8_t *data, uint8_t length) {
    uprintf("Make directory\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH) {
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    int err = lfs_mkdir(&lfs, path);
    if (err < 0) {
        uprintf("Error creating directory: %d\n", err);
        return err;
    }

    return module_ret_success;
}

static int parse_touch(uint8_t *data, uint8_t length) {
    uprintf("Create empty file\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH) {
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    lfs_file_t file;
    int err = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT);
    if (err < 0) {
        uprintf("Error creating file: %d\n", err);
        return err;
    }

    err = lfs_file_close(&lfs, &file);
    if (err < 0) {
        uprintf("Error closing file: %d\n", err);
        return err;
    }

    return module_ret_success;
}

static int parse_cat(uint8_t *data, uint8_t length) {
    uprintf("Read file contents\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH) {
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    lfs_file_t file;
    int err = open_file(&lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        return err;
    }

    uint8_t return_buf[RAW_EPSIZE] = {0};
    lfs_ssize_t bytes_read;
    while ((bytes_read = lfs_file_read(&lfs, &file, return_buf + 1, RAW_EPSIZE - 1)) > 0) {
        uprintf("Read %d bytes from file %s\n", bytes_read, path);
        return_buf[0] = module_ret_success;
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

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH) {
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    int err = open_file(&lfs, &current_file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err < 0) {
        return err;
    }

    return module_ret_success;
}

static int parse_write(uint8_t *data, uint8_t length) {
    uprintf("Write to file\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("No data to write\n");
        return module_ret_invalid_command;
    }

    uint8_t *write_data = data + sizeof(struct packet_header);
    size_t data_length = length - sizeof(struct packet_header);

    lfs_ssize_t written = lfs_file_write(&lfs, &current_file, write_data, data_length);
    if (written < 0) {
        uprintf("Error writing to file: %d\n", written);
        return written;
    }

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[0] = module_ret_success;
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
    return_buf[0] = module_ret_success;
    uint32_t remaining = 128 - size;  // Adjust based on actual flash size
    memcpy(return_buf + 1, &remaining, sizeof(remaining));

    raw_hid_send(return_buf, RAW_EPSIZE);

    uprintf("Size: %li\n", remaining);
    return module_ret_success;
}

uint8_t current_image_map[(128 * 128) * LV_COLOR_DEPTH / 8];

lv_img_dsc_t current_image = {
    .header.always_zero = 0,
    .header.w = 128,
    .header.h = 128,
    .data_size = (128 * 128) * LV_COLOR_SIZE / 8,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = current_image_map,
};

static int parse_choose_image(uint8_t *data, uint8_t length) {
    uprintf("Choose image\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("Insufficient data length\n");
        return module_ret_invalid_command;
    }

    uint8_t *path_data = data + sizeof(struct packet_header);
    size_t path_length = length - sizeof(struct packet_header);

    char path[MAX_PATH_LENGTH];
    if (path_length >= MAX_PATH_LENGTH - 2) {  // Leave space for "L:" and null terminator
        uprintf("Path too long\n");
        return module_ret_invalid_command;
    }
    memcpy(path, path_data, path_length);
    path[path_length] = '\0';

    char full_path[MAX_PATH_LENGTH];
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
        uprintf("Warning: Read %ld bytes, expected %ld bytes\n", bytes_read, sizeof(current_image_map));
    }

    err = close_file(&lfs, &file);
    if (err < 0) {
        uprintf("Error closing image file: %d\n", err);
        return err;
    }

    lv_obj_t *img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &current_image);

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[0] = module_ret_success;
    raw_hid_send(return_buf, RAW_EPSIZE);
    return module_ret_success;
}

uint32_t image_write_pointer = 0;
static int parse_write_display(uint8_t *data, uint8_t length) {
    uprintf("Write to display\n");

    if (length <= sizeof(struct packet_header)) {
        uprintf("No data to write to display\n");
        return module_ret_invalid_command;
    }

    uint8_t *write_data = data + sizeof(struct packet_header);
    uint32_t bytes_to_write = length - sizeof(struct packet_header);

    // Ensure we don't exceed the buffer size
    if (image_write_pointer + bytes_to_write > sizeof(current_image_map)) {
        bytes_to_write = sizeof(current_image_map) - image_write_pointer;
    }

    memcpy(current_image_map + image_write_pointer, write_data, bytes_to_write);
    image_write_pointer += bytes_to_write;

    // Update the display if the image buffer is full
    if (image_write_pointer >= sizeof(current_image_map)) {
        image_write_pointer = 0;  // Reset the pointer

        lv_obj_t *img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &current_image);
    }

    uprintf("Wrote %u bytes to display buffer\n", bytes_to_write);

    uint8_t return_buf[RAW_EPSIZE] = {0};
    return_buf[0] = module_ret_success;
    raw_hid_send(return_buf, RAW_EPSIZE);

    return module_ret_success;
}

static int parse_set_time(uint8_t *data, uint8_t length) {
    uprintf("Set time\n");

    if (length < sizeof(struct packet_header) + 3) {
        uprintf("Insufficient data length for time\n");
        return module_ret_invalid_command;
    }

    uint8_t *time_data = data + sizeof(struct packet_header);

    uint8_t hour = time_data[0];
    uint8_t minute = time_data[1];
    uint8_t second = time_data[2];

    // Implement logic to set the system time
    // This might involve updating an RTC or internal time counter

    uprintf("Time set to: %02d:%02d:%02d\n", hour, minute, second);
    return module_ret_success;
}

int module_raw_hid_parse_packet(uint8_t *data, uint8_t length) {
    int err;
    uint8_t return_buf[RAW_EPSIZE] = {0};

    uprintf("Received packet. Parsing command.\r\n");

    if (length < 6 || length > RAW_EPSIZE) {  // Assuming header is 6 bytes
        uprintf("Invalid packet length\n");
        return -1;
    }

    // Manually parse header
    uint8_t magic_number = data[0];
    uint8_t command_id = data[1];
    uint32_t packet_id = *(uint32_t *)(data + 2);

    uprintf("Buffer contents: ");
    for (int i = 0; i < length; i++) {
        uprintf("%02X ", data[i]);
    }
    uprintf("\n");

    if (magic_number != 0x09) {
        uprintf("Invalid magic number: %02X\n", magic_number);
        return -1;
    }

    command_id -= id_module_cmd_base;
    uprintf("Command ID: %d\n", command_id);

    if (command_id >= (sizeof(parse_packet_funcs) / sizeof(parse_packet_funcs[0]))) {
        uprintf("Invalid command ID\n");
        return -1;
    }

    // Call the appropriate parsing function
    err = parse_packet_funcs[command_id](data, length);

    if (err < 0) {
        uprintf("Error parsing packet: %d\n", err);
        return_buf[0] = err;
    } else {
        return_buf[0] = module_ret_success;
    }

    raw_hid_send(return_buf, RAW_EPSIZE);

    return err;
}
