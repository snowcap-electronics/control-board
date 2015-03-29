##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -falign-functions=16
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT = 
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti -fno-exceptions
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = no
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x400
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#
ifeq ($(MAKECMDGOALS),clean)
  ifeq ($(SC_PROJECT_CONFIG),)
#   If cleaning up, it doesn't matter which project is used
	SC_PROJECT_CONFIG = projects/f4v1_test.mk
  endif
endif

ifneq ($(SC_PROJECT_CONFIG),)
  include $(SC_PROJECT_CONFIG)
else
  ifneq ($(wildcard local_config.mk),)
    include local_config.mk
  else
    $(warning Provide a config using either of the following methods:)
    $(warning 1: Copy a project config from projects directory to local_config.mk)
    $(warning 2: make SC_PROJECT_CONFIG=<path/to/config.mk>)
    $(error No project config found)
  endif
endif

ifeq ($(SC_PROJECT),)
  $(error SC_PROJECT not defined, use e.g. "test")
endif


# Define project name here
PROJECT = $(SC_PROJECT)

# Imported source files
CHIBIOS = ChibiOS

ifeq ($(SC_BOARD),SC_SNOWCAP_V1)
# STM32F2 not supported by Chibios v3 (currently, 2015-01-13)
LDSCRIPT= $(PORTLD)/STM32F405xG.ld
MCU = cortex-m4
USE_FPU = hard
include $(CHIBIOS)/os/hal/hal.mk
include boards/SNOWCAP_CONTROL_BOARD_V1/board.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/rt/ports/ARMCMx/compilers/GCC/mk/port_stm32f4xx.mk
else ifeq ($(SC_BOARD),SC_SNOWCAP_STM32F4_V1)
LDSCRIPT= $(PORTLD)/STM32F405xG.ld
MCU = cortex-m4
USE_FPU = hard
include $(CHIBIOS)/os/hal/hal.mk
include boards/SNOWCAP_STM32F4_V1/board.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/rt/ports/ARMCMx/compilers/GCC/mk/port_stm32f4xx.mk
else ifeq ($(SC_BOARD),SC_F4_DISCOVERY)
LDSCRIPT= $(PORTLD)/STM32F407xG.ld
MCU = cortex-m4
USE_FPU = hard
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/boards/ST_STM32F4_DISCOVERY/board.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/rt/ports/ARMCMx/compilers/GCC/mk/port_stm32f4xx.mk
else ifeq ($(SC_BOARD),SC_F1_DISCOVERY)
LDSCRIPT= $(PORTLD)/STM32F100xB.ld
MCU = cortex-m3
USE_FPU = no
include $(CHIBIOS)/os/hal/boards/ST_STM32VL_DISCOVERY/board.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32F1xx/platform.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/rt/ports/ARMCMx/compilers/GCC/mk/port_stm32f1xx.mk
else ifeq ($(SC_BOARD),RT_C2)
LDSCRIPT= $(PORTLD)/STM32F405xG.ld
MCU = cortex-m4
USE_FPU = hard
include $(CHIBIOS)/os/hal/hal.mk
include boards/RT_C2/board.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/rt/ports/ARMCMx/compilers/GCC/mk/port_stm32f4xx.mk
else
$(error SC_BOARD not defined or unknown)
endif

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(PORTSRC) \
       $(KERNSRC) \
       $(HALSRC) \
       $(OSALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC) \
       $(CHIBIOS)/os/hal/lib/streams/memstreams.c \
       $(CHIBIOS)/os/hal/lib/streams/chprintf.c \
       $(CHIBIOS)/os/various/syscalls.c \
       drivers/sc_lis302dl.c \
       drivers/sc_lsm9ds0.c \
       drivers/gsm.c \
       src/sc.c \
       src/sc_led.c \
       src/sc_log.c \
       src/sc_pwm.c \
       src/sc_uart.c \
       src/sc_utils.c \
       src/sc_icu.c \
       src/sc_cmd.c \
       src/sc_i2c.c \
       src/sc_pwr.c \
       src/sc_event.c \
       src/sc_sdu.c \
       src/sc_hid.c \
       src/sc_adc.c \
       src/sc_gpio.c \
       src/sc_spi.c \
       src/sc_9dof.c \
       src/sc_extint.c \
       src/sc_radio.c \
       src/sc_ahrs.c \
       src/sc_filter.c \
       src/sc_i2s.c \
       src/sha1.c \
       src/nmea.c \
       src/slre.c \
       src/testplatform.c \
       src/main-$(SC_PROJECT).c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(PORTASM)

INCDIR = $(PORTINC) $(KERNINC) \
         $(HALINC) $(OSALINC) $(PLATFORMINC) $(BOARDINC) \
         $(CHIBIOS)/os/various \
         $(CHIBIOS)/os/hal/include \
         $(CHIBIOS)/os/hal/lib/streams \
         config include

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
DDEFS =

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS = -lm

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################
ifeq ($(SC_BUILD_TYPE),release)
  USE_OPT += -O3
  DDEFS += -DRELEASE_BUILD
else
  USE_OPT += -O0 -ggdb -fno-omit-frame-pointer
endif

ifeq ($(SC_PEDANTIC_COMPILER),1)
  USE_OPT += -Werror
endif

ifneq ($(SC_DEFINES),)
  DDEFS += $(SC_DEFINES)
endif

RULESPATH = $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC
include $(RULESPATH)/rules.mk
