obj-m:=raspin.o

raspin.ko: raspin.c
	make -C /usr/src/linux M=`pwd` V=1 modules

clean:
	make -C /usr/src/linux M=`pwd` V=1 clean

install: raspin.ko
	sudo insmod raspin.ko
	sudo chmod 666 /dev/raspin0

uninstall:
	sudo rmmod raspin
