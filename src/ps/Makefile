TARGET = hideps

KDIR =/usr/src/linux-headers-$(shell uname -r)

PWD := $(shell pwd)

obj-m :=$(TARGET).o

default: 
	make -C $(KDIR) M=$(PWD) modules

#清除，删除源码的目标文件
clean: 
	rm -rf *.o
#安装内核模块
install: 
	sudo insmod $(TARGET).ko
#显示内核中的该模块
lsmod: 
	sudo lsmod|grep $(TARGET)
#卸载该模块
uninstall: 
	sudo rmmod $(TARGET)