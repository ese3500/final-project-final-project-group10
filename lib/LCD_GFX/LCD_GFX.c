/*
 * LCD_GFX.c
 *
 * Created: 9/20/2021 6:54:25 PM
 *  Author: You
 */ 

#include "LCD_GFX.h"
#include "ST7735.h"
#include <string.h>
#include <math.h>

/******************************************************************************
* Local Functions
******************************************************************************/



/******************************************************************************
* Global Functions
******************************************************************************/

/**************************************************************************//**
* @fn			uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
* @brief		Convert RGB888 value to RGB565 16-bit color data
* @note
*****************************************************************************/
uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((((31*(red+4))/255)<<11) | (((63*(green+2))/255)<<5) | ((31*(blue+4))/255));
}

/**************************************************************************//**
* @fn			void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color)
* @brief		Draw a single pixel of 16-bit rgb565 color to the x & y coordinate
* @note
*****************************************************************************/
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddr(x,y,x,y);
	SPI_ControllerTx_16bit(color);
}

/**************************************************************************//**
* @fn			void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor)
* @brief		Draw a character starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor){
	uint16_t row = character - 0x20;		//Determine row of ASCII table starting at space
	int i, j;
	if ((LCD_WIDTH-x>7)&&(LCD_HEIGHT-y>7)){
		for(i=0;i<5;i++){
			uint8_t pixels = ASCII[row][i]; //Go through the list of pixels
			for(j=0;j<8;j++){
				if (((pixels>>j)&1)==1){
					LCD_drawPixel(x+i,y+j,fColor);
				}
				else {
					LCD_drawPixel(x+i,y+j,bColor);
				}
			}
		}
	}
}


/******************************************************************************
* LAB 4 TO DO. COMPLETE THE FUNCTIONS BELOW.
* You are free to create and add any additional files, libraries, and/or
*  helper function. All code must be authentically yours.
******************************************************************************/

/**************************************************************************//**
* @fn			void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
* @brief		Draw a colored circle of set radius at coordinates
* @note
*****************************************************************************/
void LCD_drawCircle(uint8_t x0, uint8_t y0, uint8_t radius,uint16_t color)
{
	double maxLevels = 10;
	LCD_drawBlock(x0 - radius, y0 - (radius / 5), radius + x0, y0 + (radius / 5), color);
	//LCD_drawBlock(x0 - radius, y0 + (radius / maxLevels) - (radius / 5), radius + x0, y0 + (radius / maxLevels) + (radius / 5), color);
	for (int i = 1; i < maxLevels; i++) {
		int h = radius * i / maxLevels;
		int w = sqrt(radius * radius - (h * h));
		LCD_drawBlock(x0 - w, y0 + h - (radius / 5), w + x0, y0 + h + (radius / 5), color);
		LCD_drawBlock(x0 - w, y0 - h - (radius / 5), w + x0, y0 - h + (radius / 5), color);
	}
	
	//LCD_drawBlock(x0-radius, y0 - radius, x0 + radius, y0+radius, color);	
}


/**************************************************************************//**
* @fn			void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
* @brief		Draw a line from and to a point with a color
* @note
*****************************************************************************/
void LCD_drawLine(short x0,short y0,short x1,short y1,uint16_t c)
{//100, 50, 10, 10
	int y = 0;
	int dx = x1 - x0;//-90
	int dy = y1 - y0;//-40
	int w = 5;
	if (dx > 0) {
		for (int x = 0; x < dx; x+=w) {
			y = x * dy / dx;
			LCD_setAddr(x+x0,y+y0,x+x0+w,y+y0+w);
			SPI_ControllerTx_16bit(c);
			//LCD_drawPixel(x + x0, y + y0, c);
		}
	} else if (dx < 0) {
		for (int x = 0; x > dx; x-=w) {
			y = x * dy / dx;
			LCD_setAddr(x+x0,y+y0,x+x0+w,y+y0+w);
			SPI_ControllerTx_16bit(c);
			//LCD_drawPixel(x + x0, y + y0, c);
		}
	} else if (dy > 0) {//dx = 0, increasing y
		for (int i = 0; i < dy; i+=w) {
			LCD_setAddr(x0,i+y0,x0+w,i+y0+w);
			SPI_ControllerTx_16bit(c);
			//LCD_drawPixel(x0, i + y0, c);
		}
	} else if (dy < 0) {//dx = 0, decreasing y
		for (int i = 0; i > dy; i-=w) {
			LCD_setAddr(x0,i+y0,x0+w,i+y0+w);
			SPI_ControllerTx_16bit(c);
			//LCD_drawPixel(x0, i + y0, c);
		}
	} else {//one point
		LCD_drawPixel(x0, y0, c);
	}
}



/**************************************************************************//**
* @fn			void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
* @brief		Draw a colored block at coordinates
* @note
*****************************************************************************/
void LCD_drawBlock(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,uint16_t color)
{
	LCD_setAddr(x0,y0,x1,y1);
	for (int x = x0; x < x1; x++) {
		for (int y = y0; y < y1; y++) {
			//LCD_drawPixel(x, y, color);
			SPI_ControllerTx_16bit(color);
		}
	}
	
	
}

/**************************************************************************//**
* @fn			void LCD_setScreen(uint16_t color)
* @brief		Draw the entire screen to a color
* @note
*****************************************************************************/
void LCD_setScreen(uint16_t color) 
{
	LCD_drawBlock(0, 0, 180, 128, color);
}

/**************************************************************************//**
* @fn			void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
* @brief		Draw a string starting at the point with foreground and background colors
* @note
*****************************************************************************/
void LCD_drawString(uint8_t x, uint8_t y, char* str, uint16_t fg, uint16_t bg)
{
	int char_width = 6;
	for (int i = 0; i < strlen(str); i++) {
		LCD_drawChar(x + char_width*i, y, str[i], fg, bg);
	}
}

void LCD_drawBigString(uint8_t x, uint8_t y, char* str, uint16_t fColor, uint16_t bColor, uint8_t size) {
	//only give strings of number characters
	int char_width = 6 * size;
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			LCD_drawBigChar(x + char_width * i, y, str[i], fColor, bColor, size);
		} else {
			//LCD_drawChar(x + char_width * i, y, str[i], fColor, bColor);
		}
	}
}

void LCD_drawBigChar(uint8_t x, uint8_t y, uint16_t character, uint16_t fColor, uint16_t bColor, uint8_t size) {
	//only draws numbers
	character -= 48;
	uint16_t c = character;
	if (c != 1 && c != 4) {
		LCD_drawBlock(x + size, y, x + size * 4, y + size + 1, fColor);
	} else {
		LCD_drawBlock(x + size, y, x + size * 4, y + size + 1, bColor);
	}
	if (c < 1 || (c > 3 && c != 7)) {
		LCD_drawBlock(x, y + size, x + size, y + size * 4 + 1, fColor);
	} else {
		LCD_drawBlock(x, y + size, x + size, y + size * 4 + 1, bColor);
	}
	if (c != 5 && c != 6) {
		LCD_drawBlock(x + size * 4, y + size, x + size * 5, y + size * 4 + 1, fColor);
	} else {
		LCD_drawBlock(x + size * 4, y + size, x + size * 5, y + size * 4 + 1, bColor);
	}
	if (c != 7 && c > 1) {
		LCD_drawBlock(x + size, y + size * 4, x + size * 4, y + size * 5 + 1, fColor);
	} else {
		LCD_drawBlock(x + size, y + size * 4, x + size * 4, y + size * 5 + 1, bColor);
	}
	if (c == 0 || c == 2 || c == 6 || c == 8) {
		LCD_drawBlock(x, y + size * 5, x + size, y + size * 8 + 1, fColor);
	} else {
		LCD_drawBlock(x, y + size * 5, x + size, y + size * 8 + 1, bColor);
	}
	if (c != 2) {
		LCD_drawBlock(x + size * 4, y + size * 5, x + size * 5, y + size * 8 + 1, fColor);
	} else {
		LCD_drawBlock(x + size * 4, y + size * 5, x + size * 5, y + size * 8 + 1, bColor);
	}
	if (c != 1 && c != 4 && c != 7) {
		LCD_drawBlock(x + size, y + size * 8, x + size * 4, y + size * 9 + 1, fColor);
	} else {
		LCD_drawBlock(x + size, y + size * 8, x + size * 4, y + size * 9 + 1, bColor);
	}
}
	
void LCD_drawBigIntLen(uint8_t x, uint8_t y, char* str, uint16_t fColor, uint16_t bColor, uint8_t size, uint8_t numDigits, uint8_t drawZero) {
	//only give strings of number characters
	int char_width = 6 * size;
	int start = 0;
	if (strlen(str) < numDigits) {
		for (int i = 0; i < numDigits - strlen(str); i++) {
			if (drawZero == 1) {
				LCD_drawBigChar(x + char_width * i, y, '0', fColor, bColor, size);
			}
		}
		start = numDigits - strlen(str);
	}
	for (int i = start; i < numDigits; i++) {
		if (str[i-start] >= '0' && str[i-start] <= '9') {
			LCD_drawBigChar(x + char_width * (i), y, str[i-start], fColor, bColor, size);
		} else {
			LCD_drawChar(x + char_width * (i), y, str[i-start], fColor, bColor);
		}
	}
}
