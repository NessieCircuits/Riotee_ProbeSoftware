# Riotee Probe Firmware

[![Cmake build](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/cmake.yml/badge.svg)](https://github.com/NessieCircuits/Riotee_ProbeFirmware/actions/workflows/cmake.yml)

This repository hosts the firmware running on the [Riotee probe](https://github.com/NessieCircuits/Riotee_ProbeHardware) and the [Riotee board](https://github.com/NessieCircuits/Riotee_Board). Both devices have a Raspberry Pi RP2040 that is connected to a PC via USB and controls programming and debugging of the microncontrollers inside the Riotee module. Specifically, the software supports ARM's Serial-Wire-Debug (SWD) and TI's Spy-By-Wire (SBW) protocols. Host-side it exposes a CMSIS-DAP v2 compatible interface and a custom CDC-ACM interface for SBW programming.

# References

The software in this repository is heavily based on Raspberry Pi's [Picoprobe](https://github.com/raspberrypi/picoprobe) and uses an implementation of TI's SBW based on the infamous [SLAU320 application note](https://www.ti.com/lit/ug/slau320aj/slau320aj.pdf).
