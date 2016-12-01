#include <stdio.h>
#include <inttypes.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include "ADS1015.h"
#include "Pulse_Oximeter.h"

float getPulse(int pulseOximeterFd)
{
	int measure_ok = 0;
	int fProm = 0;
	int fMin, meas, fMax;
	int fMin_f = 0;
	int fMax_f = 0;
	struct timeval start, stop;
	float avrg_seconds = 0;
	long microseconds = 0;
	float seconds = 0;
	int x,i;
	    

	printf("-----------Obteniendo Freq Minima y Maxima-----------\n");
	for(x = 5; x > 0; x--)
	{
	  fMin = 3000;
	  fMax = 0;
	  measure_ok = 0;
	  for(i = 500; i > 0; i-- )
	  {
	    meas = readADC_SingleEnded(pulseOximeterFd, OXIMETER_ADS_CHANNEL);
	    if ( fMin > meas )
	    {
	        fMin = meas;
	    }
	    
	    if ( fMax < meas )
	    {
	        fMax = meas;
	    }
	  }
	  if((fMax-fMin) > 900)
	  {
	    printf("Do not move! Repeating test...\n");
	    x++;
	  }
	  else if ((fMax - fMin) > 200)
	  {
	    measure_ok = 1;
	    fMin_f += fMin/5;
	    fMax_f += fMax/5; 
	  }
	  else
	  {
	    printf("Error in Measurement! Repeating test...\n");
	    x++;
	  }
	  delay(100);

	}
	fProm += (fMax-fMin)/2;
	printf("Frecuencia Minima= %d\n", fMin_f);
	printf("Frecuencia Maxima= %d\n", fMax_f);
	printf("Frecuencia Promedio = %d\n", fProm);

	printf("---------------Obteniendo Pulso----------------\n");
	double final_time = 0;
	for(x = 3; x > 0; x--)
	{
	    do{
	        meas = readADC_SingleEnded(pulseOximeterFd, OXIMETER_ADS_CHANNEL);
	    }while(meas > (fProm + fMin_f));
	    
	    while( meas < ( fMax_f - fProm/3 ) )
	    {
	        meas = readADC_SingleEnded(pulseOximeterFd, OXIMETER_ADS_CHANNEL); 
	    }
	    gettimeofday(&start, NULL);
	    while(meas > (fProm + fMin_f))
	    {
	        meas = readADC_SingleEnded(pulseOximeterFd, OXIMETER_ADS_CHANNEL);
	    }
	    while( meas < ( fMax_f - fProm/3 ) )
	    {
	        meas = readADC_SingleEnded(pulseOximeterFd, OXIMETER_ADS_CHANNEL); 
	    }
	    gettimeofday(&stop, NULL);
	    microseconds = (((stop.tv_sec * 1000000 + stop.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
	    seconds = (float)microseconds / 1000000.00; 
	    if( seconds < 0.3)
	    {
	        printf("Error en timming, muy pequeño\n");
	        x++;
	    }
	    else if(seconds > 2)
	    {
	        printf("Error en timming, muy grande\n");
	        x++;
	    }
	    else
	    {
	        printf("Seconds = %f\n", seconds);
	        avrg_seconds += seconds; 
	    }
	           
	}
	avrg_seconds = avrg_seconds/3.0;
	printf("Average Seconds= %f\n", avrg_seconds);
	printf("PPM= %f\n", (60 / avrg_seconds));
	return (60 / avrg_seconds);
}
