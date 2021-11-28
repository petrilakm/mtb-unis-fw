# MTB-UNI v4 Firmware

This repository contains firmware for ATmega128 MCU for
[MTB-UNI v4 module](https://mtb.kmz-brno.cz/uni). Firmware consists of main
firmware & bootloader. Bootloader is placed in `bootloader` directory, it is
a separate compilable project. To compile main fw & bootloader to single hex
file, compile bootloader first and then main firmware. See build instructions
below.

## Build & requirements

This firmware is developed in C language, compiled via `avr-gcc` with help
of `make`. You may also find tools like `avrdude` helpful.

Hex files are available in *Releases* section.

## Programming

Firmware could be programmed

1. via programming connector on MTB-UNI module (ISP),
2. over MTBbus (if bootloader is already present).

> Warning: MTB-UNI v4.0 contains bug: MISO & MOSI pins are not on programming connector,
> they are placed on RS485 driver's pins. For programming v4.0 modules, please
> remove RS485 driver and connect pins MISO & MOSI to appropriate pins. SCK,
> RESET, VCC & GND could be connected via programming connector.

For ISP, please remove RS485 driver as it uses same pins as are used for serial
programming.

This FW uses EEPROM, however programming of EEPROM is not required. There should
be just empty EEPROM on fresh devices.

Flash main application, fuses & bootloader:

```bash
$ cd bootloader
$ make
$ cd ..
$ make
$ make fuses
$ make program
```

Flash fuses & bootloader only:

```bash
$ make fuses
$ cd bootloader
$ make
$ make program
```

## Author's toolkit

Text editor + `make`. No more, no less.

## See also

* [MTB-UNI v4 schematics & pcb](https://github.com/kmzbrnoI/mtb-uni-4-ele)
* [MTBbus protocol](https://github.com/kmzbrnoI/mtbbus-protocol)

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
