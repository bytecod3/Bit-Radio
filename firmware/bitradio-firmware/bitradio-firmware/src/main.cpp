#include <Arduino.h>
#include <Wire.h>
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

#include "lvgl.h"

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


/* ssd1306 functions */
void ssd1306_write_command(uint8_t cmd) {
    Wire.beginTransmission(SCREEN_ADDR);
    /* control byte: 0, D/C# 0*/
    Wire.write(0x00);
    Wire.write(cmd);
    Wire.endTransmission();

}
void ssd1306_write_data(uint8_t* data, size_t size) {
    Wire.beginTransmission(SCREEN_ADDR);
    Wire.write(0x40);
    for(size_t i = 0; i < size; i++) {
        Wire.write(data[i]);
    }

    Wire.endTransmission();
}

void ssd1306_set_page(uint8_t page) {
    /* set page address (0 - 7 */
    ssd1306_write_command(0xB0 | page);
}

void ssd1306_set_column(uint8_t col) {
    ssd1306_write_command(0x00 | (col & 0x0F)); // lo byte
    ssd1306_write_command(0x10 | ((col >> 4) & 0x0F)); // hi byte
}

void ssd1306_init() {
    Wire.begin(21, 22);
    delay(100);

    ssd1306_write_command(0xAE); // Display off
    ssd1306_write_command(0xD5); ssd1306_write_command(0x80); // Set display clock divide ratio/oscillator
    ssd1306_write_command(0xA8); ssd1306_write_command(0x3F); // Multiplex ratio (1 to 64)
    ssd1306_write_command(0xD3); ssd1306_write_command(0x00); // Display offset
    ssd1306_write_command(0x40); // Start line address = 0
    ssd1306_write_command(0x8D); ssd1306_write_command(0x14); // Enable charge pump
    ssd1306_write_command(0x20); ssd1306_write_command(0x00); // Horizontal addressing mode
    ssd1306_write_command(0xA1); // Segment re-map (column address 127 mapped to SEG0)
    ssd1306_write_command(0xC8); // COM output scan direction: remapped
    ssd1306_write_command(0xDA); ssd1306_write_command(0x12); // COM pins config
    ssd1306_write_command(0x81); ssd1306_write_command(0x7F); // Contrast control
    ssd1306_write_command(0xA4); // Resume from display RAM
    ssd1306_write_command(0xA6); // Normal display (not inverted)
    ssd1306_write_command(0x2E); // Deactivate scroll
    ssd1306_write_command(0xAF); // Display ON
}

void ssd1306_clear() {
    uint8_t zero[128] = {0};
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_write_command(0xB0 | page);  // Set page address
        ssd1306_write_command(0x00);         // Set lower column address
        ssd1306_write_command(0x10);         // Set higher column address
        ssd1306_write_data(zero, 128);
    }
}

const uint8_t font5x7[][5] = {
        // Basic ASCII 32–127, you can fill in more if you want
        {0x00,0x00,0x00,0x00,0x00}, // (space)
        {0x00,0x00,0x5F,0x00,0x00}, // !
        {0x00,0x07,0x00,0x07,0x00}, // "
        // ...
        {0x7E,0x11,0x11,0x11,0x7E}, // A (0x41)
        {0x7F,0x49,0x49,0x49,0x36}, // B
        {0x3E,0x41,0x41,0x41,0x22}, // C
        // ...
};

/* Draw a character at (x, page) */
void ssd1306_draw_char(uint8_t x, uint8_t page, char c) {
    if (c < 32 || c > 126) c = ' ';
    ssd1306_write_command(0xB0 | page);
    ssd1306_write_command(0x00 | (x & 0x0F));
    ssd1306_write_command(0x10 | (x >> 4));

    ssd1306_write_data((uint8_t*)font5x7[c - 32], 5);
    uint8_t space = 0x00;
    ssd1306_write_data(&space, 1);  // 1-pixel spacing
}

void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str) {
    while (*str) {
        ssd1306_draw_char(x, page, *str++);
        x += 6;
        if (x + 6 >= SCREEN_WIDTH) break; // stop if line full
    }
}

/* lvgl settings */
/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
//#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 8)
//uint8_t draw_buf[DRAW_BUF_SIZE / 4];
static uint8_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 8];

lv_display_t * disp;

/* LVGL calls it when a rendered image needs to copied to the display*/
void my_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {

    uint8_t row1 = area->y1 >> 3;
    uint8_t row2 = area->y2 >> 3;

    for (uint8_t page = row1; page <= row2; page++) {
        ssd1306_write_command(0xB0 | page);
        ssd1306_write_command(0x00 | (area->x1 & 0x0F));
        ssd1306_write_command(0x10 | ((area->x1 >> 4) & 0x0F));

        for (uint16_t x = area->x1; x <= area->x2; x++) {
            uint8_t data = 0;

            // Construct one vertical byte (8 pixels)
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint16_t y = (page << 3) + bit;
                if (y > area->y2) break;

                uint16_t index = y * SCREEN_WIDTH + x;
                uint8_t pixel = px_map[index >> 3] & (1 << (index & 7));

                if (pixel) data |= (1 << bit);
            }

            ssd1306_write_data(&data, 1);
        }
    }
    lv_display_flush_ready(disp);
}

static uint32_t my_tick(void) {
    return millis();
}

/* function prototypes */
void lv_port_disp_init() {
    ssd1306_init();

    disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, my_disp_flush);
}

void setup() {
//    lv_init();
//    lv_port_disp_init();
//
//    /* create a dummy label */
//    lv_obj_t* label = lv_label_create(lv_screen_active());
//    lv_label_set_text(label, "Hello arduino. Im lvgl!");
//    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    Wire.begin(21, 22);
    ssd1306_clear();
    ssd1306_draw_string(0, 1, "SSD1306 OLED");
}

unsigned long last_tick = 0;
void loop() {
//    unsigned long now = millis();
//    if(now - last_tick >= 5) {
//        lv_tick_inc(5);
//        last_tick = now;
//    }
//
//    lv_timer_handler();
//    delay(1);

}


