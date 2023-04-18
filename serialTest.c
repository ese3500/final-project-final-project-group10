/*
 * 350finalProjectTest.c
 *
 * Created: 4/14/2023 11:15:02 AM
 * Author : erica
 */ 

#include <avr/io.h>
#include "MS5803.h"
#include "i2c.h"
#include <util/delay.h>

int main(void)
{
    /* Replace with your application code */
	i2cinit();
    while (1) 
    {
		i2cStart();
		uint8_t addr = 0b1110110;
		i2cWrite(addr);
		i2cStop();
		_delay_ms(1000);
    }
}

