SC_PROJECT = tracker
SC_BOARD = RT_C2

# Disable ChibiOS HAL features
SC_DEFINES = -DHAL_USE_ADC=FALSE -DHAL_USE_CAN=FALSE -DHAL_USE_ICU=FALSE -DHAL_USE_MAC=FALSE -DHAL_USE_MMC_SPI=FALSE -DHAL_USE_PWM=FALSE

# Disable ChibiOS features not used currently by Snowcap projects
# vsnprintf with %f seems to need CH_USE_MEMCORE, CH_USE_HEAP and CH_USE_DYNAMIC
SC_DEFINES += -DCH_USE_CONDVARS=FALSE -DCH_USE_MESSAGES=FALSE -DCH_USE_MEMPOOLS=FALSE

# Enabled Snowcap features
SC_DEFINES += -DSC_USE_NMEA -DSC_USE_SLRE -DSC_USE_GSM -DSC_USE_SHA1

# Enable float type support in ch*printf
SC_DEFINES += -DCHPRINTF_USE_FLOAT=TRUE

# Increase the size of maximum UART message
SC_DEFINES += -DUART_MAX_SEND_BUF_LEN=254

# Increase the event queue size and the event loop thread's work area
SC_DEFINES += -DEVENT_MB_SIZE=512 -DSC_EVENT_LOOP_THREAD_WA=4096

# Increase the number of USB serial send buffers
SC_DEFINES += -DSDU_MAX_SEND_BUFFERS=8

# Enable WFI
SC_DEFINES += -DCORTEX_ENABLE_WFI_IDLE=TRUE
