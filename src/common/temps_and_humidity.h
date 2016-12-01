#ifndef __TEMP_HUM_H__
#define __TEMP_HUM_H__

// Defining offsets for different temperatures
#define _TEMP_AMBIENT_ 6
#define _TEMP_OBJ1_ 7
#define _TEMP_OBJ2_ 8
#define _MLX90614_ADDRESS_ 0x5a


/* Macros to toggle port state of SCK line. */
#define SHT1x_SCK_LO    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW)
#define SHT1x_SCK_HI    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH)
#define SHT1x_DATA_LO   bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA, LOW);\
                        bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_OUTP)
#define SHT1x_DATA_HI   bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT)
#define SHT1x_GET_BIT   bcm2835_gpio_lev(RPI_GPIO_SHT1x_DATA)
#define SHT1x_RESET 0x1E            // Perform a sensor soft reset.

// Define the Raspberry Pi GPIO Pins for the SHT1x
#define RPI_GPIO_SHT1x_SCK RPI_GPIO_P1_16
#define RPI_GPIO_SHT1x_DATA RPI_GPIO_P1_18

/* Definitions of all known SHT1x commands */
#define SHT1x_MEAS_T    0x03            // Start measuring of temperature.
#define SHT1x_MEAS_RH   0x05            // Start measuring of humidity.



typedef unsigned char BYTE;

/* Enum to select between temperature and humidity measuring */
typedef enum _SHT1xMeasureType {
    SHT1xMeaT   = SHT1x_MEAS_T,     // Temperature
    SHT1xMeaRh  = SHT1x_MEAS_RH     // Humidity
} SHT1xMeasureType;

typedef union 
{
    unsigned short int i;
    float f;
} value;

typedef struct tempAndHumi_t {
  float envTemp;
  float humidity;
} tempAndHumi_t;

// Declaration of functions to be used in threads
double getBodyTemperature( void );
void getTempAndHumidity( tempAndHumi_t *tempAndHumiVals );

#endif // __TEMP_HUM_H__