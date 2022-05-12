# RCU85 Remote Monitor
This project is intended to support remote hardware monitoring and programming of the RCU85 board (AKA DaCosta Bot - YouTube Project).
The monitor supports DMA of the memory and I/O bus on the RCU85 embedded computer, manual halt and resume (with optional reset).
Simple commands can be used to read the memory and I/O busses and to write or modify memory and I/O, up to 8 bytes per command.
An alternate "bulk write" command (needs a manual halt) can program N bytes in a serial fashion into memory or I/O and is terminated by a ETX (0x03) byte.
The bulk write supports a copy-paste workflow.

Hexadecimal values - All numerical values used in this monitor are hexadecimal unless otherwise stated. The entering of hexadecimal numbers DOES NOT require a leading "0x" or trailing "h".

# Commands
The following commands are supported:

## help (h) 
USAGE: {help | h} (enter)
Displays all supported commands in a brief System Help menu.

## mode (m)
USAGE: {mode | m} [{term | kybd}] (enter)

If no arguments given then the command returns the current mode setting. 
On power/reset the mode is [term]inal.

 - term : terminal mode. LED display controlled by serial terminal data.
 - kybd : keyboard/LED mode. LED display show the key-switch panel (keyboard) settings.
          (key-switch mode is not yet fully supported)

Future (kybd): This mode is intended to support the Atmega2650 as a key-switch & LED controller for a way to monitor the RCU85 directly from the robot's key-switch panel.

## iom (im)
USAGE: {iom | im} [io | mem] (enter)

Set or read the current access mode.

 - io  : Set the access mode to read and write the I/O space (0x00 .. 0xff)
 - mem : Set the access mode to rad and write program and data memory
  
By default the mode is set to 'mem'. The mode must be changed prior to 
writing data or reading data in the monitor.

## read (r)
USAGE: {read | r} addr len (enter)

Read from I/O or program/data memory. starting address and length are manditory parameters.

 - addr : start address for the read, in hexadecimal.
 - len  : # bytes to read. max. is 64 in one command.

## write (w)
USAGE: {write | w} addr d0 [d1 [d2 ...]] (enter)

Write one or more bytes to I/O or program/data memory. An address and at least one write-byte is required. 

 - addr      : start address for the write, in hexadecimal.
 - d0,d1..d7 : write byte(s) in hexadecimal MAX: 8 bytes in one command.

Note: This operation is considered atomic, so a halt is not required for this command. If the CPU is not already on hold then it will be halted while memory is being written and then released again without a reset.

## addr (a)
USAGE: {addr | a} [addr] (enter)

Set the starting address for a write. This command supports 'bwrt' (bulk-write) and may be removed in future as Intel hex write support does not require a pre-set address.

 - addr : start address for the write, in hexadecimal.

## bwrt (b)
USAGE: {bwrt | b} data.. EOF (enter)

(!) The RCU85 should be in a halted state prior to using this command.

(!) This command requires the starting address and access mode to be set prior to usage.

 - data : data is 2 characters per byte (ASCII-HEX) with no leading or trailing additional characters, eg. "0x". No spaces are required between bytes. The last byte must be by byte value 03 (ETX).
  
DEPRECATED - This raw write command is not recommended for writing bulk data into I/O or program/data memory. Use the Intel hex write command 'hwrt' instead.

## hwrt
USAGE: hwrt (enter)

(!) The RCU85 should be in a halted state prior to using this command.

Write data to I/O or program/data memory. Data is fed to the interpretor in t he form of Intel hex record lines. 
The interpretor will parse each line and generate an "OK" on a CRC match or "ERR-CRC" is there is a CRC error. 
Once the "End Record" line is fed to the interpretor, the parsing will complete and the command will complete. 

(!) The hex record interpretor will remain running until it receives the "end record" line.
(!) be careful trying to send large chunks of data in the form of multiple hex record lines. Try to restrict 
each copy-paste operation to 1 or 2 lines. Multi-line pastes into the serial terminal are supported.

Note: as each hex record line has its own starting address, this command will support "scattered" writes to memory.

## halt (hold)
USAGE: {halt | hold} (enter)

Halt the RCU85. Use this to stop program execution when downloading or altering program memory.

## run (go)
USAGE: {run | go} [reset] (enter)

Resume program execution from a HALT condition. If 'reset' is given as an argument then the CPU is reset when hold is being de-asserted, re-starting progam execution from address 0x0000.

