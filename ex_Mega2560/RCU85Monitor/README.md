# RCU85 Remote Monitor
This project is intended to support remote hardware monitoring and programming of the RCU85 board (AKA DaCosta Bot - YouTube Project).
The monitor supports DMA of the memory and I/O bus on the RCU85 embedded computer, manual halt and resume (with optional reset).
Simple commands can be used to read the memory and I/O busses and to write or modify memory and I/O, up to 8 bytes per command.
An alternate "bulk write" command (needs a manual halt) can program N bytes in a serial fashion into memory or I/O and is terminated by a ETX (0x03) byte.
The bulk write supports a copy-paste workflow.

# Commands
The following commands are supported:

## help (h)
Displays all supported commands in a brief System Help menu.

## mode (m)
USAGE: {mode | m} [{term | kybd}]
  term - terminal mode. LED display controlled by serial terminal data.
  kybd - keyboard/LED mode. LED display show the key-switch panel (keyboard) settings.
         (key-switch mode is not yet fully supported)
If no arguments given then the command returns the current mode setting. 
On power/reset the mode is [term]inal.

Future (kybd): This mode is intended to support the Atmega2650 as a key-switch & LED controller for a way to monitor the RCU85 directly from the robot's key-switch panel.


