ifneq ($(KERNELRELEASE),)
	obj-m := ip.o 
else
	KERNELDIR ?= /home/socuser/Desktop/zynq/linux-xlnx
default:
	make -C /home/socuser/Desktop/zynq/linux-xlnx M=$(PWD) modules
	##$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	make -C /home/socuser/Desktop/zynq/linux-xlnx M=$(PWD) clean

