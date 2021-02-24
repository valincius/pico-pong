#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0

void init_display();
void show_scr();
void fill_scr(uint8_t v);
void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
void ssd1306_print(const char *str);
void set_cursor(int x = 0, int y = 0);