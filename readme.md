# Firmware Update via SPI for Microchip G55 Microcontrollers

This program provides a way to update the firmware of the Microchip SAMG55 chip through its SPI interface by using the pre-programmed bootloader.

A detailed description of the bootloader functionality can be found in the Microchip Bootloader Application Note[^1].



## Getting Started

If you want to include spiflasher in a Yocto generated image, you can add the [spiflash.bb](spiflash.bb) recipe to your layer together with [spiflasher.c](spiflasher.c).

Then you can trigger a SPI flash update with:

```
spiflasher path/to/fw/file.bin
```

### HW Configuration
The SPI flasher needs to know which GPIOs can be used to communicate with the NRST and NCHG pins ot the chip. Additionally, the SPI device number and chip select ID are also required.

You must therefore modify the corresponding definitions in [spiflasher.c](spiflasher.c) to include this information. For example:

```
#define GPIO_NRST   35
#define GPIO_NCHG   47
#define SPI_DEVICE   1
#define CHIP_SELECT  0 
```

### Prepare firmware update file
To create a firmware file that can be used with spiflasher we need to take into consideration the following:

1) The application file that we want to flash needs to be compiled with a specific linker script so that the build process is aware of the booloader. See linker script configuration section in the Bootloader Aplication Note[^1].

2) The firmware update file can be created from the .bin file obtained in the previous step by splitting the binary in frames which include CRC codes. The hex2fw.py script provided by Microchip[^2] offers this functionality.
```
  hex2fw.py -c config\config_atsamg55.icf -i input_file.bin -o output_file.bin
```

## Build locally
If you prefer to compile the project locally, you can do so by simply typing:

make

This will generate a spiflasher binary in the main project folder.

Please note that this project depends on the library libsoc and only version 0.8.2 has been checked for compatibility. You can find its installation instructions in the project page:
https://github.com/jackmitch/libsoc



## Authors

* **Juan José Alcaraz** - [jjalcaraz-neuron](https://github.com/jjalcaraz-neuron)
* **Andreas Brunschweiler** - [neuronabr](https://github.com/neuronabr)


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.



[^1]:http://ww1.microchip.com/downloads/en/AppNotes/Atmel-42305-SAM-I2C-SPI-Bootloader_ApplicationNote_AT09002.pdf

[^2]:http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42305-SAM-I2C-SPI-Bootloader_ApplicationNote_AT09002.zip
