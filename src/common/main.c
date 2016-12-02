#include <stdio.h>
#include <pthread.h>
#include <bcm2835.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "temps_and_humidity.h"
#include "Pulse_Oximeter.h"
#include "i2cUVsensor.h"
#include "ADS1015.h"

// Additional one is for the null character
#define PPM_SIZE    ( 3 + 1 )
#define UV_SIZE     ( 4 + 1 )
#define BODY_SIZE   ( 3 + 1 )
#define ENV_SIZE    ( 3 + 1 )
#define HUMI_SIZE   ( 4 + 1 )
#define TX_MSG_SIZE ( PPM_SIZE + 1 + UV_SIZE + 1 + BODY_SIZE + 1 + ENV_SIZE + 1 + HUMI_SIZE )

// Attributes
float pulsePerMinute;
int uvIntensity;
double bodyTemperature;
float envTemperature;
float humidity;

// Initialize condition variables
pthread_mutex_t count_mutex     = PTHREAD_MUTEX_INITIALIZER;

void *BodyTemp_Sensor(void *p) {
	while(1) {

	bodyTemperature = 0;
	// Only one sensor can be using the I2C pins at a time
	pthread_mutex_lock( &count_mutex );
	
  bodyTemperature = getBodyTemperature();

  pthread_mutex_unlock( &count_mutex );
  bcm2835_delay(2000);
	}
}

void *TempAndHumidity_Sensor(void *p) {
  tempAndHumi_t tempAndHumiVals;

  humidity = 0;
  envTemperature = 0;

	while(1) {
	pthread_mutex_lock( &count_mutex );
	
  getTempAndHumidity(&tempAndHumiVals);
  humidity = tempAndHumiVals.humidity;
  envTemperature = tempAndHumiVals.envTemp;

  pthread_mutex_unlock( &count_mutex );
  bcm2835_delay(2000);		
	}
}

void *pulseOximeter_Sensor(void *p) {
   int *ads1015_Fd = (int*)p;
   pulsePerMinute = 0;

   while(1) {
   	pthread_mutex_lock( &count_mutex );

   	pulsePerMinute = getPulse(*ads1015_Fd);
   	
    pthread_mutex_unlock( &count_mutex );
   	bcm2835_delay(2000);
   }
}

void *UV_Sensor(void* p) {
	int *ads1015_Fd = (int *)p;
  uvIntensity = 0;
  
  while (1) {
  	pthread_mutex_lock( &count_mutex );
    
    uvIntensity = getUV(*ads1015_Fd);
  
    pthread_mutex_unlock( &count_mutex );
    bcm2835_delay(2000);
  }

}

void *bluetoothTx(void *p) {
	int *bluetoothFd = (int*) p;
	int count;
	char *ppmStr  = malloc(sizeof(char) * PPM_SIZE);
	char *uvStr   = malloc(sizeof(char) * UV_SIZE);
	char *bodyStr = malloc(sizeof(char) * BODY_SIZE);
	char *envStr = malloc(sizeof(char) * ENV_SIZE);
	char *humiStr = malloc(sizeof(char) * HUMI_SIZE);
	char *txMsg   = malloc(sizeof(char) * TX_MSG_SIZE + 1);

	while (1) {
		snprintf(ppmStr,  PPM_SIZE,  "%d", (int)pulsePerMinute);
		snprintf(uvStr,   UV_SIZE,   "%d", (int)uvIntensity);
		snprintf(bodyStr, BODY_SIZE, "%d", (int)bodyTemperature);
		snprintf(envStr,  ENV_SIZE,  "%d", (int)envTemperature);
		snprintf(humiStr, HUMI_SIZE, "%d", (int)humidity);

		snprintf(txMsg, TX_MSG_SIZE+1, "p%su%sb%se%sh%s", 
			       ppmStr, uvStr, bodyStr, envStr, humiStr);
    count = write(*bluetoothFd, txMsg, TX_MSG_SIZE);
    if (count < 0) {
    	printf("Error in bluetooth communication\n");
    }
    bcm2835_delay(750);
	}
}


int main() 
{
  int fail;
  // File descriptor the ADC 
  int ads1015_Fd;
  int bluetoothFd;
	// Declare threads
	pthread_t bodyTempThd;
	pthread_t tempAndHumdThd;
	pthread_t pulseThd;
	pthread_t uvThd;
	pthread_t bluetoothThd;
  struct termios options;
 
  /**************************************
  I2C initialization
  ***************************************/
	/*
	if (!bcm2835_init())
	{
	  printf("Cannot initialize bcm2835\n");
	  return 1;
	}

  if (!bcm2835_i2c_begin())
  {
    printf("bcm2835_i2c_begin failed. Are you running as root??\n");
	  return 1;
  }
	bcm2835_i2c_set_baudrate(25000);
	// Pulse Oximeter and UV sensor use the ADS1015 to get measurements.
  // Since ADC design is using wiringpi, and that API encapsultates setup and intialization
  // in one step, we cannot afford the luxury of doing i2c initialization every time the 
  // function is called
	ads1015_Fd = wiringPiI2CSetup(ADS1015_ADDRESS);
	if (ads1015_Fd < 0) {
		printf("Pulse Oximeter sensor not properly initialized\n");
		return 1;
	}*/
  /*****************************************
  * Bluetooth initialization
  ******************************************/
  // Open port
	bluetoothFd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
	if (bluetoothFd == -1){
		printf("Unable to open Serial Bluetooth Port /dev/ttyAMA0\n");
		return -1;
	} 
	/*****************
	* Initialization of communication flags
	*****************/
	// Set communication parameters
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; // Baud rate and word size
	options.c_iflag = IGNPAR;                       // Ignore parity
	options.c_oflag = 0;                            // 
	options.c_lflag = 0;                            //
	tcflush(bluetoothFd, TCIFLUSH);
	tcsetattr(bluetoothFd, TCSANOW, &options);
  /*****************************************
  SHT1x Sensor initialization
  ******************************************/
	// Set up the SHT1x Data and Clock Pins
	//SHT1x_InitPins();

	// Reset the SHT1x
	//SHT1x_Reset();

  /*****************************************
  Threads creation
  ******************************************/
  /*
	fail = pthread_create( &bodyTempThd, NULL, &BodyTemp_Sensor, NULL);
	if (fail)
	{
	    printf("Error creating thread for Body Temperature Sensor\n");
	    return 1;
	}

	fail = pthread_create( &tempAndHumdThd, NULL, &TempAndHumidity_Sensor, NULL);
	if (fail)
	{
	    printf("Error creating thread for enviromental Temperature and Humidity Sensor\n");
	    return 1;
	}

	fail = pthread_create(&pulseThd, NULL, &pulseOximeter_Sensor, &ads1015_Fd);
	if (fail) {
		printf("Error creating thread for pulse Oximeter Sensor\n");
		return 1;
	}

	fail = pthread_create(&uvThd, NULL, &UV_Sensor, &ads1015_Fd);
	if (fail) {
		printf("Error creating thread for UV Intensity sensor\n");
		return 1;
	}
  */
	fail = pthread_create(&bluetoothThd, NULL, &bluetoothTx, &bluetoothFd);
	if (fail) {
		printf("Error creating thread for bluetooth\n");
	}
  /*
	pthread_join(bodyTempThd, NULL);
	pthread_join(tempAndHumdThd, NULL);
	pthread_join(pulseThd, NULL);
	pthread_join(uvThd, NULL);
	*/
	pthread_join(bluetoothThd, NULL);
	bcm2835_i2c_end();
	return 0;
}