# Atmel For Linux
Atmel MCU Projects and demos using a Linux host.

This repo holds libraries, build systems and example source for "bare metal" C projects on the Atmel AVR series 8-bit RISC MCUs.

## Supported Series
- ATmega2560x

# Environment Setup - Ubuntu 16.04, 18.04

sudo apt-get update
sudo apt-get install gcc build-essential
sudo apt-get install gcc-avr binutils-avr avr-libc gdb-avr
sudo apt-get install avrdude

## ONLY FOR USB PROGRAMMERS (Does not include boards with built-in USB)

sudo apt-get install libusb-dev

Note: some AVR debuggers like avrice do not work with the latest version of libusb (1.0 or later) They need the older version of libusb. instructions for getting this working are in progress.

