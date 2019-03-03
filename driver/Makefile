#lcd1602a driver makefile
#author jacob psimos 2018 - 2019

obj-m += lcd1602a.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
