echo Executing GDB with .gdbinit to connect to OpenOCD.\n
target remote localhost:3333
monitor reset init
monitor halt
