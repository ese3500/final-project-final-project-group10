/*
 * 350FinalProject.c
 *
 * Created: 4/3/2023 2:55:11 PM
 * Author : erica
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL

#include "ST7735.h"
#include "LCD_GFX.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

//this will be a problem bc longs are 8 bits
double depth = 0;
double temp = 0; 

long seconds = 0;
long minutes = 0;
int hours = 0;

bool inSafetyStop = 0;
int safetyTimeLeft = 180;

int calibration_coeffs[] = {0, 0, 0, 0, 0, 0};
int assumed_water_density = 1023.6; //kg/m3 for saltwater
double g = 9.80665; //m/s

char str[] = "holder";

void Initialize() {
	lcd_init();

	cli(); //disable global interrupts for setup
	
	//timer 1 setup such that OCA is called once per second
	//set prescaler to 256
	TCCR0B &= ~(1<<CS00);
	TCCR0B &= ~(1<<CS01);
	TCCR0B |= (1<<CS02);
	//sets prescaler to 256
	TIMSK1 |= (1<<OCIE1A);//enables output compare A
	OCR1A = 62500;
	
	setCalibrationCoeffs();
	
	sei();//enable global interrupts

}

ISR(TIMER1_COMPA_vect) {
	OCR1A = (OCR1A + 62500) % 65536;
	seconds ++;
	if (seconds == 60) {
		minutes ++;
		seconds = 0;
		if (minutes == 60) {
			hours ++;
			minutes = 0;
		}
	}
	if (inSafetyStop) {
		if (depth < 15) {
			//some sort of warning
		} else if (depth > 20) {
			inSafetyStop = 0;
			safetyTimeLeft = 180;
		} else if (safetyTimeLeft == 0) {
			//done with safety stop
		} else {
			safetyTimeLeft--;
			//set LEDs accordingly
			//display safety time left to screen
		}
	}
}
void display() {
	sprintf(str, "%l C", temp);
	LCD_drawString(0, 0, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l ft", depth);
	LCD_drawString(0, 50, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l hrs", hours);
	LCD_drawString(50, 0, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l min", minutes);
	LCD_drawString(50, 50, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l s", seconds);
	LCD_drawString(50, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	
}
void setCalibrationCoeffs() { //called once in the beginning
	
}
void measure() { //gets sensor raw values and converts them to depth and temperature
	//TODO: call for D1 conversion
	//TODO: read ADC result
	//TODO: call for D2 conversion
	//TODO: read ADC result
	
	//dummy values so i dont get compiler errors for now:
	double D1 = 0;
	double D2 = 0;
	//the calculations they give us on the datasheet
	double dT = D2 = calibration_coeffs[5] * pow(2, 8);
	temp = 2000 + dT * calibration_coeffs[6] / pow(2, 23);
	double off = calibration_coeffs[2] * pow(2,16) + (calibration_coeffs[4] * dT) / pow(2,7);
	double sens = calibration_coeffs[1] * pow(2, 15) + (calibration_coeffs[3] * dT) / pow(2,8);
	double pressure = (D1 * sens / pow(2, 21) - off) / pow(2,15);
	//conversion to nicer units
	temp /= 100; //temp is now in C
	depth = (pressure * 10000) / assumed_water_density / g;//in meters
	depth = 3.28084 * depth; //depth is now in ft
}

int main(void)
{
	Initialize();
    while (1) {
		display();
    }
}

