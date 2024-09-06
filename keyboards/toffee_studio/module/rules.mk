LITTLEFS_ENABLE = yes
ALLOW_WARNINGS = yes
CONSOLE_ENABLE = yes

QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7735_spi
QUANTUM_PAINTER_LVGL_INTEGRATION = yes

SRC += rawhid/module_raw_hid.c

VPATH += keyboards/toffee_studio/module/rawhid
