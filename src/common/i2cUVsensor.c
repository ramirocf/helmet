#include <stdio.h>
#include <inttypes.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include "i2cUVsensor.h"

int getUV(int UV_fd)
{
  int meas;
  meas = readADC_SingleEnded(UV_fd, UV_ADS_CHANNEL);
	return meas;
}