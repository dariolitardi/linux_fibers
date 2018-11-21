obj-m += prova.o
 
KDIR = /lib/modules/$(shell uname -r)/build
 
 
all:
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean
mount:
	make $(shell insmod ./driver.ko)
unmount:
	make $(shell rmmod ./driver.ko)
log:
	make $(shell cat /var/log/kern.log | grep DEBUG)
