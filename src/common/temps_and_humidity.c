//gcc sensors.c -o sensors -l bcm2835 -pthread -lm
#include <stdio.h>
#include <bcm2835.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "temps_and_humidity.h"

// Declarations of functions for the SHT1x
void SHT1x_InitPins( void );
void SHT1x_Reset( void );
void SHT1x_Transmission_Start( void );
unsigned char SHT1x_Mirrorbyte(unsigned char value);
unsigned char SHT1x_Measure_Start( SHT1xMeasureType type );
unsigned char SHT1x_Sendbyte( unsigned char value );
void SHT1x_Crc_Check(unsigned char value);
unsigned char SHT1x_Get_Measure_Value( unsigned short int * value );
unsigned char SHT1x_Readbyte( unsigned char send_ack );
void SHT1x_Calc(float *p_humidity ,float *p_temperature);
void SHT1x_CalcDewpoint(float fRH ,float fTemp, float *fDP);

time_t t;
struct tm tm;
BYTE slave_reg;
BYTE buf[6];
double temp, calc;
unsigned char SHT1x_crc;
unsigned char SHT1x_status_reg = 0;

void SHT1x_InitPins( void ) 
{
    // SCK line as output but set to low first
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    // Set SCK as output
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_SCK, BCM2835_GPIO_FSEL_OUTP);
    // Then, get sure is LOW again
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);

    // DATA to input. External pull up.
    // Set PORT to 0 => pull data line low by setting port as output
    // Set pull up/down off
    bcm2835_gpio_set_pud(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_PUD_OFF);
    // Set DATA to low
    bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA,LOW);
    // Set DATA as output
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set_pud(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_PUD_OFF);
    bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA,LOW);
}


void SHT1x_Reset( void ) 
{
    // Chapter 3.4
    unsigned char i;

    // Enable DATA to HIGH
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT);
    // delay
    delayMicroseconds(2);

    for (i=9; i; i--)
    {
        //Set SCK to HIGH
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
        delayMicroseconds(2);

        // Set SCK to LOW
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
        delayMicroseconds(2);

    }
    SHT1x_Transmission_Start();
    SHT1x_Sendbyte(SHT1x_RESET);  // Soft reset
}

void SHT1x_Transmission_Start( void ) 
{
    // Chapter 3.2
    
    //Set SCK to HIGH
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
    delayMicroseconds(2);
    
    //Set DATA to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA, LOW);
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_OUTP);
    delayMicroseconds(2);
    
    // Set SCK to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    delayMicroseconds(2);
    
    //Set SCK to HIGH
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
    delayMicroseconds(2);
    
    //Set DATA to HIGH
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT);
    delayMicroseconds(2);
    
    // Set SCK to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    delayMicroseconds(2);
    
    // Reset crc calculation. Start value is the content of the status register.
    SHT1x_crc = SHT1x_Mirrorbyte( SHT1x_status_reg & 0x0F );
}

unsigned char SHT1x_Mirrorbyte(unsigned char value) 
{
    unsigned char ret=0, i;

    for (i = 0x80; i ; i >>= 1)
    {
        if(value & 0x01)
            ret |= i;

        value >>= 1;
    }

    return ret;
}

unsigned char SHT1x_Measure_Start( SHT1xMeasureType type ) 
{
    // send a transmission start and reset crc calculation
    SHT1x_Transmission_Start();
    // send command. Crc gets updated!
    return SHT1x_Sendbyte( (unsigned char) type );
}

unsigned char SHT1x_Sendbyte( unsigned char value ) 
{
    unsigned char mask;
    unsigned char ack;

    for(mask = 0x80; mask; mask >>=1)
    {
        // Set SCK to LOW
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
        delayMicroseconds(2);

        if( value & mask )
        {  
            //Set DATA to HIGH
            bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT);
            delayMicroseconds(2);
        }
        else
        {
            //Set DATA to LOW
            bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA, LOW);
            bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_OUTP);
            delayMicroseconds(2);
        }

        //Set SCK to HIGH
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
        delayMicroseconds(2);  // SCK hi => sensor reads data
    }

    // Set SCK to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    delayMicroseconds(2);

    // Release DATA line
    bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT);
    delayMicroseconds(2);
    
    //Set SCK to HIGH
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
    delayMicroseconds(2);

    ack = 0;

    // Get bit
    if(!bcm2835_gpio_lev(RPI_GPIO_SHT1x_DATA))
        ack = 1;

    // Set SCK to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    delayMicroseconds(2);

    SHT1x_Crc_Check(value);   // crc calculation

    return ack;
}


void SHT1x_Crc_Check(unsigned char value) 
{
    unsigned char i;

    for (i=8; i; i--)
    {
        if ((SHT1x_crc ^ value) & 0x80)
        {
            SHT1x_crc <<= 1;
            SHT1x_crc ^= 0x31;
        }
        else
        {
            SHT1x_crc <<= 1;
        }
        value <<=1;
    }
}


unsigned char SHT1x_Get_Measure_Value( unsigned short int * value ) 
{
    unsigned char * chPtr = (unsigned char*) value;
    unsigned char checksum;
    unsigned char delay_count=62;  /* delay is 62 * 5ms */

    /* Wait for measurement to complete (DATA pin gets LOW) */
    /* Raise an error after we waited 250ms without success (210ms + 15%) */
    while( bcm2835_gpio_lev(RPI_GPIO_SHT1x_DATA) )
    {
        delayMicroseconds(5000);            // $$$$$$$$$$$$$$$$$$ 1 ms not working $$$$$$$$$$$$$$$$$$$$$$$$
    
        delay_count--;
        if (delay_count == 0)
            return 0;
    }

    *(chPtr + 1) = SHT1x_Readbyte( 1 );  // read hi byte
    SHT1x_Crc_Check(*(chPtr + 1));          // crc calculation
    *chPtr = SHT1x_Readbyte( 1 );        // read lo byte
    SHT1x_Crc_Check(* chPtr);               // crc calculation

    checksum = SHT1x_Readbyte( 0 );   // crc calculation
    // compare it.
    return SHT1x_Mirrorbyte( checksum ) == SHT1x_crc ? 1 : 0;
}


unsigned char SHT1x_Readbyte( unsigned char send_ack ) 
{
    unsigned char mask;
    unsigned char value = 0;

    // SCK is low here !
    for(mask = 0x80; mask; mask >>= 1 )
    {
        //Set SCK to HIGH
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
        delayMicroseconds(2);  // SCK hi => sensor reads data
        
        if( SHT1x_GET_BIT != 0 )    // and read data
            value |= mask;

        // Set SCK to LOW
        bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
        delayMicroseconds(2);
    }

    /* send ACK if required */
    if ( send_ack )
    {
        //Set DATA to LOW
        bcm2835_gpio_write(RPI_GPIO_SHT1x_DATA, LOW);
        bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_OUTP);
        delayMicroseconds(2);
    }
    
    //Set SCK to HIGH
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, HIGH);
    delayMicroseconds(2);  // give a clock pulse
    
    // Set SCK to LOW
    bcm2835_gpio_write(RPI_GPIO_SHT1x_SCK, LOW);
    delayMicroseconds(2); 
    
    if ( send_ack )
    {   
        // Release DATA line
        //Set DATA to HIGH
        bcm2835_gpio_fsel(RPI_GPIO_SHT1x_DATA, BCM2835_GPIO_FSEL_INPT);
        delayMicroseconds(2);
    }

    return value;
}


//----------------------------------------------------------------------------------------
void SHT1x_Calc(float *p_humidity ,float *p_temperature)
//----------------------------------------------------------------------------------------
{
    const float C1=-2.0468;                 // for 12 Bit
    const float C2=+0.0367;                 // for 12 Bit
    const float C3=-0.0000015955;           // for 12 Bit
    const float T1=+0.01;                   // for 12 Bit
    const float T2=+0.00008;                // for 12 Bit

    const float D1 = -39.66;            // For 3.3 Volt power supply, Centigrade
    const float D2 = 0.01;          // For 14 Bit temperature, Centigrade

    float rh = *p_humidity;                     // rh:      Humidity [Ticks] 12 Bit 
    float t = *p_temperature;               // t:       Temperature [Ticks] 14 Bit
    float rh_lin;                           // rh_lin:  Humidity linear
    float rh_true;                          // rh_true: Temperature compensated humidity
    float t_C;                              // t_C   :  Temperature [C]

    t_C = D1 + (D2 * t);                        // calc. temperature from ticks to [C]
    rh_lin = C1 + (C2 * rh) + (C3 * rh * rh);   // calc. humidity from ticks to [%RH]
    rh_true = (t_C - 25) *(T1 + (T2 * rh)) + rh_lin;// calc. temperature compensated humidity [%RH]
    if(rh_true>100) rh_true=100;            // cut if the value is outside of
    if(rh_true<0.1) rh_true=0.1;            // the physical possible range
    *p_temperature = t_C;                       // return temperature [C]
    *p_humidity = rh_true;                      // return humidity[%RH]
}

//----------------------------------------------------------------------------------------
void SHT1x_CalcDewpoint(float fRH ,float fTemp, float *fDP)
//----------------------------------------------------------------------------------------
// Calculates Dewpoint based on Page 9 (v5, Dec 2011) of SHT1x Datasheet
{
    // Set some constants for the temperature range
    float Tn = 243.12;
    float m = 17.62;
    if (fTemp < 0)
    {
        Tn = 272.62;
        m = 22.46;
    }
    float lnRH = log(fRH/100);
    float mTTnT = (m * fTemp)/(Tn+fTemp);
    
    *fDP = Tn * ((lnRH + mTTnT)/(m - lnRH - mTTnT));
}

// Thread definition for body temperature
double getBodyTemperature( void )
{
	t = time(NULL);
  tm = *localtime(&t);
	// set address
	bcm2835_i2c_setSlaveAddress( _MLX90614_ADDRESS_ );

	slave_reg = _TEMP_OBJ1_;

	double temp = 0; 
	double calc = 0;

	bcm2835_i2c_begin();
	bcm2835_i2c_write (&slave_reg, 1);
	bcm2835_i2c_read_register_rs(&slave_reg, &buf[0], 3);
	temp = (double) (((buf[1]) << 8) + buf[0]);
	temp = (temp * 0.02) - 0.01;
	temp = temp - 273.15;
	calc += temp;
	printf("%04d-%02d-%02d %02d:%02d:%02d, %04.2f degrees\n",
		       tm.tm_year+1900, tm.tm_mon +1, tm.tm_mday,
		       tm.tm_hour, tm.tm_min, tm.tm_sec, calc);
	return calc;
}


void getTempAndHumidity( tempAndHumi_t *tempAndHumiVals )
{
	t = time(NULL);
  tm = *localtime(&t);
	unsigned char noError;  
	value humi_val,temp_val;

	noError = 1;

	// Request Temperature measurement
	noError = SHT1x_Measure_Start( SHT1xMeaT );
	if (!noError) 
	{
	    printf("ERROR: Cannot request temperature measurement\n");
	    return;
	}

	// Read Temperature measurement
	noError = SHT1x_Get_Measure_Value( (unsigned short int*) &temp_val.i );
	if (!noError)
	{
	    printf("ERROR: Cannot read temperature\n");
	    return;
	}

	// Request Humidity Measurement
	noError = SHT1x_Measure_Start( SHT1xMeaRh );
	if (!noError)
	{
	    printf("ERROR: Cannot request humidity measurement\n");
	    return;
	}
	    
	// Read Humidity measurement
	noError = SHT1x_Get_Measure_Value( (unsigned short int*) &humi_val.i );
	if (!noError)
	{
	    printf("ERROR: Cannot read humidity\n");
	    return;
	}

	// Convert intergers to float and calculate true values
	temp_val.f = (float)temp_val.i;
	humi_val.f = (float)humi_val.i;

	// Calculate Temperature and Humidity
	SHT1x_Calc(&humi_val.f, &temp_val.f);

	//Calculate and print the Dew Point
	float fDewPoint;
	SHT1x_CalcDewpoint(humi_val.f ,temp_val.f, &fDewPoint);


	printf("HUMID[%04d-%02d-%02d %02d:%02d:%02d]: %04.2f percent\n",
		      tm.tm_year+1900, tm.tm_mon +1, tm.tm_mday,tm.tm_hour, tm.tm_min, 
		      tm.tm_sec, humi_val.f);

	printf("DEW_POINT[%04d-%02d-%02d %02d:%02d:%02d]: %04.2f degrees\n",
		      tm.tm_year+1900, tm.tm_mon +1, tm.tm_mday,tm.tm_hour, tm.tm_min, 
		      tm.tm_sec, fDewPoint);

	printf("TEMP[%04d-%02d-%02d %02d:%02d:%02d]: %04.2f degrees\n",
		      tm.tm_year+1900, tm.tm_mon +1, tm.tm_mday,tm.tm_hour,
		      tm.tm_min, tm.tm_sec, temp_val.f);

	tempAndHumiVals->envTemp  = temp_val.f;
	tempAndHumiVals->humidity = humi_val.f; 
}

