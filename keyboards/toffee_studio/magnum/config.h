// Copyright 2024 Kelly Helmut Lord (@Kelly Helmut Lord)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define DYNAMIC_KEYMAP_LAYER_COUNT 8

#define QUANTUM_PAINTER_DISPLAY_TIMEOUT 0

// SPI Configuration
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP2
#define SPI_MOSI_PIN GP3

// Display Configuration
#define OLED_DC_PIN GP1
#define OLED_BL_PIN GP0

// LVGL Configuration
#define QP_LVGL_TASK_PERIOD 40
