ifneq ($(KERNELRELEASE),)
	obj-m := my_md.o

else
	CURRENT = $(shell uname -r)
	KDIR = /lib/modules/$(CURRENT)/build 
	PWD = $(shell pwd)

default:
	echo $(MAKE) -C $(KDIR) M=$(PWD) modules
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	make clean

clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order
	@rm -f .*.*.cmd *~ *.*~ TODO.*
	@rm -fR .tmp*
	@rm -rf .tmp_versions

disclean: clean
	@rm *.ko *.symvers

show:
	lsmod

proc-mount:
	sudo cat /proc/slabinfo

ls:
	ls -ial

cat:
	cat /proc/mounts

slabinfo:
	sudo cat /proc/slabinfo
	
msg:
	sudo dmesg | grep -- '--'

reg:
	sudo cat /proc/filesystems

mount:
	sudo mount -t my_vfs -o loop image ./dir1
	sudo mount -t my_vfs -o loop image ./dir2

mount1:
	sudo mount -t my_vfs -o loop image ./dir1

unmount:
	sudo umount ./dir1
	sudo umount ./dir2

insmod:
	sudo insmod my_md.ko

rmmod:	
	sudo rmmod my_md

endif