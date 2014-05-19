echo Executing GDB with gdbinit to connect to OpenOCD.\n
target remote localhost:3333
monitor reset init
monitor halt
set {int}0xE0042004 = 7
