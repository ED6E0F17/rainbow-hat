/*
 *  (c) 2016 pimoroni.com: no warranty given or implied
 */

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/types.h>
#include <signal.h>
#include <unistd.h>
#include "blinkt.h"

#define APA_SOF 0b11100000
#define DEFAULT_BRIGHTNESS 8
#define BLINKT_LEDS 8

#ifdef BTEST
 #define BLINKT
 #define TEST
#endif

#ifdef BLINKT
 #define MOSI 23
 #define SCLK 24
#else
 #define RAINBOW
 #define MOSI 10
 #define SCLK 11
 #define NSS   8
#endif

#define HIGH 1
#define LOW  0

#ifdef TEST
volatile int running = 0;
#endif

uint32_t leds[BLINKT_LEDS] = {};

#ifdef TEST
void sigint_handler(int unused){
	unused += running;
	running = 0;
	return;
}
#endif

void blinkt_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b){
	uint32_t result;
	if(led > BLINKT_LEDS)
		return;
	result = leds[led] & 0xff;
	result |= (uint32_t)r << 24;
	result |= (uint32_t)g << 16;
	result |= (uint16_t)b << 8;

	leds[led] = result;
}

void blinkt_brightness(uint8_t led, uint8_t brightness){
	if(led > BLINKT_LEDS)
		return;
	leds[led] = (leds[led] & 0xFFFFFF00) | (brightness & 0b11111);
}

void write_byte(uint8_t byte){
	int n;
	for(n = 0; n < 8; n++){
		digitalWrite(MOSI, (byte >> (7-n)) & 1);
		digitalWrite(SCLK, HIGH);
		digitalWrite(SCLK, LOW);
	}
}

void show_blinkt(){
	int x;
#ifdef RAINBOW
	digitalWrite(NSS, LOW);
#endif
	for(x = 0; x < 4; x++) // start with 32+ zeros
		write_byte(0);
	for(x = 0; x < BLINKT_LEDS; x++){
		write_byte(0xFF & (leds[x] | APA_SOF));
		write_byte(0xFF & (leds[x] >> 8 ));
		write_byte(0xFF & (leds[x] >> 16));
		write_byte(0xFF & (leds[x] >> 24));
	}
	for(x = 0; x < 5; x++)	// close and latch with 36+ zeros
		write_byte(0x00);
#ifdef RAINBOW
	digitalWrite(NSS, HIGH);
#endif
}

int setup(void) {
	int x;
	if (wiringPiSetupGpio() < 0)
		return 0;
#ifdef TEST
	printf("WiringPi Initialized.\n");
#endif
	for(x = 0; x < BLINKT_LEDS; x++)
		leds[x] = DEFAULT_BRIGHTNESS;
	return 1;
}

int start_blinkt() {
	if ( !setup() )
		return 0;
	pinMode(MOSI, OUTPUT);
	digitalWrite(MOSI, LOW);
	pinMode(SCLK, OUTPUT);
	digitalWrite(SCLK, LOW);
#ifdef RAINBOW
	pinMode(NSS, OUTPUT);
	digitalWrite(NSS, HIGH);
#endif
	return 1;
}


#ifdef TEST
int main() {
	int z, x;
	uint8_t v, w, y = 1;
	int col = 0;

	running = start_blinkt();
	if (!running){
		printf("Unable to start apa102\n");
		return -19;
	}
	printf("Running test cycle\n");

	signal(SIGINT, sigint_handler);

	for(x = 0; x < BLINKT_LEDS; x++)
		leds[x] = DEFAULT_BRIGHTNESS;

	while(running){
		w = (y>>1) + (y>>3); //  These LEDS have extra-bright Green
		v = (y>>1) + (y>>2); //  And less-bright Red.
		for(z = 0; z < BLINKT_LEDS; z++){
			switch(col >> 1){
				default:
				case 0: blinkt_color(z,y,0,0); break;
				case 1: blinkt_color(z,y,w,0); break;
				case 2: blinkt_color(z,0,w,0); break;
				case 3: blinkt_color(z,0,w,v); break;
				case 4: blinkt_color(z,0,0,v); break;
				case 5: blinkt_color(z,y,0,v); break;
				case 6: blinkt_color(z,y,w,v); break;


			}
		}
		usleep(8000);

		if (y == 0)
			col++;
		else
			show_blinkt();
                if (col > 13) {
			col = 0;
		}
		if (col & 1)
			y--;
		else
			y++;
	}

	for(x = 0; x < BLINKT_LEDS; x++)
		leds[x] = DEFAULT_BRIGHTNESS;
	show_blinkt();

	return 0;

}
#endif
