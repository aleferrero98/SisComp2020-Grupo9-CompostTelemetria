obj-m:=raspin.o
#GPIO4 pin del sensor DHT11
DHT11=4

raspin.ko: raspin.c
	make -C /usr/src/linux M=`pwd` V=1 modules

clean:
	make -C /usr/src/linux M=`pwd` V=1 clean

install: raspin.ko
	sudo insmod raspin.ko
	sudo chmod 666 /dev/raspin$(DHT11)

uninstall:
	sudo rmmod raspin
