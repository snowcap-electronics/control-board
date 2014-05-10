SC_PROJECT = tracker
SC_BOARD = RT_C2

# Disable ChibiOS HAL features
SC_DEFINES = -DHAL_USE_ADC=FALSE -DHAL_USE_CAN=FALSE -DHAL_USE_ICU=FALSE -DHAL_USE_MAC=FALSE -DHAL_USE_MMC_SPI=FALSE -DHAL_USE_PWM=FALSE

# Disable ChibiOS features not used currently by Snowcap projects
#SC_DEFINES += -DCH_USE_CONDVARS=FALSE -DCH_USE_MESSAGES=FALSE -DCH_USE_MEMCORE=FALSE -DCH_USE_HEAP=FALSE -DCH_USE_MEMPOOLS=FALSE -DCH_USE_DYNAMIC=FALSE

# Enabled Snowcap features
SC_DEFINES += -DSC_USE_NMEA -DSC_USE_SLRE -DSC_USE_GSM -DSC_USE_SHA1

# Enable float type support in ch*printf
SC_DEFINES += -DCHPRINTF_USE_FLOAT=TRUE

# Increase the size of maximum UART message
SC_DEFINES += -DUART_MAX_SEND_BUF_LEN=254

# Increase the event queue size
SC_DEFINES += -DEVENT_MB_SIZE=512

# Increase the number of USB serial send buffers
SC_DEFINES += -DSDU_MAX_SEND_BUFFERS=8