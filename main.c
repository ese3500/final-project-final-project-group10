/*
 * 350FinalProject.c
 *
 * Created: 4/3/2023 2:55:11 PM
 * Author : erica
 */ 

#include <avr/io.h>
#define F_CPU 16000000UL

#include "ST7735.h"
#include "MS5803.h"
#include "LCD_GFX.h"
#include "Buhlman.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

// current dive parameters
float depth = 0;
float pressure = 0;
int temp = 0; 
int maximumDepth = 0;
bool inSafetyStop = 0;
bool requiresSafetyStop = 0;
int safetyTimeLeft = 0;
int previousDepthReading = 0;
bool alertingAscentRate = 0;
bool alertingNoStopTime = 0;
int noStopTime = 100; //dummy start value so it doesn't start at 0

//timing, since current dive started
long seconds = 0;
long minutes = 0;
int hours = 0;
int partialSecond = 0;

//mode settings
bool currentlyDiving = 1;

//dive-related constants
int safetyStopTime = 180;
int requireSafetyStopAfterDepth = 30;
int safetyStopLowDepth = 20;
int safetyStopHighDepth = 15;

//sensor reading related constants
uint16_t calibration_coeffs[6];
//uint16_t serialCodeAndCRC;
int water_density = 1023.6; //kg/m3 for saltwater
double g = 9.80665; //m/s

char str[] = "holder";
int timerInterruptsPerSecond = 1;

void Initialize() {

	cli(); //disable global interrupts for setup
	
	lcd_init();
	reset();//for sensor

	//setup for active buzzer at pin 2
	DDRD |= (1<<DDD2); //set buzzer to output pin
	PORTD |= (1<<PORTD2); // set initially to low
	
	//setup output LED at pin 3
	DDRD |= (1<<DDD3); //set buzzer to output pin
	PORTD |= (1<<PORTD3); // set initially to low
	
	//timer 1 setup such that OCA is called as many times per second as timerInterruptsPerSecond
	//set prescaler to 256
	TCCR0B &= ~(1<<CS00);
	TCCR0B &= ~(1<<CS01);
	TCCR0B |= (1<<CS02);
	//sets prescaler to 256
	TIMSK1 |= (1<<OCIE1A);//enables output compare A
	OCR1A = 62500/timerInterruptsPerSecond; //should be 62500 for 1s
	
	setCalibrationCoeffs();
	
	sei();//enable global interrupts

}

// called every half second
ISR(TIMER1_COMPA_vect) {
	//TODO: move everything out of the interrupt, just set a flag to run this
	partialSecond ++;
	OCR1A = (OCR1A + (62500/timerInterruptsPerSecond)) % 65536;
	if (!currentlyDiving) {
		measure();
		return;
	}
	if (partialSecond == timerInterruptsPerSecond) {
		partialSecond = 0;
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
			if (depth <= safetyStopLowDepth && depth >= safetyStopHighDepth) {
				safetyTimeLeft --;
			}
			if (safetyTimeLeft == 0) {
				inSafetyStop = 0;
			}
		}
	}
	measure();
	
	if (alertingNoStopTime) {
		//turn on buzzer
		PORTD &= ~(1<<PORTD2);
		//turn on LED
		PORTD &= ~(1<<PORTD3);
	} else if (alertingAscentRate) {
		//turn on buzzer
		PORTD &= ~(1<<PORTD2);
		//turn off LED
		PORTD |= (1<<PORTD3);
	} else {
		//turn both off
		PORTD |= (1<<PORTD2);
		PORTD |= (1<<PORTD3);
	}
}

void setCalibrationCoeffs() { //called once in the beginning
	for (int i = 0; i < 6; i++) {
		//TODO: update
		calibration_coeffs[i] = getPROMVal(0x76, i+1);
	}
	//serialCodeAndCRC = readProm(7);
}
void measure() { //gets sensor raw values and converts them to depth and temperature
	previousDepthReading = depth;
	
	//TODO: update
	unsigned int D1 = getPressure(0x76);
	unsigned int D2 = getTemperature(0x76);
	
	//dummy values so i dont get compiler errors for now:
	//unsigned int D1 = 0;
	//unsigned int D2 = 0;

	//the calculations they give us on the datasheet
	int dT = D2 - calibration_coeffs[5] * pow(2, 8);
	int temp = 2000 + dT * calibration_coeffs[6] / pow(2, 23);
	long off = calibration_coeffs[2] * pow(2,16) + (calibration_coeffs[4] * dT) / pow(2,7);
	long sens = calibration_coeffs[1] * pow(2, 15) + (calibration_coeffs[3] * dT) / pow(2,8);
	pressure = (D1 * sens / pow(2, 21) - off) / pow(2,15); //in 0.1mBar
	pressure /= 10; //in mBar
	pressure *= 100000; //in Pa
	temp /= 100; //temp is now in C
	depth = calculateDepthFromPressure(pressure);
	handleNewMeasurements();
}

void handleNewMeasurements() {
	if (!currentlyDiving) {
		if (depth < 4) {
			return;
		}
		//start dive at 4 ft at set time to 0
		currentlyDiving = 1;
		hours = 0;
		minutes = 0;
		seconds = 0;
		maximumDepth = depth;
	}
	if (depth > maximumDepth) {
		maximumDepth = depth;
	}
	if (!requiresSafetyStop && depth > requireSafetyStopAfterDepth) {
		requiresSafetyStop = 1;
	}
	if (requiresSafetyStop && depth <= safetyStopLowDepth) {
		requiresSafetyStop = 0;
		inSafetyStop = 1;
		safetyTimeLeft = safetyStopTime;
	}
	if (previousDepthReading > depth) { //ascending
		if (requiresSafetyStop && depth <= safetyStopLowDepth) {
			inSafetyStop = 1;
		}
		if (previousDepthReading - depth > 1) {
			alertingAscentRate = 1;
		}
	}
	if (previousDepthReading != depth) {
		advanceCalculations(previousDepthReading, pressure, seconds + 60 * minutes + 60 * 60 * hours);
	}
	if (alertingAscentRate && previousDepthReading - depth <= 1) {
		alertingAscentRate = 0;
	}
	noStopTime = calculateNoStopTime();
	if (noStopTime <= 0) {
		alertingNoStopTime = 1;
	}
	if (alertingNoStopTime && noStopTime > 0) {
		alertingNoStopTime = 0;
	}
}

int calculateNoStopTime() {
	return getNoDecoStopMinutes(pressure);
}


void displayDiveScreen() {
	sprintf(str, "%l C", temp);
	LCD_drawString(0, 0, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l ft", depth);
	if (alertingAscentRate) {
		LCD_drawString(0, 10, str, rgb565(255, 0, 0), rgb565(255, 255, 255));
	} else {
		LCD_drawString(0, 10, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	}
	sprintf(str, "%l pressure", pressure);
	LCD_drawString(0, 20, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l hrs", hours);
	LCD_drawString(0, 30, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l min", minutes);
	LCD_drawString(0, 40, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l s", seconds);
	LCD_drawString(0, 50, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%l max ft", maximumDepth);
	LCD_drawString(0, 60, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, "%u NST", noStopTime);
	if (alertingNoStopTime) {
		LCD_drawString(0, 70, str, rgb565(255, 0, 0), rgb565(255, 255, 255));
	} else {
		LCD_drawString(0, 70, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	}
}

int main(void)
{
	Initialize();
    while (1) {
		displayDiveScreen();
    }
}

