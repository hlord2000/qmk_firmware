OPT_DEFS += -DLITTLEFS_ENABLE

SRC += $(LIB_PATH)/littlefs/lfs.c
SRC += $(LIB_PATH)/littlefs/lfs_util.c
COMMON_VPATH += $(LIB_PATH)/littlefs

SRC += $(PLATFORM_PATH)/$(PLATFORM_KEY)/$(DRIVER_DIR)/littlefs/file_system.c
COMMON_VPATH += $(PLATFORM_PATH)/$(PLATFORM_KEY)/$(DRIVER_DIR)/littlefs
