#include "display.h"

#include "font.h"

#define SET_CONTRAST 0x81
#define SET_ENTIRE_ON 0xA4
#define SET_NORM_INV 0xA6
#define SET_DISP 0xAE
#define SET_MEM_ADDR 0x20
#define SET_COL_ADDR 0x21
#define SET_PAGE_ADDR 0x22
#define SET_DISP_START_LINE 0x40
#define SET_SEG_REMAP 0xA0
#define SET_MUX_RATIO 0xA8
#define SET_COM_OUT_DIR 0xC0
#define SET_DISP_OFFSET 0xD3
#define SET_COM_PIN_CFG 0xDA
#define SET_DISP_CLK_DIV 0xD5
#define SET_PRECHARGE 0xD9
#define SET_VCOM_DESEL 0xDB
#define SET_CHARGE_PUMP 0x8D

const uint8_t height = 64;
const uint8_t SID = (height == 64) ? 0x3C : 0x3D; // different height displays have different addr
const uint8_t width = 128;
const int page_size = 8;
const int pages = height / page_size;
const bool external_vcc = false;

uint8_t scr[pages*width+1]; // extra byte holds data send instruction

void write_cmd(uint8_t cmd)  {
    uint8_t buf[] = { 0x80, cmd };
	i2c_write_blocking(I2C_PORT, SID, buf, sizeof(buf), false);
}

void fill_scr(uint8_t v) {
	memset(scr, v, sizeof(scr));
}

void show_scr() {
	write_cmd(SET_MEM_ADDR); // 0x20
	write_cmd(0b01); // vertical addressing mode

	write_cmd(SET_COL_ADDR); // 0x21
	write_cmd(0);
	write_cmd(127);

	write_cmd(SET_PAGE_ADDR); // 0x22
	write_cmd(0);
	write_cmd(pages-1);

	scr[0] = 0x40; // the data instruction	
	i2c_write_blocking(I2C_PORT, SID, scr, sizeof(scr), false);
}

void init_display() {
	uint8_t cmds[] = {
		SET_DISP | 0x00,  // display off 0x0E | 0x00

		SET_MEM_ADDR, // 0x20
		0x00,  // horizontal

		//# resolution and layout
		SET_DISP_START_LINE | 0x00, // 0x40
		SET_SEG_REMAP | 0x01,  //# column addr 127 mapped to SEG0

		SET_MUX_RATIO, // 0xA8
		height - 1,

		SET_COM_OUT_DIR | 0x08,  //# scan from COM[N] to COM0  (0xC0 | val)
		SET_DISP_OFFSET, // 0xD3
		0x00,

		//SET_COM_PIN_CFG, // 0xDA
		//0x02 if self.width > 2 * self.height else 0x12,
		//width > 2*height ? 0x02 : 0x12,
		//SET_COM_PIN_CFG, height == 32 ? 0x02 : 0x12,

		//# timing and driving scheme
		SET_DISP_CLK_DIV, // 0xD5
		0x80,

		SET_PRECHARGE, // 0xD9
		//0x22 if self.external_vcc else 0xF1,
		external_vcc ? 0x22 : 0xF1,

		SET_VCOM_DESEL, // 0xDB
		//0x30,  //# 0.83*Vcc
		0x40, // changed by mcarter

		//# display
		SET_CONTRAST, // 0x81
		0xFF,  //# maximum

		SET_ENTIRE_ON,  //# output follows RAM contents // 0xA4
		SET_NORM_INV,  //# not inverted 0xA6

		SET_CHARGE_PUMP, // 0x8D
		//0x10 if self.external_vcc else 0x14,
		external_vcc ? 0x10 : 0x14,

		SET_DISP | 0x01
	};

	// write all the commands
	for(int i=0; i<sizeof(cmds); i++)
		write_cmd(cmds[i]);

	fill_scr(0);
	show_scr();
}

void draw_pixel(int16_t x, int16_t y, int color)  {
	if(x<0 || x >= width || y<0 || y>= height) return;

	int page = y/page_size;
	int bit = 1<<(y % page_size);
	uint8_t *pixel = &scr[x*page_size + page + 1];

	switch (color) {
		case 1: // white
			*pixel |= bit;
			break;
		case 0: // black
			*pixel &= ~bit;
			break;
		case -1: //inverse
			*pixel ^= bit;
			break;
	}

}

void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        byte <<= 1;
      else
        byte = bitmap[j * byteWidth + i / 8];
      if (byte & 0x80)
        draw_pixel(x + i, y, color);
    }
  }
}

void draw_letter_at(uint8_t x, uint8_t y, char c) {
	if(c< ' ' || c>  0x7F) c = '?'; // 0x7F is the DEL key

	int offset = 4 + (c - ' ' )*6;
	for(int col = 0 ; col < 6; col++) {
		uint8_t line =  ssd1306_font6x8[offset+col];
		for(int row =0; row <8; row++) {
			draw_pixel(x+col, y+row, line & 1);
			line >>= 1;
		}
	}

	for(int row = 0; row<8; row++) {
		draw_pixel(x+6, y+row, 0);
		draw_pixel(x+7, y+row, 0);
	}
}

int cursorx = 0;
int cursory = 0;
void ssd1306_print(const char *str) {
	char c;
	while(c = *str) {
		str++;
		if(c == '\n') {
			cursorx = 0;
			cursory += 8;
			continue;
		}
		draw_letter_at(cursorx, cursory, c);
		cursorx += 8;
	}
}

void set_cursor(int x, int y) {
    cursorx = x;
    cursory = y;
}


