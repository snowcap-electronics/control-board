SC_PROJECT = wireless-soil
SC_BOARD = SC_SNOWCAP_STM32F4_V1

# Use 25 Mhz MCU clock instead of 168 Mhz to save power
SC_DEFINES += -DSC_MCU_LOW_SPEED=1

# Use 5 Mhz PWM clock
SC_DEFINES += -DSC_PWM_CLOCK=5000000

# Use Spirit1 with ST licensed code
SC_DEFINES += -DSC_ALLOW_ST_LIBERTY_V2=1 -DSC_HAS_SPIRIT1=1

SC_EXTRA_CSRC = \
			drivers/SPIRIT1_Library/Src/SPIRIT_Aes.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Calibration.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Commands.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Csma.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_DirectRF.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_General.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Gpio.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Irq.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_LinearFifo.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Management.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_PktBasic.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_PktCommon.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_PktMbus.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_PktStack.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Qi.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Radio.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Timer.c \
			drivers/SPIRIT1_Library/Src/SPIRIT_Types.c
