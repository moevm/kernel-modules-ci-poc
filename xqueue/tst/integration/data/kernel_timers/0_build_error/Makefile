obj-m += solution.o

export $(shell uname -r)=/lib/modules/$(shell uname -r)/build

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
kernel_clean:
	make -C /lid/modules/$(shell uname -r)/build M=$(PWD) clear

load:
	sudo insmod solution.ko
	sudo dmes -c
unload:
	sudo rmmod solution.ko
	sudo dmes -c


