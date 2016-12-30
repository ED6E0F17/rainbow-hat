/*
 *  (c) 2016 pimoroni.com: no warranty given or implied
 */

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include <signal.h>
#include <unistd.h>
#include "buttons.h"
#include "bmp280.h"
#include "alphanum.h"

#include "blinkt.c"

// scrolling delay in tenths per char
#define PAUSE 3

// Compensate for CPU temp
#define TEMP_ADJUST (-10.0)

volatile int running = 0;
void sigint_handler(int unused){
	unused += running;
	running = 0;
	return;
}

int display_off = 0;
int buttonstate = 0;
void buttonpress( int b ) {
	if (b & (1 << 2))
		running = 0;
	if (b & (1 << 1))
		display_off = 0;
	if (b & (1 << 0))
		display_off = 1;
	buttonstate = b;
}

void pollAndPause(int tenths) {
	int i, b;
	for (i = 0; running & (i < tenths); i++) {
		b = poll_buttons(); // delays 0.1 seconds
		if ( b != buttonstate )
			buttonpress( b );
	}
}

int fd_HT16K33 = -1;
unsigned int digits[5] = {32,32,32,32,0};

void show_HT16K33(int n, unsigned int digit) {
	int msb, unused = 0;

	if (display_off > 1)
		return;
	if (display_off == 1) {
		unused += wiringPiI2CWriteReg8(fd_HT16K33, HT16K33_DISPLAYOFF, 0);
		display_off = 2;
		return;
	}
	if (display_off == 0) {
		unused += wiringPiI2CWriteReg8(fd_HT16K33, HT16K33_DISPLAYON, 0);
		display_off = -1;
	}

	if (digit > 0x7f) {
		digit &= 0x7f;
		msb = ((font14[digit] >> 8) & 0x3f ) | (1<<6); // add decimal point
	} else 	msb = (font14[digit] >> 8) & 0x3f;

	unused += wiringPiI2CWriteReg8(fd_HT16K33, n*2, font14[digit] & 0xff);
	unused += wiringPiI2CWriteReg8(fd_HT16K33, n*2 + 1, msb);
}

int setup_HT16K33( void ) {
	int i, unused = 0;

	fd_HT16K33 = wiringPiI2CSetup( HT16K33_I2C );
	if (fd_HT16K33 < 0)
		return 0;

	unused += wiringPiI2CWriteReg8(fd_HT16K33, HT16K33_SETUP, 0);
	unused += wiringPiI2CWriteReg8(fd_HT16K33, HT16K33_CMD_BRIGHTNESS | MIDBRIGHT, 0);
	unused += wiringPiI2CWriteReg8(fd_HT16K33, HT16K33_DISPLAYON, 0);
	for (i=0; i<4; i++)
		show_HT16K33(i, digits[i]);
	return 1;
    }

void scroll_HT16K33(int digit, int pause) {
	int i;
	if ((digit == 46) && (digits[3]<128)) {
			digits[3] += 128; // add real decimal point
			show_HT16K33(3, digits[3]);
	} else {
		// pause BEFORE scrolling
		pollAndPause( pause );

		if (digit == 46)
			digit = 32 + 128;
		digits[4] = digit;
		for (i=0; i<4; i++) {
			digits[i] = digits[i+1];
			show_HT16K33(i, digits[i]);
		}
	}
}

void scrollString( char* mystring ) {
	int i;
	for (i = 0; mystring[i]; i++)
		scroll_HT16K33( mystring[i], PAUSE );
}

int minmax(int min, int x, int max) {
	return (x > min) ? ( (x < max) ? x : max) : min;
}

void TP_led(double temperature, double pressure) {
	int t, p;
	uint8_t r, g, b;

	p = (int)( (1030 - pressure + 5) / 10 );
	p = minmax( 0, p, 6);
	t = (int)((temperature - 20) * 10);
	t = minmax (-100, t, +100);
	if (t < 0) {
		r = 0;
		b = -t;
		g = 100 + t;
	} else {
		r = t;
		g = 100 - t;
		b = 0;
	}
	blinkt_color(p, r, g, b);
	show_blinkt();
	blinkt_color(p, 0, 0, 0);
}

int main() {
	int x;
	double temperature, pressure;
	char teststring[]="Pimoroni Rainbow Hat 2016 Weather Demo.     ";

	printf ("%s\n", teststring);
	printf ("\tButton A hides Display.\n");
	printf ("\tButton B shows Display.\n");
	printf ("\tPress Button C to exit.\n");

	if (!setup_buttons()) {
		printf("Unable to open wiringPi.\n");
		return -19;
	}

	if (!setup_bmp280()) {
		printf("Unable to start BMP280 driver.\n");
		return -19;
	}

	if (!setup_HT16K33()) {
		printf("Unable to start HT1633 LED driver.\n");
		return -19;
	}
	running = start_blinkt();
	if (!running){
		printf("Unable to start apa102.\n");
		return -19;
	}

	signal(SIGINT, sigint_handler);

	for(x = 0; x < BLINKT_LEDS; x++)
		leds[x] = DEFAULT_BRIGHTNESS;

	temperature = bmp280temperature() + TEMP_ADJUST;
	pressure = bmp280pressure() / 100;
	TP_led( (int)temperature, (int)pressure);

	printf("Temp:%1.1f\n", temperature );
	printf("Pressure:%1.2f hPa\n", pressure );

	scrollString( teststring );

	while (running) {
		TP_led( temperature, pressure);

		temperature = bmp280temperature() + TEMP_ADJUST;
		snprintf(teststring, 30, "  TEMP  %1.1fC", temperature);
		scrollString(teststring);
		pollAndPause( 20 );

		pressure = bmp280pressure() / 100;
		snprintf(teststring, 30, "   PRESSURE  %1.0f", pressure);
		scrollString( teststring );
		pollAndPause( 20 );

		snprintf(teststring, 30, " hPa    %1.1fmmHg   ", pressure * 0.075);
		scrollString( teststring );
	}

	for(x = 0; x < BLINKT_LEDS; x++)
		leds[x] = DEFAULT_BRIGHTNESS;
	show_blinkt();
	scrollString("    ");
	return !setup_buttons();
}
