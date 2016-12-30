/*
 * test2.c:
 *      Simple test program to test the wiringPi functions
 *      PWM buzzer test
 */

#include <wiringPi.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define BASETONE 440
// BCM Pin Number 13 not WiringPi 23.
#define PWM_PIN 13
#define ONEK 1000
#define DIVISOR 32
#define RANGE (int)(19.2 * ONEK * ONEK / DIVISOR / BASETONE)
// frequency = ( 19.2 MHz /divisor /range)
// range = (19.2 M / divisor / base tone)

int running = 1;
void sigint_handler( int unused ) {
	unused += running;
	running = 0;
	return;
}

int main( void ) {
	// SETUP
	int i, range[14];
	float tempre = 1 / 1.05946309436;
	float tempre2 = tempre * tempre;
	float tempre3 = tempre2 * tempre * tempre2;

	range[0] = (int)(RANGE); // A
	range[1] = (int)(RANGE * tempre2); // B
	range[2] = (int)(RANGE * tempre2 * tempre); // C
	range[3] = (int)(RANGE * tempre3); // D
	range[4] = (int)(RANGE * tempre3 * tempre2); // E
	range[5] = (int)(RANGE * tempre3 * tempre2 * tempre2); // F#
	range[6] = (int)(RANGE * tempre3 * tempre3); // G

	printf( "Raspberry Pi wiringPi PWM test program\n" ) ;
	printf( "Divisor:%d\n", DIVISOR) ;
	
	for (i = 0; i < 7; i++) {
		range[i + 7] = 2 * range[i]; // octave DOWN
		printf( "Range %d:%d\n", i, range[i]) ;
	}

	if ( wiringPiSetupGpio() == -1 ) {
		exit( 1 ) ;
	}



	pinMode( PWM_PIN, OUTPUT );
/*
	for (i=0; i<500; i++) {
		digitalWrite( PWM_PIN, 1);
		usleep(ONEK);
		digitalWrite( PWM_PIN, 0);
		usleep(ONEK);
	}
*/
	pinMode( PWM_PIN, PWM_OUTPUT );
	pwmSetMode( PWM_MODE_MS );
	pwmSetRange( RANGE );
	pwmWrite( PWM_PIN, RANGE / 2 ) ; // 50% mark/space
	pwmSetClock( DIVISOR );

	// MAIN 
	int fullNote = 1600;
	int halfNote = fullNote / 2;
	int quarterNote = halfNote / 2;
	int eighthNote = quarterNote / 2;

	//The Simpson's main theme plays: C-E-F#-A-G-E-C-G-F#-F#-F#-G
	//The tune rises for the first 4 notes, then from the G goes down to the G an octave lower. Three fast F#'s and back to the G.
	//{HALF, NOTE_C4, QUARTER, NOTE_E4, NOTE_FS4, EIGHTH, NOTE_A4, HALF, NOTE_G4, QUARTER, NOTE_E4, NOTE_C4,
	//   NOTE_G3, EIGHTH, NOTE_FS3, NOTE_FS3, NOTE_FS3, QUARTER, NOTE_G3};
	char note, *simpsons = "hCqEFoAhGqECgof.f.fqg.\0";
	int tone, j = 0;
	int duration = halfNote;

	printf( "Playing single tune...\n" ) ;

	signal(SIGINT, sigint_handler);
	while ( running && simpsons[j] ) {
		note = simpsons[j++];
		switch ( note ) {
		case 'h':
			duration = halfNote;
			break;
		case 'q':
			duration = quarterNote;
			break;
		case 'o':
			duration = eighthNote;
			break;
		case 's': // silence is broken
			pinMode( PWM_PIN, INPUT );
			usleep( duration * ONEK );
			pinMode( PWM_PIN, PWM_OUTPUT );
			break;
		case  '.': // pause
			pwmSetRange( 16 * range[0] );
			pwmWrite( PWM_PIN, 16 * range[0] );
			usleep( 10 * ONEK );
			break;
		default:
			tone = note - 'A';
			if (tone > 6)
				tone = note - 'a' + 7;
			if ((tone > 13) || (tone < 0))
				break;
			pwmSetRange( range[tone] );
			pwmWrite( PWM_PIN, range[tone] >> 1 );
			usleep( duration * ONEK );
		}
	}
	printf( ".... done.\n" ) ;


	pinMode( PWM_PIN, INPUT );
	return 0 ;
}
