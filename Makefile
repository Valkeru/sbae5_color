obj-m += sbae5_color.o

KERNELRELEASE	?= `uname -r`
KERNEL_DIR	?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(CURDIR)

all: sbae5_color.ko install

sbae5_color.ko:
	@echo "Building SB AE-5plus backlight control module"
	${MAKE} -C $(KERNEL_DIR) M=$(PWD) modules
install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
	@echo "SB AE-5plus backlight control module successfully installed"
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
