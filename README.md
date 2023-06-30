# Riotee Probe Firmware

[![Firmware build](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/build-firmware.yml/badge.svg)](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/build-firmware.yml)
[![Python build](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/build-tool.yml/badge.svg)](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/build-tool.yml)
[![PyPiVersion](https://img.shields.io/pypi/v/riotee_probe.svg)](https://pypi.org/project/riotee_probe)

This repository hosts the firmware running on the [Riotee Probe](https://github.com/NessieCircuits/Riotee_ProbeHardware) and the [Riotee Board](https://github.com/NessieCircuits/Riotee_Board). Both devices have a Raspberry Pi RP2040 that is connected to a PC via USB and controls programming and debugging of the microcontrollers inside the Riotee module. Specifically, the software supports ARM's Serial-Wire-Debug (SWD) and TI's Spy-By-Wire (SBW) protocols. Host-side it exposes a CMSIS-DAP v2 compatible interface and a custom CDC-ACM interface for SBW programming.

Download the latest UF2 binaries for the Riotee Board or Riotee Probe from the [release page](https://github.com/NessieCircuits/Riotee_ProbeFirmware/releases/latest).


## Building

Follow the [official instructions](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to install and setup the Pico SDK.

Clone this repository and install the submodules:

```
git clone git@github.com:NessieCircuits/Riotee_ProbeFirmware.git
cd Riotee_ProbeFirmware
git submodule init
git submodule update
```

Change to the `build` directory, configure cmake and build:

```
cd build
cmake ..
make
```

By default, cmake will build the firmware for the Riotee probe hardware. If you want to build for the Riotee board hardware instead, set the environment variable `PICO_BOARD` to `riotee_board` before configuring cmake:

```
export PICO_BOARD=riotee_board
cmake ..
```

## Uploading the firmware

To upload the firmware to the Riotee probe or Riotee board, connect a wire from one of the ground pins to the pad labelled 'USB_BOOT' on the bottom of the board, while plugging in the USB cable. A removable storage drive should appear on your PC. Drop a UF2 compatible binary into the drive.

## USB IDs

Thanks to the fantastic [pidcodes project](https://pid.codes/) we got two USB product IDs for the Riotee board and the Riotee probe. If you plan to use this software on any other hardware you must obtain a USB Vendor ID and Product ID first and assign it in `src/usb_descriptor.c`.

## References

The software in this repository is heavily based on Raspberry Pi's [Picoprobe](https://github.com/raspberrypi/picoprobe) and uses an implementation of TI's SBW based on the infamous [SLAU320 application note](https://www.ti.com/lit/ug/slau320aj/slau320aj.pdf).
