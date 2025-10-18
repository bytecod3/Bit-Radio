/**
 * @author Edwin
 */

#ifndef BITRADIO_FIRMWARE_CONFIG_H
#define BITRADIO_FIRMWARE_CONFIG_H

#include "lvgl.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDR  0x3C
#define TFT_ROTATION LV_DISPLAY_ROTATION_0

/* set to 0 to disable debugging */
#define DEBUG (1)

/* enable or disable u8g2 graphics lib */
#define USE_U8G2 0

/* enable or disable lvgl graphics lib */
#define USE_LVGL 1

#endif //BITRADIO_FIRMWARE_CONFIG_H
