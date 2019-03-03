#Makefile for lcdctl program
#Author Jacob Psimos 2018 - 2019

all:
	$(CC) -o lcdctl lcdctl.c --std=gnu99 -O3 -fstack-protector-all

install:
	[ ! -d /usr/local/bin ] && mkdir -p /usr/local/bin || /bin/true
	[ -f lcdctl ] && cp lcdctl /usr/local/bin/lcdctl

uninstall:
	[ -f /usr/local/bin/lcdctl ] && rm /usr/local/bin/lcdctl

clean:
	[ -f lcdctl ] && rm lcdctl || /bin/true
