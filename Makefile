obj-m += sbae5_color.o

KERNELRELEASE	?= `uname -r`
KERNEL_DIR	?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(CURDIR)

all:
	@echo "Building SB AE-5plus LED control module"
	${MAKE} -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
