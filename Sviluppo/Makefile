obj-m += Fiber.o

KDIR = /lib/modules/$(shell uname -r)/build
 
 
all:
	make -C $(KDIR) M=$(shell pwd) modules
	gcc -C test_app.c -o test_app
	gcc -C test_switch.c -o test_switch
clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm test_app
mount:
	make $(shell insmod ./driver.ko)
unmount:
	make $(shell rmmod ./driver.ko)
log:
	make $(shell cat /var/log/kern.log | grep DEBUG)
