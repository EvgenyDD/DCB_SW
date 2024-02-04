EXE_NAME=pwr
PRODUCT = pwr
VER_MAJ =   1
VER_MIN =   0
VER_PATCH = 0
MAKE_BINARY=yes

TCHAIN = arm-none-eabi-
MCPU += -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CDIALECT = gnu99
OPT_LVL = 2
DBG_OPTS = -gdwarf-2 -ggdb -g

CFLAGS   += -fdata-sections -ffunction-sections 
CFLAGS   += -fsingle-precision-constant
CFLAGS   += -fmessage-length=0
CFLAGS   += -fno-exceptions -funsafe-math-optimizations
CFLAGS   += -fno-move-loop-invariants -ffreestanding
CFLAGS   += -Wno-pointer-sign -Wswitch-default -Wshadow -Wno-unused
CFLAGS   += -Wall -Wstrict-prototypes -Wdisabled-optimization -Wformat=2 -Winit-self  -Wmissing-include-dirs
CFLAGS   += -Wstrict-overflow=2
# CFLAGS   += -Werror

# LDFLAGS  += -nostartfiles
LDFLAGS  += -specs=nano.specs
LDFLAGS  += -Wl,--gc-sections
LDFLAGS  += -Wl,--print-memory-usage

EXT_LIBS +=c m nosys

PPDEFS += USE_HAL_DRIVER STM32F429xx


INCDIR += base
INCDIR += base/Core/Inc
INCDIR += base/Drivers/CMSIS/Include
INCDIR += base/Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCDIR += base/Drivers/STM32F4xx_HAL_Driver/Inc
INCDIR += common
INCDIR += display

# SOURCES += $(call rwildcard, app, *.c *.S *.s)
SOURCES += $(call rwildcard, base platform display, *.c *.S *.s)
SOURCES += $(wildcard common/*.c)
SOURCES += $(wildcard *.c)

LDSCRIPT += base/STM32F429BITx_FLASH.ld

include common/core.mk

#####################
### FLASH & DEBUG ###
#####################

flash: $(BINARY)
	openocd -f target/stm32f4xx.cfg -c "program $(BINARY) 0x08000000 verify reset exit"

debug:
	@echo "file $(EXECUTABLE)" > .gdbinit
	@echo "set auto-load safe-path /" >> .gdbinit
	@echo "set confirm off" >> .gdbinit
	@echo "target remote | openocd -c \"gdb_port pipe\" -f target/stm32f4xx.cfg" >> .gdbinit
	@arm-none-eabi-gdb -q -x .gdbinit
