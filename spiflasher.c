#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libsoc_debug.h"
#include "libsoc_gpio.h"
#include "libsoc_spi.h"

#define GPIO_NRST   35
#define GPIO_NCHG   47
#define SPI_DEVICE   1
#define CHIP_SELECT  0 

static uint8_t GET_EXT_STATUS_CMD[] = {0xFF, 0xFF, 0xFF};
static uint8_t UNLOCK_CMD[]         = {0xDC, 0xAA};
static uint8_t GET_STATUS_CMD[]     = {0xFF};
static const uint8_t STATUS_CHECKING_CRC = 0x02;
static const uint8_t STATUS_CRC_OK       = 0x04;
static const uint8_t RC_OK    = 0;
static const uint8_t RC_ERROR = 1;
static spi* spi_dev;
gpio *gpio_nchg;
#define MAX_FRAME_SIZE 532

static void startBootloader()
{
    printf("Starting bootloader\n");
    gpio *gpio_output = libsoc_gpio_request(GPIO_NRST, LS_SHARED);
    libsoc_gpio_set_direction(gpio_output, OUTPUT);
    libsoc_gpio_set_level(gpio_output, HIGH);
    usleep(50000);
    for (int i=0; i < 10; i++)
    {
        libsoc_gpio_set_level(gpio_output, LOW);
        usleep(2000);
        libsoc_gpio_set_level(gpio_output, HIGH);
        usleep(60000);
    }
    libsoc_gpio_free(gpio_output);
    usleep(200000);
}

static void waitNCHG()
{
    while(libsoc_gpio_get_level(gpio_nchg) == HIGH)
    {
        usleep(100);
    }
}

static void printExtendedStatus()
{
    waitNCHG();
    uint8_t status_ext[] = {0x00, 0x00, 0x00};
    //Read extended status (3 bytes)
    //Out:[0xFF, 0xFF, 0xFF]
    //In: [status, id, version]

    libsoc_spi_rw(spi_dev, GET_EXT_STATUS_CMD, 
            status_ext, sizeof(GET_EXT_STATUS_CMD));
    printf("Extended status = %02x, id = %02x, version = %02x\n", 
            status_ext[0], status_ext[1], status_ext[2]);
}

static uint8_t getStatus()
{
    waitNCHG();
    uint8_t status = 0x00;
    libsoc_spi_rw(spi_dev, GET_STATUS_CMD, &status, sizeof(GET_STATUS_CMD));
    return status;
}

int upgradeApp(const char* fwFileName)
{
    uint8_t status = 0x00;
    libsoc_spi_rw(spi_dev, GET_STATUS_CMD, &status, sizeof(GET_STATUS_CMD));
    printf("Initial status 0x%02X\n", status);

    startBootloader();
    printExtendedStatus();
    //
    //--State bootloader
    //Unlock the device: 
    //Out:[0xDC, 0xAA]
    //In: [?, ?]

    printf("Send unlock command\n");
    libsoc_spi_write(spi_dev, UNLOCK_CMD, sizeof(UNLOCK_CMD));
    status = getStatus();

    //--State application update
    //Update the application:
    //Out:[lengthA,lengthB,data0, data1....]
    //
    //
    //
    uint8_t frame[MAX_FRAME_SIZE];
    FILE *f;
    f= fopen(fwFileName,"rb");  // r for read, b for binary
    if (!f)
    {
        fprintf(stderr,"Cannot open %s\n", fwFileName);
        return RC_ERROR;
    }
    fseek(f, 0L, SEEK_END);
    int fileSize = ftell(f);
    rewind(f);
    int byteFramePos = 0;
    int bytesTransmitted = 0;
    uint16_t frameSize = MAX_FRAME_SIZE;
    printf("Write data...\n");
    while(fread(frame + byteFramePos,1,1,f)) // read 1 byte to our buffer
    {
        if (byteFramePos == 1)
        {
            frameSize = frame[0] << 8 | frame[1];
            frameSize += 2;
            if (frameSize > MAX_FRAME_SIZE)
            {
                fprintf(stderr,"frame size too large\n");
                return RC_ERROR;
            }
        }
        byteFramePos++;
        if (byteFramePos == frameSize)
        {
            libsoc_spi_write(spi_dev, frame, frameSize);
            bytesTransmitted += frameSize;
            status = getStatus();
            while (status != STATUS_CHECKING_CRC)
            {
                fprintf(stderr,"unexpected state 0x%02X (expected 0x%02X: checking crc)\n", 
                        status, STATUS_CHECKING_CRC);
                return RC_ERROR;
            }
            status = getStatus();
            if (status != STATUS_CRC_OK)
            {
                fprintf(stderr,"unexpected state 0x%02X (expected 0x%02X: crc ok)\n", 
                        status, STATUS_CRC_OK);
                return RC_ERROR;
            }
            if (bytesTransmitted < fileSize)
            {
                status = getStatus();
            }

            byteFramePos = 0;
        }
    }
    if (bytesTransmitted != fileSize)
    {
        fprintf(stderr,"Transfer failed, completed %d bytes out of %d\n", bytesTransmitted, fileSize);
        return RC_ERROR;
    }
    printf("Application updated\n");
    fclose(f);
    return RC_OK;
}

void usage()
{
    fprintf(stderr,"Usage:\n");
    fprintf(stderr,"spiflasher /path/to/firmwareFile.bin\n");
}

int main(int argc, const char ** argv) {

    if (argc != 2)
    {
        usage();
        return EXIT_FAILURE;
    }
    const char* fwFileName = argv[1];

    spi_dev = libsoc_spi_init(SPI_DEVICE, CHIP_SELECT);
    gpio_nchg = libsoc_gpio_request(GPIO_NCHG, LS_SHARED);
    libsoc_gpio_set_direction(gpio_nchg, INPUT);

    if (!spi_dev)
    {
        fprintf(stderr,"Failed to get spidev. Device %d chip %d.\n ", SPI_DEVICE, CHIP_SELECT);
        return EXIT_FAILURE;
    }
    libsoc_spi_set_mode(spi_dev, MODE_1);
    libsoc_spi_set_speed(spi_dev, 1000000);
    libsoc_spi_set_bits_per_word(spi_dev, BITS_8);

    int rc = upgradeApp(fwFileName);
    libsoc_gpio_free(gpio_nchg);
    libsoc_spi_free(spi_dev);
    return rc == RC_OK? EXIT_SUCCESS : EXIT_FAILURE;
}
