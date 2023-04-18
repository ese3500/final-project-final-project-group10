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
#include <avr/io.h>
#define F_CPU 16000000UL

#include "ST7735.h"
#include "MS5803.h"
#include "LCD_GFX.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>

uint16_t calibration_coeffs[6];
uint16_t serialCodeAndCRC;
char str[] = "holder";

void Initialize() {
	i2cinit();
	lcd_init();
	reset();//for sensor
}
void setCalibrationCoeffs() { //called once in the beginning
	for (int i = 0; i < 6; i++) {
		calibration_coeffs[i] = readProm(i+1);
	}
	serialCodeAndCRC = readProm(7);
}

int main(void)
{

    while (1) 
    {
		unsigned int D1 = readPressure();
		unsigned int D2 = readTemperature();

		//the calculations they give us on the datasheet
		int dT = D2 - calibration_coeffs[5] * pow(2, 8);
		int temp = 2000 + dT * calibration_coeffs[6] / pow(2, 23);
		long off = calibration_coeffs[2] * pow(2,16) + (calibration_coeffs[4] * dT) / pow(2,7);
		long sens = calibration_coeffs[1] * pow(2, 15) + (calibration_coeffs[3] * dT) / pow(2,8);
		int pressure = (D1 * sens / pow(2, 21) - off) / pow(2,15);
		sprintf(str, "%l temp", temp);
		LCD_drawString(0, 0, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
		sprintf(str, "%l pressure", pressure);
		LCD_drawString(0, 20, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
		_delay_ms(1000);
    }
}

