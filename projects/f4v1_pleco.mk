SC_PROJECT = pleco
SC_BOARD = SC_SNOWCAP_STM32F4_V1

# Enable third PWM driver to get 9th PWM pin for the LED shield
SC_DEFINES = -DPWMDX3=PWMD2
SC_DEFINES += -DSC_PWM3_2_PORT=GPIOB
SC_DEFINES += -DSC_PWM3_2_PIN=3
SC_DEFINES += -DSC_PWM3_2_AF=1

# Define    Silk screen    F4v1   LED shield for Pleco
# ----------------------------------------------------
# GPIO1     GPIO1          PA15
# GPIO2     GPIO2          PC0
# GPIO3     GPIO3          PC1
# N/A       GPIO4          PB3    PWM for glow
# N/A       GPIO5          PB4    Head lights
# N/A       GPIO6          PB5    Rear lights
# GPIO4     GPIO7          PD2    N/A
# GPIO5     GPIO8          PA8
# GPIO6     GPIO9          PC4
# GPIO7     GPIO10         PC5

# Override GPIO1 define to silk screen GPIO5 (PB4)
# Override GPIO5 define to silk screen GPIO6 (PB5)
SC_DEFINES += -DGPIO1_PORT=GPIOB
SC_DEFINES += -DGPIO1_PIN=4
SC_DEFINES += -DGPIO5_PORT=GPIOB
SC_DEFINES += -DGPIO5_PIN=5
