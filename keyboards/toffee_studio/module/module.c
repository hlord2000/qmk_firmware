#if 1
#include <stdio.h>
#include <string.h>
#include "print.h"
#include "quantum.h"
#include "file_system.h"
#include "lfs.h"
#include "qp.h"
#include "qp_gc9107.h"
#include "graphics/thintel15.qff.c"
#include "platforms/chibios/gpio.h"
#include "qp_lvgl.h"
#include "lvgl.h"

#include "module.h"
#include "module_raw_hid.h"

#ifdef VIA_ENABLE

void board_init(void) {
    rp2040_mount_lfs(&lfs);

    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;
    debug_mouse    = true;
}

static painter_device_t      oled;
static painter_font_handle_t font;

#include "qp_comms.h"
#include "qp_gc9xxx_opcodes.h"

__attribute__((weak)) void ui_init(void) {
    oled = qp_gc9107_make_spi_device(128, 128, 0xFF, OLED_DC_PIN, 0xFF, 8, 0);
    font = qp_load_font_mem(font_thintel15);

    qp_init(oled, QP_ROTATION_180);

    #if 0
    qp_comms_start(oled);
    qp_comms_stop(oled);
    #endif

    qp_power(oled, true);

    if (qp_lvgl_attach(oled)) {
        volatile lv_fs_drv_t *result;

        result = lv_fs_littlefs_set_driver(LV_FS_LITTLEFS_LETTER, &lfs);
        if (result == NULL) {
            uprintf("Error mounting LFS");
        }

        lv_obj_t *background = lv_obj_create(lv_scr_act());
        lv_obj_set_size(background, 128, 128);
        lv_obj_set_style_bg_color(background, lv_color_hex(0xFF0000), 0);
    }
}

#ifdef QUANTUM_PAINTER_ENABLE
void keyboard_post_init_kb(void) {
    // Init the display
    setPinOutputPushPull(OLED_BL_PIN);
    writePinHigh(OLED_BL_PIN);
    ui_init();
    keyboard_post_init_user();
}
void housekeeping_task_kb(void) {
    // Draw the display
}
#endif //QUANTUM_PAINTER_ENABLE

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    int err;

    err = module_raw_hid_parse_packet(data, length);
    if (err < 0) {
        uprintf("Error parsing packet: %d\n", err);
    }
}
#endif
#endif


