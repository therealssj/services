This firmware had been copied and modified from [trezor-mcu](https://github.com/trezor/trezor-mcu). Please refer to the [README.md file](https://github.com/trezor/trezor-mcu/blob/master/README.md) on that repository for extra details about bootloader and firmware compilation.

This code aims at tranforming the cipher library from [this repository](https://github.com/skycoin/skycoin/tree/develop/src/cipher) into firmware for the STM32 hardware.


# 1. Prepare environment

#### Download GNU ARM Embedded Toolchain

    wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/6-2017q2/gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2

#### Extract crosscompiler and add it to your path

    tar xjf gcc-arm-none-eabi-6-2017-q2-update-linux.tar.bz2
    export PATH="gcc-arm-none-eabi-6-2017-q2-update/bin:$PATH"

#### Install ST-LINK

Follow the steps [here](https://github.com/texane/stlink/blob/master/doc/compiling.md).

#### Configure your usb module

We need to tell your kernel to use the [hidraw module](https://www.kernel.org/doc/Documentation/hid/hidraw.txt) to communicate with the hardware device. If you don't your kernel will treat the device as a mouse or a keyboard.

Create a file named 99-dev-kit.rules in your /etc/udev/rules.d/ folder and write this content in that file (*super user priviledges are required for this step*).

    # 0483:df11 STMicroelectronics STM Device in DFU Mode
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="df11", MODE="0666"
    # 0483:3748 STMicroelectronics ST-LINK/V2
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666"
    # 0483:374b STMicroelectronics ST-LINK/V2.1 (Nucleo-F103RB)
    SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="374b", MODE="0666"
    # 534c:0001 SatoshiLabs Bitcoin Wallet [TREZOR]
    SUBSYSTEM=="usb", ATTR{idVendor}=="534c", ATTR{idProduct}=="0001", MODE="0666"
    KERNEL=="hidraw*", ATTRS{idVendor}=="534c", ATTRS{idProduct}=="0001", MODE="0666"
    # 1209:53c0 SatoshiLabs TREZOR v2 Bootloader
    SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="53c0", MODE="0666"
    KERNEL=="hidraw*", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="53c0", MODE="0666"
    # 1209:53c1 SatoshiLabs TREZOR v2
    SUBSYSTEM=="usb", ATTR{idVendor}=="1209", ATTR{idProduct}=="53c1", MODE="0666"
    KERNEL=="hidraw*", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="53c1", MODE="0666"

Restart your machine or force your udev kernel module to [reload the rules](https://unix.stackexchange.com/questions/39370/how-to-reload-udev-rules-without-reboot).

# 2. How to compile firmware

Then for the actual compilation source make_firmware.sh script:
    cd tiny-firmware
    . make_firmware.sh

# 3. How to burn the firmware in the device

## Use ST-LINK to burn the device

You can check the device is seen by your ST-LINK using this command:

    st-info --probe

To flash the device on a microcontroller of STM32f2xx family the command is:

    st-flash write combined.bin 0x08000000; 
    
If you sourced the make_firmware.sh file as recommended. You can use the alias st-trezor to burn the device.

    st-trezor

# 4. Firmware signature

## Skip the wrong firmware signature warning

We are compiling a modified version of trezor original firmware. The bootloader we are using is configured to detect it and warn the user about this.
We are still in developpement steps, this warning is acceptable. 
The devices allows the user to skip the warning but during that time the OS of the host computer may miss the opportunity to load the driver to communicate with the device.

If when you plug the device your OS is not seeing the device, skip the warning on device's screen saying that the signature is wrong and then try [this](https://askubuntu.com/questions/645/how-do-you-reset-a-usb-device-from-the-command-line).

If you are fast enough you can also quickly click the button "accept" on the device when the warning about wrong firmware signature appears.

## How to perform a custom signature

### Environment setup

The first time we need to prepare the script for firmware signature:

    cd tiny-firmware/bootloader
    ./prepare_signature.sh

This script will generate and copy libskycoin-crypto.so and libtrezor-crypto.so in bootloader repertory.

These libraries are required to perform signature and signature checking from [firmware_sign.py](https://github.com/skycoin/services/blob/master/hardware-wallet/tiny-firmware/bootloader/firmware_sign.py) script.

Note: you also need to be able to generate skycoin key pairs. You can for instance use the [skycoin-cli](https://github.com/skycoin/skycoin).

### You need three signatures:

The system stores five public keys and expects three signatures issued from one of these public keys.

The public keys are hardwritten in the bootloader's source code in file [signatures.c](https://github.com/skycoin/services/blob/master/hardware-wallet/tiny-firmware/bootloader/signatures.c)

The signatures are also present in [firmware_sign.py](https://github.com/skycoin/services/blob/master/hardware-wallet/tiny-firmware/bootloader/firmware_sign.py) script, in the "pubkeys" array.

### Use your secret key to perform signature

Run:

    ./firmware_sign.py -s -f skycoin.bin

The command line tool will ask you in which of the three slots do you want to store the signature.

The it will ask you to provide a secret key that must correspond to one of the five public keys stored in the bootloader and the script as described above.

# 5. Communicate with the device

## Use golang code examples

Check [here](https://github.com/skycoin/services/tree/master/hardware-wallet/go-api-for-hardware-wallet) for golang code example communicating with the device.

Feel free to hack [main.go](https://github.com/skycoin/services/blob/master/hardware-wallet/go-api-for-hardware-wallet/main.go) file.

You can also try the trezorctl [python based command line](https://github.com/trezor/python-trezor).