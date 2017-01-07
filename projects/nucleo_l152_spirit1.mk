SC_PROJECT = spirit
SC_BOARD = SC_L1_NUCLEO
SC_DEFINES = -DHAL_USE_TM=FALSE -DHAL_USE_ADC=FALSE -DHAL_USE_CAN=FALSE -DHAL_USE_GPT=FALSE -DHAL_USE_I2C=FALSE -DHAL_USE_ICU=FALSE -DHAL_USE_MAC=FALSE -DHAL_USE_MMC_SPI=FALSE -DHAL_USE_PWM=FALSE -DHAL_USE_RTC=FALSE -DHAL_USE_SDC=FALSE -DHAL_USE_SERIAL=FALSE -DHAL_USE_SERIAL_USB=FALSE -DHAL_USE_USB=FALSE

# Use Spirit1 with ST licensed code
SC_DEFINES += -DSC_ALLOW_ST_LIBERTY_V2=1 -DSC_HAS_SPIRIT1=1

# STM32L152 has only 16bit timer
SC_DEFINES += -DCH_CFG_ST_RESOLUTION=16

# Decrease frequency from 10000 to 1000 for STM32L152
SC_DEFINES += -DCH_CFG_ST_FREQUENCY=1000

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
