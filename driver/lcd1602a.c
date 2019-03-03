/*
	lcd1602a.c
	author jacob psimos
	december 27, 2018

	Used References:
	https://github.com/arduino-libraries/LiquidCrystal/blob/master/src/LiquidCrystal.cpp
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include "lcd1602a.h"
#include "lcd1602a-params.h"

//#define GPIO_BASE 0x3F000000
#define GPSET0 0x1C
#define GPCLR0 0x28
#define GPLEV0 0x34

static volatile u8 busy = 0;
static u8 pins[11];
static char text[2][17];
static int line0_length;
static int line1_length;
static u8 display_function;
static u8 display_mode;
static u8 display_control;
static u8 display_mode;
static struct lcd1602a_characters custom_characters;
static struct lcd1602a_cursor cursor;

static int lcd1602a_setup(void);
static int lcd1602a_defaults(void);
static int lcd1602a_write(const u8 mode, const u8 value);
static int lcd1602a_pulse(void);
static loff_t lcd1602a_set_cursor(const u8 x, const u8 y);
static loff_t lcd1602a_dev_llseek(struct file *lcd_file, loff_t offset, int flags);
static ssize_t lcd1602a_dev_read(struct file *lcd_file, char __user *ptr, size_t nmemb, loff_t *offset);
static ssize_t lcd1602a_dev_write(struct file *lcd_file, const char __user *ptr, size_t nmemb, loff_t *offset);
static long lcd1602a_dev_ioctl(struct file *lcd_file, unsigned int cmd, unsigned long arg);
static int lcd1602a_dev_open(struct inode *lcd_node, struct file *lcd_file);
static int lcd1602a_dev_close(struct inode *lcd_node, struct file *lcd_file);

static int lcd1602a_dev_major;
static u8 lcd1602a_dev_is_open;

static const struct file_operations lcd1602a_fopts = {
	.owner = THIS_MODULE,
	.llseek = lcd1602a_dev_llseek,
	.read = lcd1602a_dev_read,
	.write = lcd1602a_dev_write,
	.unlocked_ioctl = lcd1602a_dev_ioctl,
	.open = lcd1602a_dev_open,
	.release = lcd1602a_dev_close,
};

static struct miscdevice lcd1602a_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lcd",
	.fops = &lcd1602a_fopts,
	.mode = 0666,
};

static int __init lcd1602a_driver_init(void){
	//lcd1602a_dev_major = register_chrdev(1, "lcd1602a", &lcd1602a_fops);
	int err;

	err = misc_register(&lcd1602a_dev);

	if(lcd1602a_setup()){
		return -1;
	}

	if(lcd1602a_dev_major < 0){
		printk(KERN_ALERT "Unable to register character device for lcd1602a.\n");
		return -1;
	}

	return 0;
}

static void __exit lcd1602a_driver_unload(void){
	int i;

	if(lcd1602a_dev_is_open){
		printk(KERN_NOTICE "Device is still open.\n");
	}

	//unregister_chrdev(lcd1602a_dev_major, "lcd1602a");

	for(i = 0; i < 11; i++){
		gpio_free(pins[i]);
	}

	misc_deregister(&lcd1602a_dev);

}

static int lcd1602a_setup(void){

	int i;
	int e;

	const char *labels[11] = {
		"lcd_rs", "lcd_rw", "lcd_enable", "lcd_db0", "lcd_db1",
		"lcd_db2", "lcd_db3", "lcd_db4", "lcd_db5", "lcd_db6", "lcd_db7"
	};

	pins[0] = PIN_RS;
	pins[1] = PIN_RW;
	pins[2] = PIN_E;
	pins[3] = PIN_DB0;
	pins[4] = PIN_DB1;
	pins[5] = PIN_DB2;
	pins[6] = PIN_DB3;
	pins[7] = PIN_DB4;
	pins[8] = PIN_DB5;
	pins[9] = PIN_DB6;
	pins[10] = PIN_DB7;

	for(i = 0; i < 11; i++){
		if(gpio_request(pins[i], labels[i])){
			for(e = 0; e < i; e++){
				gpio_free(pins[e]);
			}
			printk(KERN_ALERT "Invalid %s(%u)\n", labels[i], pins[i]);
			return -ENOTSUPP;
		}else{
			gpio_direction_output(pins[i], 0);
		}
	}

	lcd1602a_defaults();

	return 0;
}

static int lcd1602a_defaults(void){
	static u8 init = 0;

	cursor.offset = 0L;
	cursor.x = 0;
	cursor.y = 0;
	display_function = LCD_2LINE | LCD_8BITMODE | LCD_5x8DOTS;
	display_control = LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON;
	display_mode = LCD_ENTRYLEFT;

	memset(text[0], '\0', 17);
	memset(text[1], '\0', 17);
	line0_length = 0;
	line1_length = 0;

	if(!init){
		lcd1602a_write(LCD_COMMAND, LCD_FUNCTIONSET | display_function);
		usleep_range(4480, 4700);

		lcd1602a_write(LCD_COMMAND, LCD_FUNCTIONSET | display_function);
		usleep_range(140, 160);

		lcd1602a_write(LCD_COMMAND, LCD_FUNCTIONSET | display_function);
		init = 1;
	}

	lcd1602a_write(LCD_COMMAND, LCD_DISPLAYCONTROL | display_control);

	lcd1602a_write(LCD_COMMAND, LCD_ENTRYMODESET | display_mode);

	lcd1602a_write(LCD_COMMAND, LCD_CLEARDISPLAY);
	usleep_range(2000, 2200);

	lcd1602a_write(LCD_COMMAND, LCD_RETURNHOME);

	usleep_range(2000, 2200);
	return 0;
}

static int lcd1602a_write(const u8 mode, const u8 value){
	int i;
	int val;

	if(busy){
		printk(KERN_NOTICE "LCD Busy.\n");
		return -1;
	}

	busy = 1;

	gpio_set_value(PIN_RS, mode);
	gpio_set_value(PIN_RW, 0);

	for(i = 0; i < 8; i++){
		//gpio_set_value(pins[3 + i], (value & (u8)(1 << i)) ? 1 : 0);
		val = (value >> i) & 1;
		gpio_set_value(pins[3 + i], (value >> i) & 1);
	}

	lcd1602a_pulse();

	for(i = 0; i < 8; i++){
		gpio_set_value(pins[3 + i], 0);
	}

	busy = 0;

	return 0;
}


static int lcd1602a_pulse(void){
	gpio_set_value(PIN_E, 0);
	usleep_range(1, 10);
	gpio_set_value(PIN_E, 1);
	usleep_range(1, 10);
	gpio_set_value(PIN_E, 0);
	usleep_range(100, 200);

	return 0;
}

static loff_t lcd1602a_set_cursor(const u8 x, const u8 y){
	u32 offset;

	if(x > 15 || y > 1){
		return -ENOTSUPP;
	}

	offset = y == 0 ? 0 : 64;
	cursor.offset = (loff_t)((u32)x + offset);
	cursor.x = x;
	cursor.y = y;

	lcd1602a_write(LCD_COMMAND, LCD_SETDDRAMADDR | (u8)cursor.offset);

	return cursor.offset;
}

static loff_t lcd1602a_dev_llseek(struct file *lcd_file, loff_t offset, int orig){
	/*
	loff_t new_offset = 0;

	if(offset < 0 || offset > 31){
		return -1;
	}

	switch(orig){
		case SEEK_SET:
			cursor.offset = offset;
		break;
		case SEEK_CUR:
			new_offset = cursor.offset + offset;
			if(new_offset >= 0 && new_offset <= 31){
				cursor.offset = new_offset;
				if((u8)new_offset >= 16){
					cursor.x = (u8)new_offset - 16;
					cursor.y = 1;
				}else{
					cursor.x = (u8)new_offset;
					cursor.y = 0;
				}
			}
		break;
		case SEEK_END:
			cursor.offset = 31;
			cursor.y = 1;
			cursor.x = 15;
		break;
	}

	lcd_file->f_pos = cursor.offset;
	*/

	return offset;
}

static ssize_t lcd1602a_dev_read(struct file *lcd_file, char __user *ptr, size_t nmemb, loff_t *offset){
	return -ENOSPC;
}

static ssize_t lcd1602a_dev_write(struct file *lcd_file, const char __user *ptr, size_t nmemb, loff_t *offset){
	char buffer[17];
	char next;
	char prev;
	int line0_length = strlen(text[0]);
	int line1_length = strlen(text[1]);
	ssize_t i;

	if(busy){
		return -1;
	}

	if(nmemb > 15){
		return -1;
	}

	memset(buffer, '\0', sizeof(buffer));

	if(copy_from_user(buffer, ptr, nmemb)){
		return -1;
	}

	if(cursor.y == 0){
		i = 0;
		//cursor.x = line0_length > 0 ? (u8)line0_length - 1 : 0;
		while(i < 16 && cursor.x < 16){
			next = buffer[i++];
			prev = text[0][cursor.x];
			if(next != prev){
				if(isprint(next)){
					text[0][cursor.x] = next;
				}else if(next == '\0'){
					text[0][cursor.x] = '\0';
					break;
				}else{
					text[0][cursor.x] = ' ';
				}
				lcd1602a_write(LCD_DATA, text[0][cursor.x]);
			}
			cursor.x = cursor.x + 1;
			if(~display_mode & LCD_ENTRYSHIFTDECREMENT){
				//lcd1602a_set_cursor(cursor.x, cursor.y);				
			}
		}
	}else if(cursor.y == 1){
		i = 0;
		//cursor.x = line1_length > 0 ? (u8)line1_length - 1 : 0;
		lcd1602a_set_cursor(cursor.x, cursor.y);
		while(i < 16 && cursor.x < 16){
			next = buffer[i++];
			prev = text[1][cursor.x];
			if(next != prev){
				if(isprint(next)){
					text[1][cursor.x] = next;
				}else if(next == '\0'){
					text[1][cursor.x] = '\0';
					break;
				}else{
					text[1][cursor.x] = ' ';
				}
				lcd1602a_write(LCD_DATA, text[1][cursor.x]);
			}
			cursor.x = cursor.x + 1;
			if(~display_mode & LCD_ENTRYSHIFTDECREMENT){
				//lcd1602a_set_cursor(cursor.x, cursor.y);
			}
		}
	}

	return cursor.y == 0 ? line0_length : line1_length;
}

static long lcd1602a_dev_ioctl(struct file *lcd_file, unsigned int cmd, unsigned long arg){
	struct lcd1602a_characters __user *ptr_characters = NULL;
	struct lcd1602a_cursor __user *ptr_cursor = NULL;
	int j = 0;
	int k = 0;
	u8 update_displaymode = 0;
	u8 update_displaycontrol = 0;
	long result = 0L;

	switch(cmd){
		case LCD1602A_HOME:
			lcd1602a_write(LCD_COMMAND, LCD_RETURNHOME);
			usleep_range(2000, 2100);
		break;
		case LCD1602A_CURSOR_SET:
			ptr_cursor = (struct lcd1602a_cursor __user*)arg;
			if(ptr_cursor && !copy_from_user(&cursor, ptr_cursor, sizeof(cursor))){
				lcd1602a_set_cursor(cursor.x, cursor.y);
			}else{
				result = -1L;
			}
		break;
		case LCD1602A_CURSOR_BLINK:
			display_control |= LCD_BLINKON;
			update_displaycontrol = 1;
		break;
		case LCD1602A_CURSOR_NOBLINK:
			display_control &= ~LCD_BLINKON;
			update_displaycontrol = 1;
		break;
		case LCD1602A_CURSOR_ON:
			display_control |= LCD_CURSORON;
			update_displaycontrol = 1;
		break;
		case LCD1602A_CURSOR_OFF:
			display_control &= ~LCD_CURSORON;
			update_displaycontrol = 1;
		break;
		case LCD1602A_LEFT_TO_RIGHT:
			display_mode |= LCD_ENTRYLEFT;
			update_displaymode = 1;
		break;
		case LCD1602A_RIGHT_TO_LEFT:
			display_mode &= ~LCD_ENTRYLEFT;
			update_displaymode = 1;
		break;
		case LCD1602A_AUTOSCROLL_ON:
			display_mode |= LCD_ENTRYSHIFTINCREMENT;
			update_displaymode = 1;
		break;
		case LCD1602A_AUTOSCROLL_OFF:
			display_mode &= ~LCD_ENTRYSHIFTINCREMENT;
			update_displaymode = 1;
		break;
		case LCD1602A_DEFINE_CHARACTERS:
			ptr_characters = (struct lcd1602a_characters __user*)arg;
			if(!copy_from_user(&custom_characters, ptr_characters, sizeof(custom_characters))){
				for(j = 0; j < 8; j++){
					if((custom_characters.mask >> j) & 1){
						lcd1602a_write(LCD_COMMAND, LCD_SETCGRAMADDR | (j << 3));
						for(k = 0; k < 8; k++){
							lcd1602a_write(LCD_DATA, custom_characters.matrix[j][k]);
						}
					}
				}
			}else{
				result = -1;
			}
		break;
		case LCD1602A_CLEAR:
			lcd1602a_write(LCD_COMMAND, LCD_CLEARDISPLAY);
			usleep_range(2000, 3000);
		break;
		case LCD1602A_SET_DEFAULTS:
			lcd1602a_defaults();
		break;
		case LCD1602A_DISPLAY_OFF:
			display_control &= ~LCD_DISPLAYON;
			lcd1602a_write(LCD_COMMAND, LCD_DISPLAYCONTROL | display_control);
		break;
		case LCD1602A_DISPLAY_ON:
			display_control |= LCD_DISPLAYON;
			lcd1602a_write(LCD_COMMAND, LCD_DISPLAYCONTROL | display_control);
		break;
		default:
			result = -ENOTSUPP;
		break;
	}

	if(update_displaycontrol){
		lcd1602a_write(LCD_COMMAND, LCD_DISPLAYCONTROL | display_control);
	}

	if(update_displaymode){
		lcd1602a_write(LCD_COMMAND, LCD_ENTRYMODESET | display_mode);
	}

	return result;
}

static int lcd1602a_dev_open(struct inode *lcd_node, struct file *lcd_file){
    if(lcd1602a_dev_is_open){
        return -EBUSY;
    }

    lcd1602a_dev_is_open = 1;

    return 0;
}

static int lcd1602a_dev_close(struct inode *lcd_node, struct file *lcd_file){
    lcd1602a_dev_is_open = 0;
    return 0;
}

module_init(lcd1602a_driver_init);
module_exit(lcd1602a_driver_unload);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jacob Psimos");
MODULE_DESCRIPTION("A driver to control a 1602a LCD screen");

