#
#	Makefile for FreeRTOS demo on Raspberry Pi
#
BASE=$(shell pwd)/
BUILD_DIR=$(shell pwd)/build/

MODULE_NAME="RaspberryPi BSP"

TARGETS=kernel7.img kernel.list kernel.syms kernel.elf kernel.map
LINKER_SCRIPT=raspberrypi.ld

GCC_VERSION=9.2.1

-include .dbuild/dbuild.mk


all: kernel.list kernel7.img kernel.syms
	@$(SIZE) kernel.elf

kernel7.img: kernel.elf
	$(Q)$(PRETTY) IMAGE $(MODULE_NAME) $@
	$(Q)$(OBJCOPY) kernel.elf -O binary $@

kernel.list: kernel.elf
	$(Q)$(PRETTY) LIST $(MODULE_NAME) $@
	$(Q)$(OBJDUMP) -D -S  kernel.elf > $@

kernel.syms: kernel.elf
	$(Q)$(PRETTY) SYMS $(MODULE_NAME) $@
	$(Q)$(OBJDUMP) -t kernel.elf > $@

kernel.elf: LDFLAGS += --start-group -L "/usr/lib/gcc/arm-none-eabi/$(GCC_VERSION)" -lgcc
kernel.elf: LDFLAGS += -L "/usr/lib/arm-none-eabi/lib" -lc -lnosys --end-group
kernel.elf: $(OBJECTS)
	$(Q)$(LD) $(OBJECTS) -Map kernel.map -o $@ -T $(LINKER_SCRIPT) $(LDFLAGS)
