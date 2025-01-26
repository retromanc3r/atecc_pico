# ATECC608 Pico

## Overview

This project is designed to interface with the Microchip ATECC608 cryptographic co-processor using a Raspberry Pi Pico. It provides functionality for reading device configuration, checking lock status, generating secure random numbers, and interfacing with the chip over I2C.

## Features

- ✅ **Wake-up & Communication**: Initializes and wakes up the ATECC608 over I2C.
- 🔎 **Read Configuration Data**: Reads and displays the device's configuration zone.
- 🔐 **Check Lock Status**: Determines whether the ATECC608 is locked or unlocked.
- 🎲 **Generate Random Numbers**: Utilizes the ATECC608’s True Random Number Generator.
- 🔢 **Compute SHA-256 Hash**: Computes a SHA-256 hash of a message using the ATECC608.
- 📜 **Retrieve Serial Number**: Reads and displays the unique serial number of the device.
- 🛠 **I2C Communication**: Implements sending and receiving commands using the Pico I2C interface.

## Hardware Requirements

- Raspberry Pi Pico / Pico2 (or any RP2040 or RP2350 board)
- Microchip ATECC608 (A or B variant, I2C interface)
- Pull-up resistors (4.7kΩ) for SDA and SCL
- Jumper wires for I2C connections

## Wiring Guide

| Raspberry Pi Pico | ATECC608 |
|-------------------|----------|
| GP4 (SDA)         | SDA      |
| GP5 (SCL)         | SCL      |
| GND               | GND      |
| 3.3V              | VCC      |

## Software Requirements

- CMake (for building the project)
- Pico SDK (for Raspberry Pi Pico development)
- GNU Arm Toolchain (for compiling the project)
- VS Code + CMake Tools (recommended for development)

## Installation & Setup

1. Clone the repository:
    ```sh
    git clone https://github.com/your-repo/atecc_pico.git
    cd atecc_pico
    ```

2. Initialize the Pico SDK:
    ```sh
    git submodule update --init --recursive
    ```

3. Build the project:
    ```sh
    mkdir build
    cd build
    cmake ..
    make
    ```
    Use this `cmake` command instead when building for the Raspberry Pi Pico2.
    ```sh
    cmake -DPICO_BOARD=pico2 ..
    ```

4. Flash the firmware onto the Pico:
    - Hold the BOOTSEL button on the Pico and connect it to your PC.
    - Copy the generated `atecc_pico.uf2` file to the `RPI-RP2` USB drive.

## Usage

1. Run the firmware and view output:
    ```sh
    minicom -b 115200 -o -D /dev/ttyACM0
    ```
    or
    ```sh
    minicom -b 115200 -o -D /dev/ttyUSB0
    ```

2. Expected Output (Locked):
    ```
    📡 Initializing ATECC608...
    🛰️ **Wake-up Response:** 04 11 33 43 
    ✅ Wake-up successful!
    🆔 Serial Number: 
    🎲 Random Number (Mapped to Range 100-65535): 
    🔢 SHA-256: B10901DE652A676C0376E1CF5CE07FB7BEB6E4568BEF390E52696B48744B8A64
    🔎 Checking Slot 0 Configuration...
    🔎 Slot 0 Config Data: 
    🎲 Random Value (HEX):  
    🔎 Reading Configuration Data...
    01 23 EA A2 ...
    🔍 Checking ATECC608A Lock Status...
    🔐 Raw Lock Status Response: 07 00 00 00 00
    🔒 Config Lock Status: 00
    🔒 Data Lock Status: 00
    🔒 Chip is **FULLY LOCKED** (Config & Data).
    🎉 ATECC608 Test Complete!
    ```

3. Expected Output (Unlocked):
    ```
    📡 Initializing ATECC608...
    🛰️ **Wake-up Response:** 04 11 33 43 
    ✅ Wake-up successful!
    🆔 Serial Number: 
    🎲 Random Number (Mapped to Range 100-65535): 
    🔢 SHA-256: B10901DE652A676C0376E1CF5CE07FB7BEB6E4568BEF390E52696B48744B8A64
    🔎 Checking Slot 0 Configuration...
    🔎 Slot 0 Config Data: 
    🎲 Random Value (HEX):  
    🔎 Reading Configuration Data...
    01 23 EA A2 ...
    🔍 Checking ATECC608A Lock Status...
    🔐 Raw Lock Status Response: 07 00 00 55 55
    🔒 Config Lock Status: 55
    🔒 Data Lock Status: 55
    🔒 Chip is **UNLOCKED** (Config & Data).
    🎉 ATECC608 Test Complete!
    ```

## Deployment

Drop `atecc_pico.uf2` on your Pico after you build the project or use a Raspberry Pi Debug Probe to load `atecc_pico.elf` onto the board via remote debugging with OpenOCD (provided your environment is setup).

OpenOCD
```sh
openocd -f tcl/interface/cmsis-dap.cfg -f tcl/target/rp2350.cfg -c "adapter speed 5000"
```

GDB
```sh
gdb-multiarch atecc_pico.elf
target remote localhost:3333
load
c
```


## Bill of Materials (BOM)

| **Part**                     | **Description**                                      | **Quantity** | **Notes** | **Order Link** |
|------------------------------|--------------------------------------------------|------------|----------|--------------|
| **Raspberry Pi Pico / Pico2**  | RP2040-based microcontroller board                 | 1          | Can also use Pico W / Pico2 | [Adafruit](https://www.adafruit.com/product/6006) / [DigiKey](https://www.digikey.com/en/products/detail/raspberry-pi/SC1631/24627136) |
| **ATECC608 Breakout Board**    | Adafruit ATECC608 STEMMA QT / Qwiic                | 1          | Any I2C-based ATECC608 board will work | [Adafruit](https://www.adafruit.com/product/4314) / [DigiKey](https://www.digikey.com/en/products/detail/adafruit-industries-llc/4314/10419053) |
| **4.7kΩ Resistors**           | Pull-up resistors for I2C lines                   | 2          | Optional if using STEMMA QT or built-in pull-ups | [Amazon](https://www.amazon.com/Elegoo-Values-Resistor-Assortment-Compliant/dp/B072BL2VX1) |
| **Breadboard**                | Prototyping board                                 | 1          | Any standard size | [Adafruit](https://www.adafruit.com/product/239) / [DigiKey](https://www.digikey.com/en/products/detail/global-specialties/GS-830/5231309) |
| **Jumper Wires**              | Male-to-male / Male-to-female wires               | 6+         | For I2C and power connections | [Adafruit](https://www.adafruit.com/product/1957) / [DigiKey](https://www.digikey.com/en/products/filter/jumper-wire/640) |
| **USB Cable**                 | USB to Micro-USB / USB-C                          | 1          | For flashing firmware | [Adafruit](https://www.adafruit.com/product/592) |
| **Raspberry Pi Debug Probe**  | Debugging interface for OpenOCD (optional)        | 1          | Used for debugging/flashing firmware | [Adafruit](https://www.adafruit.com/product/5699) / [Mouser](https://www.mouser.com/ProductDetail/Adafruit/5699) |


## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Author

@retromanc3r

## Acknowledgments

Special thanks to the Pico SDK community and Microchip for their excellent documentation and CryptoAuthLib for the ATECC608. Also, credits to GitHub Copilot for assisting in code generation, but all debugging, I2C bus analysis, and code optimization were done manually.
