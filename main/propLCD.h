/*
 * popLCD.h
 *
 *  Created on: 16 mar. 2020
 *      Author: Julian
 */

#ifndef MAIN_PROPLCD_H_
#define MAIN_PROPLCD_H_

#include "driver/gpio.h"
#include "delay.h"

static const char *TAG = "APP";


#define     LCD_RS              18
#define     LCD_E               5
#define		LCD_D4				21
#define		LCD_D5				22
#define		LCD_D6				19
#define		LCD_D7				23

#define     LCD_ROWS_BOTH       0
#define     LCD_ROWS_TOP        1
#define     LCD_ROWS_BOTTOM     2

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// VAriables
unsigned _displayfunction;
unsigned _displaycontrol;
unsigned _displaymode;
unsigned _row_ofsets;

void lcd_init(void);



void display(void);
void clear(void);
void home(void);

void command(unsigned);

void lcd_write(char);
void lcd_write_string(char*);

void send(unsigned, unsigned);
void pulseEnable(void);
void write4bits(unsigned);

void lcd_goto(unsigned char, unsigned char);





void lcd_init(void) {
	gpio_pad_select_gpio (LCD_RS);
	gpio_pad_select_gpio (LCD_E);
	gpio_pad_select_gpio (LCD_D4);
	gpio_pad_select_gpio (LCD_D5);
	gpio_pad_select_gpio (LCD_D6);
	gpio_pad_select_gpio (LCD_D7);

	gpio_set_direction(LCD_RS, GPIO_MODE_OUTPUT);
	gpio_set_direction(LCD_E, GPIO_MODE_OUTPUT);
	gpio_set_direction(LCD_D4, GPIO_MODE_OUTPUT);
	gpio_set_direction(LCD_D5, GPIO_MODE_OUTPUT);
	gpio_set_direction(LCD_D6, GPIO_MODE_OUTPUT);
	gpio_set_direction(LCD_D7, GPIO_MODE_OUTPUT);
	/*
	_row_ofsets[0] = 0x00;
	_row_ofsets[1] = 0x40;
	*/
	delay_us(50000);

	// Now we pull both RS and R/W low to begin commands
	gpio_set_level(LCD_RS, 0);
	gpio_set_level(LCD_E, 0);

	// we start in 8bit mode, try to set 4 bit mode
	write4bits(0x03);
	delay_us(4500); // wait min 4.1ms

	// second try
	write4bits(0x03);
	delay_us(4500); // wait min 4.1ms

	// third go!
	write4bits(0x03);
	delay_us(150);

	// finally, set to 4-bit interface
	write4bits(0x02);

	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE
			| LCD_5x10DOTS;
	command(LCD_FUNCTIONSET | _displayfunction);

	clear();

	delay_ms(100);

	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();

	delay_ms(100);

	// Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	delay_ms(100);
}


/* High level commands */

void display(void) {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void clear(void) {
	command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
	delay_us(3000);					// this command takes a long time!
}

void home(void) {
	command(LCD_RETURNHOME);			// set cursor position to zero
	delay_us(2000);  // this command takes a long time!
}


void lcd_goto (unsigned char row, unsigned char col)
{
    unsigned char cmd;

    cmd = 0x80 + ((row - 1u) * 0x40) + (col - 1u);
    command(cmd);
}



/*********** mid level commands, for sending data/cmds */

void command(unsigned value) {
	send(value, 0);
}

void lcd_write_string(char* data)
{
    int i = 0;
    while (data[i] != '\0')
    {
    	lcd_write(data[i]);
        i++;
        delay_us(50);
    }
}

void lcd_write(char c) {
	send(c, 1);
}


/************ low level data pushing commands **********/

// write either command or data, with 4-bit
void send(unsigned value, unsigned mode) {
	//printf("\n %c %d \n", value, value);
	gpio_set_level(LCD_RS, mode);
	write4bits(value >> 4);
	write4bits(value);
}

void pulseEnable(void) {
	gpio_set_level(LCD_E, 0);
	delay_us(10);
	gpio_set_level(LCD_E, 1);
	delay_us(10);					// enable pulse must be >450ns
	gpio_set_level(LCD_E, 0);
	delay_us(100);  				// commands need > 37us to settle
}

void write4bits(unsigned value) {
	gpio_set_level(LCD_D4, value & 0x01);
	gpio_set_level(LCD_D5, value >> 1 & 0x01);
	gpio_set_level(LCD_D6, value >> 2 & 0x01);
	gpio_set_level(LCD_D7, value >> 3 & 0x01);

	pulseEnable();
}


#endif /* MAIN_PROPLCD_H_ */
