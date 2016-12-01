#include <inttypes.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "ADS1015.h"

uint16_t readADC_SingleEnded(int fd, uint8_t channel) {
  if (channel > 3)
  {
    return 0;
  }
  
  int m_bitShift = 4;
  
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= ADS1015_REG_CONFIG_PGA_6_144V;            // +/- 6.144V range (limited to VDD +0.3V max!)

  // Set single-ended input channel
  switch (channel)
  {
    case (0):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
      break;
    case (1):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
      break;
    case (2):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
      break;
    case (3):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
      break;
  }

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;
  config = ntohs(config);
  // Write config register to the ADC
  wiringPiI2CWriteReg16(fd, ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  delay(ADS1015_CONVERSIONDELAY);

  // Read the conversion results
  // Shift 12-bit results right 4 bits for the ADS1015
  return ntohs(wiringPiI2CReadReg16(fd, ADS1015_REG_POINTER_CONVERT) >> m_bitShift);  
}