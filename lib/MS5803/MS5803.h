#ifndef MS5803_H
#define MS5803_H
#include <stdint.h>

uint16_t getPROMVal(unsigned char TWI_targetSlaveAddress, int promVal);
uint32_t getPressure(unsigned char TWI_targetSlaveAddress); 
uint32_t getTemperature(unsigned char TWI_targetSlaveAddress);

#endif