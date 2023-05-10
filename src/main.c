#include <avr/io.h>

#include "twi.h"
#include <util/delay.h>

#include "uart.h"
#include <avr/interrupt.h>

#include <LCD_GFX.h>
#include <ST7735.h>

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include <pff.h>
#include <string.h>

#include <avr_mmcp.h>
#include <MS5803.h>

#include <Buhlman.h>

#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)

unsigned char messageBuf[8];
unsigned char TWI_targetSlaveAddress; 

// int temperature = 0;
// char str[] = "holder";

// int waterSensing = 1;

UINT w;
FATFS fs;

unsigned char messageBuf[8];
unsigned char TWI_targetSlaveAddress; 

int temperature = 0;
char strr[] = "holder";

//sensor reading related constants
uint16_t calibration_coeffs[6];
//uint16_t serialCodeAndCRC;
int water_density = 1023.6; //kg/m3 for saltwater
double g = 9.80665; //m/s

// current dive parameters
float depth = 0;
int pressure = 0;
int temp = 0; 
int maximumDepth = 0;
bool inSafetyStop = 0;
bool requiresSafetyStop = 0;
int safetyTimeLeft = 0;
float previousDepthReading = 0;
bool alertingAscentRate = 0;
bool alertingNoStopTime = 0;
int noStopTime = 100; //dummy start value so it doesn't start at 0

//timing, since current dive started
int seconds = 0;
int minutes = 0;
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

// char str[] = "holder";
int timerInterruptsPerSecond = 1;
int run_timer_update = 0;


// First dive settings
int gotWet = 0;
int diving = 0;
int logWritten = 0;

int numCalls = 0;


char divelog[215];

void setCalibrationCoeffs() { //called once in the beginning
	for (int i = 0; i < 6; i++) {
		calibration_coeffs[i] = getPROMVal(0x76, i+1);
	}
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
	noStopTime = getNoDecoStopMinutes(pressure);
	if (noStopTime <= 90) {
		alertingNoStopTime = 1;
	}
	if (alertingNoStopTime && noStopTime > 90) {
		alertingNoStopTime = 0;
	}
} 


void draw() {
	sprintf(strr, "%i C", temp);
	LCD_drawString(20, 20, strr, rgb565(0, 0, 0), rgb565(255, 255, 255));

	sprintf(strr, "%i mbar", pressure);
	LCD_drawString(20, 40, strr, rgb565(0, 0, 0), rgb565(255, 255, 255));

	sprintf(strr, "%i ft", (int) depth);
	LCD_drawString(20, 70, strr, rgb565(0, 0, 0), rgb565(255, 255, 255));

	sprintf(strr, "%i S nostop", noStopTime);
	LCD_drawString(20, 100, strr, rgb565(0, 0, 0), rgb565(255, 255, 255));
}

void measure() { //gets sensor raw values and converts them to depth and temperature
	// previousDepthReading = depth;
	
	//TODO: update
	uint32_t D1 = getPressure(0x76);
	uint32_t D2 = getTemperature(0x76);
	char buf[30];
	
	int64_t dT = D2 - ((int64_t)(calibration_coeffs[4]) * (1L << 8));

	int32_t tempp = 2000 + ((int32_t)dT * (int32_t)calibration_coeffs[5]) / (1L << 23); //(((dT * (((int64_t)calibration_coeffs[5]))) / (1L<<23))/100);

	temp = (int) tempp;

	int64_t off = ((int64_t)calibration_coeffs[1]) * (1L << 16) + (((int64_t)calibration_coeffs[3]) * dT) / (1L << 7);

	int64_t sens = ((int64_t)calibration_coeffs[0]) * (1L << 15) + (((int64_t)calibration_coeffs[2]) * dT) / (1L << 8);


	int32_t press = ((D1 * sens / (1L << 21)) - off) / (1L << 15); //in 0.1mBar


	press /= 10; //in mBar

	// WARNING: 20x pressure for demonstration purposes
	pressure = (int) press;
	temp /= 100; //temp is now in C

	depth = calculateDepthFromPressure(pressure);

	int amtTime = (3600 * hours) + (60 * minutes) + seconds;

	sprintf(buf, "%d,%d\n", amtTime, (int) depth);
	// pf_write(buf, strlen(buf), &w);
	strcat(divelog, buf);
	
	handleNewMeasurements();
}

void runTimerUpdate() {
	if (!currentlyDiving) {
		measure();
		return;
	}
	
	
	measure();
	
	if (alertingNoStopTime) {
		//turn on buzzer
		PORTD |= (1<<PORTD2);
		//turn on LED
		PORTD |= (1<<PORTD3);
	} else if (alertingAscentRate) {
		//turn on buzzer
		PORTD |= (1<<PORTD2);
		//turn off LED
		PORTD |= (1<<PORTD3);
	} else {
		//turn both off
		PORTD &= ~(1<<PORTD2);
		PORTD &= ~(1<<PORTD3);
	}
}

void initDraw() {
	LCD_setScreen(rgb565(255, 255, 255));
	char str2[] = "holderVal";
	sprintf(str2, "ft");
	LCD_drawString(65, 40, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, "C");
	LCD_drawString(105, 10, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, "Time");
	LCD_drawString(15, 90, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, ":");
	LCD_drawString(30, 100, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, ":");
	LCD_drawString(60, 100, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, "min NST");
	LCD_drawString(110, 110, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, "mbar");
	LCD_drawString(60, 75, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str2, "max ft");
	LCD_drawString(115, 30, str2, rgb565(0, 0, 0), rgb565(255, 255, 255));
}

void displayDiveScreen() {
	char str[10]; 
	
	//depth
	sprintf(str, "%d", (int) depth);
	if (alertingAscentRate) {
		//LCD_drawBigString(70, 20, str, rgb565(255, 0, 0), rgb565(255, 255, 255), 3);
		LCD_drawBigIntLen(5, 10, str, rgb565(255, 0, 0), rgb565(255, 255, 255), 5, 2, 0);
	} else {
		LCD_drawBigIntLen(5, 10, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 5, 2, 0);
	}
	sprintf(str, "%d", temp);
	LCD_drawBigIntLen(70, 10, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 3, 2, 0);
	
	sprintf(str, "%d", noStopTime);
	if (noStopTime > 99) {
		sprintf(str, "%d", 99);
	}
	if (alertingNoStopTime) {
		LCD_drawBigIntLen(110, 80, str, rgb565(255, 0, 0), rgb565(255, 255, 255), 3, 2, 1);
	} else {
		LCD_drawBigIntLen(110, 80, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 3, 2, 1);
	}
	
	sprintf(str, "%d", pressure);
	LCD_drawBigString(60, 55, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 2);
	
	sprintf(str, "%d", maximumDepth);
	LCD_drawBigString(115, 10, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 2);
	
}


void redrawTime() {
	char str[10];
	//dive time
	sprintf(str, "%d", hours);
	LCD_drawBigIntLen(5, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 2, 2, 1);
	sprintf(str, "%d", minutes);
	LCD_drawBigIntLen(35, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 2, 2, 1);
	sprintf(str, "%d", seconds);
	LCD_drawBigIntLen(65, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255), 2, 2, 1);



	if (inSafetyStop && ADC > 64)
	{
		sprintf(str, "%d", safetyTimeLeft);
		LCD_drawBigIntLen(10, 60, str, rgb565(0, 0, 0), rgb565(255, 0, 0), 3, 3, 0);
	}
	
}

void resetValues() {
	seconds = 0;
	minutes = 0;
	hours = 0;
	depth = 0;
	pressure = 0;
	temp = 0;
	maximumDepth = 0;
	inSafetyStop = 0;
	requiresSafetyStop = 0;
	safetyTimeLeft = 0;
	previousDepthReading = 0;
	alertingAscentRate = 0;
	alertingNoStopTime = 0;
	noStopTime = 100;
	currentlyDiving = 0;
	// Resets dive log string
	divelog[0] = '\0';
}

void drawOutOfWaterScreen() {
	char str[10];
	LCD_setScreen(rgb565(255, 255, 255));
	//max depth
	sprintf(str, "%d", maximumDepth);
	LCD_drawBigIntLen(5, 10, str, rgb565(0, 0, 0),rgb565(255, 255, 255), 5, 2, 0);
	sprintf(str, "Max Depth (ft)");
	LCD_drawString(65, 40, str, rgb565(0, 0, 0), rgb565(255, 255, 255));

	//dive time
	redrawTime();

	sprintf(str, "Time");
	LCD_drawString(15, 90, str, rgb565(0, 0, 0), rgb565(255, 255, 255));

	sprintf(str, ":");
	LCD_drawString(30, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255));
	sprintf(str, ":");
	LCD_drawString(60, 100, str, rgb565(0, 0, 0), rgb565(255, 255, 255));


}

void Initialize() {
	cli();
	TWI_Master_Initialise();
	UART_init(BAUD_PRESCALER);
	// CSB pulled high on ms5803
	TWI_targetSlaveAddress = 0x76;
	lcd_init();
	
	// Buzzer on pd2
	DDRD |= (1<<DDD2); //set to input pin
	PORTD &= ~(1<<PORTD2); //set to low
	// LED on pd3
	DDRD |= (1 << DDD3);
	PORTD &= ~(1 << PORTD3);
	
	/* ADC BELOW
	* ==================================
	*/
	
	//Clear power reduction
	PRR &= ~(1 << PRADC);
	
	//ADMUX
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);
	
	// ADC Clock divider of 128 for 125kHz clock
	ADCSRA |= (1 << ADPS0);
	ADCSRA |= (1 << ADPS1);
	ADCSRA |= (1 << ADPS2);
	
	//Select channel 0
	ADMUX &= ~(1 << MUX0);
	ADMUX &= ~(1 << MUX1);
	ADMUX &= ~(1 << MUX2);
	ADMUX &= ~(1 << MUX3);
	
	// Set to auto trigger
	ADCSRA |= (1 << ADATE);
	
	// Set to free running
	ADCSRB &= ~(1 << ADTS0);
	ADCSRB &= ~(1 << ADTS1);
	ADCSRB &= ~(1 << ADTS2);
	
	// Disable digital input buffer on ADC pin
	DIDR0 |= (1 << ADC0D);
	
	// Enable ADC
	ADCSRA |= (1 << ADEN);

	// Start conversion
	ADCSRA |= (1 << ADSC);

	//===================================

	//timer 1 setup such that OCA is called as many times per second as timerInterruptsPerSecond
	//set prescaler to 256
	TCCR1B &= ~(1<<CS10);
	TCCR1B &= ~(1<<CS11);
	TCCR1B |= (1<<CS12);

	//sets prescaler to 256
	TIMSK1 |= (1<<OCIE1A);//enables output compare A
	OCR1A = 62500/timerInterruptsPerSecond; //should be 62500 for 1s

	sei();

	// SD Card
	DSTATUS init_result = disk_initialize();
	FRESULT mount_result = pf_mount(&fs);
	FRESULT open_res = pf_open("test.txt");
	pf_lseek(0);
	char init_buf[20];
	sprintf(init_buf, "Init: %d, mnt: %d, open_res: %d\n", init_result, mount_result, open_res);
	UART_putstring(init_buf);
	// =====================
	setCalibrationCoeffs();
	initDraw();
	redrawTime();
}

int main() {
	Initialize();

	// char buffer[50];
	UART_putstring("Starting\n");

	while (1) {

		
		// If in water
		if (ADC > 64) {
			if (run_timer_update == 1) {
				runTimerUpdate();
				displayDiveScreen();
				run_timer_update = 0;
				// sprintf(buffer, "%d\n", numCalls);
				// UART_putstring(buffer);
			}
			if (gotWet == 0) {
				gotWet = 1;
				resetValues();
				initDraw();
			}

			// LED on and buzzer on
			// PORTD |= (1 << PORTD3);
			// PORTD |= (1 << PORTD2);

			displayDiveScreen();
			// _delay_ms(200);
		} else {
			if (gotWet == 1) {
				gotWet = 0;
				FRESULT wrote_data = pf_write(divelog, strlen(divelog), &w);
				FRESULT finished = pf_write("==FINISHED==\r\n", strlen("==FINISHED==\n"), &w);
				FRESULT finalized = pf_write(0, 0, &w);
				char buf[20];
				sprintf(buf, "wrote_data: %d, finished: %d, finalized: %d\n",wrote_data, finished, finalized);
				UART_putstring(buf);
				drawOutOfWaterScreen();
			}
			
			// LED off and buzzer off
			// PORTD &= ~(1 << PORTD3);
			// PORTD &= ~(1 << PORTD2);

			// Sets screen to be red when out of water
			// _delay_ms(200);
		}
	}
}

// called every second
ISR(TIMER1_COMPA_vect) {
	run_timer_update = 1;
	OCR1A = (OCR1A + (62500/timerInterruptsPerSecond)) % 65536;
	if (inSafetyStop) {
		if (depth <= safetyStopLowDepth && depth >= safetyStopHighDepth) {
			safetyTimeLeft --;
		}
		if (safetyTimeLeft == 0) {
			inSafetyStop = 0;
		}
	}
	if (gotWet == 1) {
		seconds++;
		if (seconds == 60) {
			minutes++;
			seconds = 0;
			if (minutes == 60) {
				hours++;
				minutes = 0;
			}
		}
		redrawTime();
	}
}