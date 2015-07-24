SC_PROJECT = test
SC_BOARD = SC_F1_DISCOVERY
SC_DEFINES = -DHAL_USE_USB=FALSE -DHAL_USE_SERIAL_USB=FALSE

# Disable ChibiOS HAL features to decrease RAM usage
SC_DEFINES += -DHAL_USE_TM=FALSE -DHAL_USE_ADC=FALSE -DHAL_USE_CAN=FALSE -DHAL_USE_GPT=FALSE -DHAL_USE_I2C=FALSE -DHAL_USE_ICU=FALSE -DHAL_USE_MAC=FALSE -DHAL_USE_MMC_SPI=FALSE -DHAL_USE_PWM=FALSE -DHAL_USE_RTC=FALSE -DHAL_USE_SDC=FALSE -DHAL_USE_SERIAL=FALSE -DHAL_USE_SPI=FALSE

# Disable ChibiOS features not used currently by Snowcap projects
SC_DEFINES += -DCH_USE_CONDVARS=FALSE -DCH_USE_EVENTS=FALSE -DCH_USE_MESSAGES=FALSE -DCH_USE_MEMCORE=FALSE -DCH_USE_HEAP=FALSE -DCH_USE_MEMPOOLS=FALSE -DCH_USE_DYNAMIC=FALSE

# Disable help for commands
SC_DEFINES += -DSC_CMD_REMOVE_HELP=1

# Tune sizes to decrease RAM usage
SC_DEFINES += -DSC_EVENT_LOOP_THREAD_WA=512 -DPORT_INT_REQUIRED_STACK=32 -DEVENT_MB_SIZE=16 -DSC_BLOB_MAX_SIZE=2 -DSC_CMD_MAX_COMMANDS=8 -DUART_MAX_CIRCULAR_BUFS=4

# STM32F1 has only 16bit timer
SC_DEFINES += -DCH_CFG_ST_RESOLUTION=16

# Decrease frequency from 10000 to 1000 for STM32F1
SC_DEFINES += -DCH_CFG_ST_FREQUENCY=1000
