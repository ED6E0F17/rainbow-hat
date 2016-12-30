#include <unistd.h>
#include "wiringPi.h"

// A-Red, B-Green, C-Blue
// GPIO numbers, not wiringPi !!
int ledPins[3] =	{6, 19, 26};
int buttonPins[3] =	{21, 20, 16};
int held[3] = 		{0, 0, 0};

int setup_buttons() {
	int i;
	if (wiringPiSetupGpio() < 0)
		return 0;
	for (i = 0; i < 3; i++) {
		pinMode( ledPins[i], OUTPUT );
		pinMode( buttonPins[i], INPUT );
		digitalWrite( ledPins[i], 0);
	}
	return 1;
}

#define ONETENTH 100000
int poll_buttons() {
	int i, b[3], result;
	for (i = 0; i < 3; i++)
		b[i] = digitalRead( buttonPins[i] );

	usleep( ONETENTH ); // debounce delay of 0.1 seconds

	result = 0;
	for (i = 0; i < 3; i++) {
		if ( b[i] ==  digitalRead( buttonPins[i] )) {
			held[i] = ! b[i];
			digitalWrite( ledPins[i], ! b[i]);
		}
		if ( held[i] )
			result |= (1 << i);
	}
	return result;
}
