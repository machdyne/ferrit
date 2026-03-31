# FERRIT Digital Archival Platform

FERRIT is a century-scale digital archival platform.

![Ferrit](https://github.com/machdyne/ferrit/blob/3d812e0852f94d980a5241f4c31794ffb859a9be/ferrit-16.png)

This repo contains PCB layouts, schematics and documentation.

FERRIT uses the [Ebenstahl](https://github.com/machdyne/ebenstahl) firmware.

Find more information on the [FERRIT project page](https://machdyne.com/ferrit/).

*The prototype hardware is under development and has not yet been fully tested.*

## Hardware

FERRIT is a modular upgradable system with 3 component types:

- A controller
- A passive backplane
- Removable ferroelectric memory cards

The controller communicates with the memory cards over a SPI/QSPI bus and exposes the memory to an interface, such as a USB mass storage device.

### Specifications

- Max capacity: 256MB+
- Max addressable memory devices per memory card: 16
- Max addressable memory devices per system: 256
- Interface: Depends on controller
- Speed: Depend on controller

*Note: The max capacity assumes double-sided memory cards with 1MB ICs. Higher capacities may be possible in the future.*

### Components

| Board | Type | Notes |
| ----- | ---- | ----- |
| FERRIT-CY | Controller | RP2040 / USB-FS |
| FERRIT-M8 | Memory card | 8 x SOP-16 |
| FERRIT-16 | Backplane | 16 memory card slots |

## Firmware

The firmware can be updated by holding down the BOOT button while plugging in the USB cable, and then dragging and dropping a new UF2 file to the device.

## License

The contents of this repo are released under the [Lone Dynamics Open License](LICENSE.md).
