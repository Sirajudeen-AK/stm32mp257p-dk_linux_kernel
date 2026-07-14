# 1. Get ALL .c files in the subfolder
ALL_SRCS       := $(wildcard $(M)/*.c)

# 2. Identify the userspace file by looking for 'main'
USERSPACE_SRC  := $(shell grep -lE "int[[:space:]]+main" $(ALL_SRCS) 2>/dev/null)

# 3. Identify temporary build files (e.g., generated .mod.c files)
TEMP_MOD_SRCS  := $(wildcard $(M)/*.mod.c)

# 4. Filter OUT both the userspace files AND the temporary build files
SRCS           := $(filter-out $(USERSPACE_SRC) $(TEMP_MOD_SRCS), $(ALL_SRCS))

# 5. Base names (no path, no .c) - one per kernel module source file
BASE_NAMES     := $(basename $(notdir $(SRCS)))

# 6. Each source file becomes its own module: Ukernel_<name>.ko
MODULE_NAMES   := $(addprefix Ukernel_,$(BASE_NAMES))
FILE_NAMES     := $(addsuffix .ko,$(MODULE_NAMES))

ifneq ($(KERNELRELEASE),)
# Kernel build context: one obj-m entry per source file
obj-m := $(addsuffix .o,$(MODULE_NAMES))

# For each ModuleX.c -> Ukernel_ModuleX.o built from ModuleX.o
define MAKE_MODULE_RULE
Ukernel_$(1)-objs := $(1).o
endef
$(foreach b,$(BASE_NAMES),$(eval $(call MAKE_MODULE_RULE,$(b))))

else
.DEFAULT_GOAL := all

KDIR := /home/siraj/Code/kernel-source/linux-stm32mp-6.6.116-stm32mp-r3-r0/linux-6.6.116
KBUILD_OUTPUT := /home/siraj/Code/kernel-source/linux-stm32mp-6.6.116-stm32mp-r3-r0/build
SYMVERS_FILE := $(KBUILD_OUTPUT)/Module.symvers
VMLINUX_SYMVERS := $(KBUILD_OUTPUT)/vmlinux.symvers

KO_OUTPUT_DIR := /home/siraj/Code/ST32MP257P-DK/KERNELSPACE/___output_files
WINDIR := /mnt/s/_______UBUNTU________/stm32_modules_application

.PHONY: all clean prepare-symvers print-files

print-files:
	@echo "========================================================================"
	@echo " [SUBFOLDER PATH]: $(M)"
	@echo "========================================================================"
	@echo " 🔹 All Detected Files:    $(notdir $(ALL_SRCS))"
	@echo "************************************************************************"
	@echo " 🟢 Found Userspace File:  $(notdir $(USERSPACE_SRC))"
	@echo "------------------------------------------------------------------------"
	@echo " 🔥 Found Kernel Srcs:     $(notdir $(SRCS))"
	@echo " 📦 Modules To Build:      $(FILE_NAMES)"
	@echo "************************************************************************"
	@echo " 🟠 Ignored Kernel Temps:  $(notdir $(TEMP_MOD_SRCS))"
	@echo "========================================================================"

prepare-symvers:
	@if [ ! -f $(SYMVERS_FILE) ] && [ -f $(VMLINUX_SYMVERS) ]; then \
		cp $(VMLINUX_SYMVERS) $(SYMVERS_FILE); \
	fi

all: prepare-symvers
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) M=$(M) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
	@mkdir -p $(KO_OUTPUT_DIR)
	@for f in $(FILE_NAMES); do \
		cp $(M)/$$f $(KO_OUTPUT_DIR); \
		cp $(M)/$$f $(WINDIR); \
	done

clean:
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) M=$(M) clean
	@for f in $(FILE_NAMES); do \
		rm -f $(KO_OUTPUT_DIR)/$$f; \
	done
endif