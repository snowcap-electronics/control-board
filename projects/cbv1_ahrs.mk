SC_PROJECT = ahrs
SC_BOARD = SC_SNOWCAP_V1
SC_DEFINES = -DSC_HAS_LSM9DS0 -DSC_USE_9DOF=TRUE -DSC_USE_AHRS=TRUE -DSC_ALLOW_GPL -DSC_WAR_ISSUE_1

# Include filters
SC_DEFINES += -DSC_USE_FILTER_ZERO_CALIBRATE -DSC_USE_FILTER_BROWN_LINEAR_EXPO