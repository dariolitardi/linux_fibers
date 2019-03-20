obj-m += Fiber.o

KDIR = /lib/modules/$(shell uname -r)/build
 
 
all:
	make -C $(KDIR) M=$(shell pwd) modules
	gcc -C test_app.c -o test_app
	gcc -C test_switch.c -o test_switch
	gcc -C test_multithread.c -pthread -o test_multithread
clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm test_app
	rm test_switch
	rm test_multithread
mount:
	make $(shell insmod ./driver.ko)
unmount:
	make $(shell rmmod ./driver.ko)
log:
	make $(shell cat /var/log/kern.log | grep DEBUG)
