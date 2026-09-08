obj-m := my_mod.o

run:
	sudo insmod my_mod.ko
	sudo dmesg
	sudo rmmod my_mod.ko

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean