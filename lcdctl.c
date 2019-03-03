/*
	ledctl.c
	author jacob psimos
	december 27, 2018

	purpose: to provide a usermode method of interfacing with the
	lcd1602a driver.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "driver/lcd1602a.h"

#define ENABLE_CURSOR 0x01
#define ENABLE_BLINK 0x02

void printHelp(const char * const argv0){
	if(!argv0)
		return;
	printf("lcdctl - control lcd 1602a screen.\n");
	printf("Usage: %s [+/- option] -d /dev/lcd -1 'line 1 text' -2 'line 2 text'\n", argv0);
	printf("+/-c\tEnable or disable cursor underline.\n");
	printf("+/-b\tEnable or disable cursor blinking.\n");
	exit(0xFF);
}

int main(int argc, char **argv){

	int exitCode = 0;
	char *device = NULL;
	char *one = NULL;
	char *two = NULL;
	char cursorOn = -1;
	char blinkOn = -1;
	int ioctlResult = -1;
	int lcdfd = -1;
	int options = 0;
	int i;
	struct lcd1602a_cursor cursor;

	for(i = 0; i < argc; i++){
		if(argv[i][0] == '-'){
			if(argv[i][1] == 'c'){
				cursorOn = 0;
			}else if(argv[i][1] == 'b'){
				blinkOn = 0;
			}else if(argv[i][1] == 'h'){
				printHelp(argv[0]);
			}else if(argv[i][1] == 'd' && i < argc - 1){
				device = argv[i + 1];
			}else if(argv[i][1] == '1' && i < argc - 1){
				one = argv[i + 1];
			}else if(argv[i][1] == '2' && i < argc - 1){
				two = argv[i + 1];
			}
		}else if(argv[i][0] == '+'){
			if(argv[i][1] == 'c'){
				cursorOn = 1;
			}else if(argv[i][1] == 'b'){
				blinkOn = 1;
			}
		}
	}

	if(device == NULL){
		device = "/dev/lcd";
	}

	if((lcdfd = open(device, O_RDWR)) < 1){
		fprintf(stderr, "Could not open device descriptor.\n");
		exitCode = 1;
		goto program_done;
	}

	if(ioctlResult = ioctl(lcdfd, LCD1602A_SET_DEFAULTS, NULL)){
		exitCode = 2;
		goto program_done_close;
	}

	printf("cursor on: %d\nblink on: %d\n", cursorOn, blinkOn);

	//cursorOn = 0;

	if(cursorOn == 1){
		ioctlResult = ioctl(lcdfd, LCD1602A_CURSOR_ON, NULL);
	}else if(!cursorOn){
		ioctlResult = ioctl(lcdfd, LCD1602A_CURSOR_OFF, NULL);
	}

	if(ioctlResult){
		exitCode = 3;
		goto program_done_close;
	}

	//blinkOn = 0;

	if(blinkOn == 1){
		ioctlResult = ioctl(lcdfd, LCD1602A_CURSOR_BLINK, NULL);
	}else if(!blinkOn){
		ioctlResult = ioctl(lcdfd, LCD1602A_CURSOR_NOBLINK, NULL);
	}

	if(ioctlResult){
		exitCode = 4;
		goto program_done_close;
	}

	if(one != NULL){
		cursor.x = 0;
		cursor.y = 0;
		ioctl(lcdfd, LCD1602A_CURSOR_SET, &cursor);
		i = strlen(one);
		write(lcdfd, one, i);
	}

	if(two != NULL){
		cursor.x = 0;
		cursor.y = 1;
		ioctl(lcdfd, LCD1602A_CURSOR_SET, &cursor);
		i = strlen(two);
		write(lcdfd, two, i);
	}

	program_done_close:
	close(lcdfd);

	program_done:
	return exitCode;
}
