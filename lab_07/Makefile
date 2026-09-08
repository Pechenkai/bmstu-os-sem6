ifneq ($(KERNELRELEASE),)
obj-m := my_md.o
else

CURRENT := $(shell uname -r)
KDIR := /lib/modules/$(CURRENT)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd .*.flags *mщd.c *.order
	rm -f .*.*.cmd *~ *.*~ TODO.*
	rm -fR .tmp*
	rm -rf .tmp_versions

disclean: clean
	rm -f *.ko *.symvers

run:
	- sudo rmmod my_md
	- sudo insmod my_md.ko
	- echo 32 | sudo tee /proc/symlinkinf
	- sudo cat /proc/symlinkinf

endif