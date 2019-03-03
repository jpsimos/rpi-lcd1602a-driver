#ifndef __LCD1602A_H
#define __LCD1602A_H

/*
    lcd1602a.h
    author jacob psimos
    december 28, 2018
*/

#define LCD1602A_CURSOR_BLINK 0x1000
#define LCD1602A_CURSOR_NOBLINK 0x2000
#define LCD1602A_CURSOR_ON 0x3000
#define LCD1602A_CURSOR_OFF 0x4000
#define LCD1602A_LEFT_TO_RIGHT 0x5000
#define LCD1602A_RIGHT_TO_LEFT 0x6000
#define LCD1602A_AUTOSCROLL_ON 0x7000
#define LCD1602A_AUTOSCROLL_OFF 0x8000
#define LCD1602A_DEFINE_CHARACTERS 0x9000
#define LCD1602A_CLEAR 0x10000
#define LCD1602A_SET_DEFAULTS 0x20000
#define LCD1602A_DISPLAY_ON 0x30000
#define LCD1602A_DISPLAY_OFF 0x40000
#define LCD1602A_CURSOR_SET 0x50000
#define LCD1602A_CURSOR_SET_LINE 0x60000
#define LCD1602A_HOME 0x70000

#define LCD_COMMAND 0
#define LCD_DATA 1

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

#ifndef __KERNEL__
	typedef uint8_t cursor_t;
	typedef uint8_t bitmask8_t;
	typedef uint8_t charbits_t;
#else
	typedef u8 cursor_t;
	typedef u8 bitmask8_t;
	typedef u8 charbits_t;
#endif

struct lcd1602a_cursor{
	cursor_t x;
	cursor_t y;
	loff_t offset;
};

struct lcd1602a_characters{
    bitmask8_t mask;
    charbits_t matrix[8][8];
};

#endif
