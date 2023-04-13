#include <avr/io.h>

void i2cinit();

void i2cStart();

void i2cStop();

uint8_t i2cWrite(uint8_t data);


uint8_t i2cReadAck();

uint8_t i2cStatus();
