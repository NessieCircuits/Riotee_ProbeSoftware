# Riotee Probe Software

[![Firmware build](https://github.com/NessieCircuits/Riotee_ProbeSoftware/actions/workflows/build-firmware.yml/badge.svg)](https://github.com/NessieCircuits/Riotee_ProbeSoftware/actions/workflows/build-firmware.yml)
[![Python build](https://github.com/NessieCircuits/Riotee_ProbeSoftware/actions/workflows/build-tool.yml/badge.svg)](https://github.com/NessieCircuits/Riotee_ProbeSoftware/actions/workflows/build-tool.yml)
[![PyPiVersion](https://img.shields.io/pypi/v/riotee_probe.svg)](https://pypi.org/project/riotee_probe)

This repository hosts the firmware running on the [Riotee Probe](https://github.com/NessieCircuits/Riotee_ProbeHardware) and the [Riotee Board](https://github.com/NessieCircuits/Riotee_Board) and a command line tool for controlling the probe via USB. The Riotee Board and Riotee Probe have a Raspberry Pi RP2040 that is connected to a PC via USB and controls programming and debugging of the microcontrollers inside the Riotee module.

Features:
 - Programming of the MSP430FR5962 on the Riotee Module
 - Programming of the nRF52833 on the Riotee Module
 - Enabling/disabling a constant power supply
 - Forwarding of UART output from the Riotee Module to a PC
 - Bypassing of power supply for measuring current consumption on the Riotee Board
 - Control of 4 GPIOs on the headers on the Riotee Probe

Download the latest UF2 binaries for the Riotee Board or Riotee Probe from the [release page](https://github.com/NessieCircuits/Riotee_ProbeSoftware/releases/latest) or [build them from source](#building-the-firmware).

## Uploading the firmware

To upload the firmware to the Riotee probe or Riotee board, connect a jumper wire from one of the ground pins to the pad labeled 'USB_BOOT' on the bottom of the board, while plugging in the USB cable. A removable storage drive should appear on your PC. Drop the UF2 binary into the drive.

## Installing the command line tool

Install the command line tool with
```bash
pip install riotee-probe
```

## Usage

To list all available commands run
```bash
riotee-probe --help
```

To upload a hex file to the MSP430 on the Riotee Module:

```bash
riotee-probe program -d msp430 -f build.hex
```

To upload a hex file to the nRF52 on the Riotee Module:

```bash
riotee-probe program -d nrf52 -f build.hex
```

To enable and disable the constant power supply:
```bash
riotee-probe target-power --on
riotee-probe target-power --off
```

## Building the firmware

Follow the [official instructions](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to install and setup the Pico SDK.

Clone this repository and install the submodules:

```bash
git clone --recursive git@github.com:NessieCircuits/Riotee_ProbeSoftware.git
```

Create the `build` directory, configure cmake and build:

```bash
cd Riotee_ProbeSoftware
mkdir firmware/build
cd firmware/build
cmake ..
make
```

By default, cmake will build the firmware for the Riotee probe hardware. If you want to build for the Riotee board hardware instead, set the environment variable `PICO_BOARD` to `riotee_board` before configuring cmake:

```bash
export PICO_BOARD=riotee_board
cmake ..
```

## Uploading the firmware

To upload the firmware to the Riotee probe or Riotee board, connect a wire from one of the ground pins to the pad labeled 'USB_BOOT' on the bottom of the board, while plugging in the USB cable. A removable storage drive should appear on your PC. Drop a UF2 compatible binary into the drive.

## USB IDs

Thanks to the fantastic [pidcodes project](https://pid.codes/) we got two USB product IDs for the Riotee board and the Riotee probe. If you plan to use this software on any other hardware you must obtain a USB Vendor ID and Product ID first and assign it in `src/usb_descriptor.c`.

## References

The software in this repository is heavily based on Raspberry Pi's [Picoprobe](https://github.com/raspberrypi/picoprobe) and uses an implementation of TI's SBW based on the infamous [SLAU320 application note](https://www.ti.com/lit/ug/slau320aj/slau320aj.pdf).
