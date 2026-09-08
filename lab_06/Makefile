CONFIG_MODULE_SIG=n

CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

obj-m := my_md1.o my_md2.o my_md3.o

.PHONY: run1 run2 error1 error2

default: 
				make -C $(KDIR) M=$(PWD) modules

run1:
			- sudo insmod my_md1.ko
			- sudo insmod my_md2.ko
			- sudo rmmod  my_md2
			- sudo rmmod  my_md1
			- sudo dmesg | grep +

run2:
				- sudo insmod my_md1.ko
				- sudo insmod my_md3.ko
				- sudo rmmod  my_md3
				- sudo rmmod  my_md1
				- sudo dmesg | grep +

error1:
				- sudo insmod my_md2.ko

error2:
				- sudo insmod my_md1.ko
				- sudo insmod my_md2.ko
				- sudo rmmod my_md1.ko


clean: 
				@rm -f *.o .*.cmd .*.flags *.mod.c *.order *.mod *.ko *.symvers 
				@rm -f .*.*.cmd *~ *.*~ TODO.* .*.d
				@rm -fR .tmp* 
				@rm -rf .tmp_versions 
disclean: clean 
				@rm *.ko *.symvers
