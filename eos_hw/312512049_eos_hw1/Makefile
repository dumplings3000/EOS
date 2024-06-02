obj-m += driver.o

KDIR = /home/$(USER)/linux
all:
	make -C $(KDIR) M=$(shell pwd) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
clean:
	make -C $(KDIR) M=$(shell pwd) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- clean
