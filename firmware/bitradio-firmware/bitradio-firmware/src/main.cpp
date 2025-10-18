#include <Arduino.h>
#include "config.h"

#if USE_U8G2
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "bitmaps.h"
#endif

const char* firmware_version = "v1.0";

#if DEBUG
static const char* debug_tag = "BIT_RADIO";
#endif

#if USE_U8G2
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
        U8G2_R0,
        U8X8_PIN_NONE
);
#endif

void draw_xbm_inverted(U8G2* disp, int x, int y, int w, int h, const uint8_t* bitmap) {
    disp->setDrawColor(0);
    disp->drawXBM(x, y, w, h, bitmap);
    disp->setDrawColor(1);
}

void setup() {
    display.begin();
    display.setDrawColor(0);
}

void loop() {

    display.clearBuffer();
    display.firstPage();
    do {
        draw_xbm_inverted(&display, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, splash_screen);
    } while (display.nextPage());

}

