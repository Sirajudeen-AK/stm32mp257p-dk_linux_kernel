# 1. Get ALL .c files in the subfolder
ALL_SRCS       := $(wildcard $(M)/*.c)

# 2. Identify the userspace file by looking for 'main'
USERSPACE_SRC  := $(shell grep -lE "int[[:space:]]+main" $(ALL_SRCS) 2>/dev/null)

# 3. Identify temporary build files (e.g., generated .mod.c files)
TEMP_MOD_SRCS  := $(wildcard $(M)/*.mod.c)

# 4. Filter OUT both the userspace files AND the temporary build files
SRCS           := $(filter-out $(USERSPACE_SRC) $(TEMP_MOD_SRCS), $(ALL_SRCS))

# 5. Extract just the filenames (without paths) for objects
OBJS           := $(notdir $(SRCS:.c=.o))

# 6. Setup module and file naming conventions
MODULE_BASE    := $(basename $(notdir $(firstword $(SRCS))))
MODULE_NAME    := kernel_example_$(MODULE_BASE)
FILE_NAME      := $(MODULE_NAME).ko


ifneq ($(KERNELRELEASE),)
# Kernel build context
obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-objs := $(OBJS)

else
.DEFAULT_GOAL := all

KDIR := /home/siraj/Code/kernel-source/linux-stm32mp-6.6.116-stm32mp-r3-r0/linux-6.6.116
KBUILD_OUTPUT := /home/siraj/Code/kernel-source/linux-stm32mp-6.6.116-stm32mp-r3-r0/build
SYMVERS_FILE := $(KBUILD_OUTPUT)/Module.symvers
VMLINUX_SYMVERS := $(KBUILD_OUTPUT)/vmlinux.symvers

KO_OUTPUT_DIR := /home/siraj/Code/ST32MP257P-DK/KERNELSPACE/___output_files
WINDIR := /mnt/s/_______UBUNTU________/stm32_modules_application

.PHONY: all clean prepare-symvers print-files

# --- NEW DEBUG TARGET ---
print-files:
	@echo "========================================================================"
	@echo " [SUBFOLDER PATH]: $(M)"
	@echo "========================================================================"
	@echo " 🔹 All Detected Files:    $(notdir $(ALL_SRCS))"
	@echo "************************************************************************"
	@echo " 🟢 Found Userspace File:  $(notdir $(USERSPACE_SRC))"
	@echo "------------------------------------------------------------------------"
	@echo " 🔥 Found Kernel Srcs:   $(notdir $(SRCS))"
	@echo "************************************************************************"
	@echo " 🟠 Ignored Kernel Temps:  $(notdir $(TEMP_MOD_SRCS))"
	@echo " 🎯 Target Kernel Objects: $(OBJS)"
	@echo "========================================================================"

prepare-symvers:
	@if [ ! -f $(SYMVERS_FILE) ] && [ -f $(VMLINUX_SYMVERS) ]; then \
		cp $(VMLINUX_SYMVERS) $(SYMVERS_FILE); \
	fi

all: prepare-symvers
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) M=$(M) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
	@mkdir -p $(KO_OUTPUT_DIR)
	@cp $(M)/$(FILE_NAME) $(KO_OUTPUT_DIR)
	@cp $(M)/$(FILE_NAME) $(WINDIR)

clean:
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) M=$(M) clean
	@rm -f $(KO_OUTPUT_DIR)/$(FILE_NAME)
endif