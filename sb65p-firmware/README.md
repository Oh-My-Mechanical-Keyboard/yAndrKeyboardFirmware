# SB65P Wireless Keyboard Firmware
Firmware for Nordic MCUs used in the Redox wireless Keyboard, contains precompiled .hex files, as well as sources buildable with the Nordic SDK
This firmware was derived from [Reversebias' Mitosis](https://github.com/reversebias/mitosis) and [Durburz's Interphase](https://github.com/Durburz/interphase-firmware/) firmware.



## Install dependencies

Tested on Ubuntu 16.04.2, but should be able to find alternatives on all distros.

```
sudo apt install openocd gcc-arm-none-eabi
```

## Download Nordic SDK

Nordic does not allow redistribution of their SDK or components, so download and extract from their site:

https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v11.x.x/nRF5_SDK_11.0.0_89a8197.zip

Firmware written and tested with version 11

```
unzip nRF5_SDK_11.0.0_89a8197.zip -d nRF5_SDK_11
cd nRF5_SDK_11
```

## Toolchain set-up

A cofiguration file that came with the SDK needs to be changed. Assuming you installed gcc-arm with apt, the compiler root path needs to be changed in /components/toolchain/gcc/Makefile.posix, the line:

```
GNU_INSTALL_ROOT := /usr/local/gcc-arm-none-eabi-4_9-2015q1
```

Replaced with:

```
GNU_INSTALL_ROOT := /usr/
```

## Flash

build firmware

```
make -C sb65p-receiver-basic/custom/armgcc;
make -C sb65p-keyboard-basic/custom/armgcc keyboard_side=left;
make -C sb65p-keyboard-basic/custom/armgcc keyboard_side=right; 
```

use Jlink to flash the firmware into corresponding chips. The Nrfgo Studio and JFrash are recommended.