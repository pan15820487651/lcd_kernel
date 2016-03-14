export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

obj-m += lcd.o

# Kernel source directory
KDIR =/opt/PHYTEC_BSPs/yocto_ti/build/tmp-glibc/work/phyboard_wega_am335x_1-phytec-linux-gnueabi/linux-ti/3.12.30-phy2-r0/git
PWD = $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
