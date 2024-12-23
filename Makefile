EXE_NAME=dcb
VER_MAJ =   1
VER_MIN =   0
VER_PATCH = 0
MAKE_BINARY=yes

TCHAIN = arm-none-eabi-
MCPU += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CDIALECT = gnu99
OPT_LVL = 2
DBG_OPTS = -gdwarf-2 -ggdb -g

CFLAGS   += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CFLAGS   += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CFLAGS   += -fsingle-precision-constant
CFLAGS   += $(C_FULL_FLAGS)
CFLAGS   += -Werror

CXXFLAGS += -fvisibility=hidden -funsafe-math-optimizations -fdata-sections -ffunction-sections -fno-move-loop-invariants
CXXFLAGS += -fmessage-length=0 -fno-exceptions -fno-common -fno-builtin -ffreestanding
CXXFLAGS += -fvisibility-inlines-hidden -fuse-cxa-atexit -felide-constructors -fno-rtti
CXXFLAGS += -fsingle-precision-constant
CXXFLAGS += $(CXX_FULL_FLAGS)
CXXFLAGS += -Werror

LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage
LDFLAGS  += -u _printf_float

EXT_LIBS +=c m nosys

PPDEFS += USE_STDPERIPH_DRIVER STM32F429_439xx STM32F429xx HSE_VALUE=12000000 FW_TYPE=FW_APP CFG_SYSTEM_SAVES_NON_NATIVE_DATA=1 DEV=\"DCB_APP\"
PPDEFS += USBD_CLASS_COMPOSITE_DFU_CDC
PPDEFS += DFU_READS_CFG_SECTION

INCDIR += app
INCDIR += app/display
INCDIR += app/sram
INCDIR += common
INCDIR += common/adc
INCDIR += common/CMSIS
INCDIR += common/config_system
INCDIR += common/console
INCDIR += common/crc
INCDIR += common/fw_header
INCDIR += common/md5
INCDIR += common/STM32_USB_Device_Library/Core
INCDIR += common/STM32_USB_OTG_Driver
INCDIR += common/STM32F4xx_StdPeriph_Driver/inc
INCDIR += common/usb

SOURCES += $(call rwildcard, app, *.c *.S *.s)
SOURCES += $(call rwildcard, common/adc, *.c *.S *.s)
SOURCES += $(call rwildcard, common/CMSIS, *.c *.S *.s)
SOURCES += $(call rwildcard, common/config_system, *.c *.S *.s)
SOURCES += $(call rwildcard, common/console, *.c *.S *.s)
SOURCES += $(call rwildcard, common/crc, *.c *.S *.s)
SOURCES += $(call rwildcard, common/fw_header, *.c *.S *.s)
SOURCES += $(call rwildcard, common/md5, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32_USB_Device_Library/Core, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32_USB_OTG_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/STM32F4xx_StdPeriph_Driver, *.c *.S *.s)
SOURCES += $(call rwildcard, common/usb, *.c *.S *.s)
SOURCES += $(wildcard common/*.c)
LDSCRIPT += ldscript/app.ld

BINARY_SIGNED = $(BUILDDIR)/$(EXE_NAME)_app_signed.bin
BINARY_MERGED = $(BUILDDIR)/$(EXE_NAME)_full.bin
SIGN = $(BUILDDIR)/sign
ARTEFACTS += $(BINARY_SIGNED)

PRELDR_SIGNED = preldr/build/$(EXE_NAME)_preldr_signed.bin
LDR_SIGNED = ldr/build/$(EXE_NAME)_ldr_signed.bin 
EXT_MAKE_TARGETS = $(PRELDR_SIGNED) $(LDR_SIGNED)

include common/core.mk

$(SIGN): sign/sign.c
	@gcc $< -o $@

$(BINARY_SIGNED): $(BINARY) $(SIGN)
	@$(SIGN) $(BINARY) $@ 1916928 \
		prod=$(EXE_NAME) \
		prod_name=$(EXE_NAME)_app \
		ver_maj=$(VER_MAJ) \
		ver_min=$(VER_MIN) \
		ver_pat=$(VER_PATCH) \
		build_ts=$$(date -u +'%Y%m%d_%H%M%S')

$(BINARY_MERGED): $(EXT_MAKE_TARGETS) $(BINARY_SIGNED)
	@echo " [Merging binaries] ..." {$@}
	@cp -f $(PRELDR_SIGNED) $@
	@dd if=$(LDR_SIGNED) of=$@ bs=1 oflag=append seek=16384 status=none
	@dd if=$(BINARY_SIGNED) of=$@ bs=1 oflag=append seek=131072 status=none

.PHONY: composite
composite: $(BINARY_MERGED)

.PHONY: clean
clean: clean_ext_targets

.PHONY: $(EXT_MAKE_TARGETS)
$(EXT_MAKE_TARGETS):
	@$(MAKE) -C $(subst build/,,$(dir $@)) --no-print-directory

.PHONY: clean_ext_targets
clean_ext_targets:
	$(foreach var,$(EXT_MAKE_TARGETS),$(MAKE) -C $(subst build/,,$(dir $(var))) clean;)

#####################
### FLASH & DEBUG ###
#####################

flash: $(BINARY_SIGNED)
	@openocd -d0 -f target/stm32f4xx.cfg -c "program $< 0x08020000 verify reset exit"

flash_all: $(BINARY_MERGED)
	@openocd -d0 -f target/stm32f4xx.cfg -c "program $< 0x08000000 verify reset exit"

program: $(BINARY_SIGNED)
	@usb_dfu_flasher w a $< DCB

ds:
	@openocd -d0 -f target/stm32f4xx.cfg

debug:
	@set _NO_DEBUG_HEAP=1
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target extended-remote :3333" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit