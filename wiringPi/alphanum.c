#include "alphanum.h"
#include <unistd.h>
#include <wiringPiI2C.h>

int fd_HT16K33 = -1;
unsigned int digits[5] = {32,32,32,32,0};

void show_HT16K33(int n, unsigned int digit) {
	int msb, unused = 0;

	if (digit>0x7f) {
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
		usleep(pause); // no pause for first decimal point
		if (digit == 46)
			digit = 32 + 128;
		digits[4] = digit;
		for (i=0; i<4; i++) {
			digits[i] = digits[i+1];
			show_HT16K33(i, digits[i]);
		}
	}
}

#ifdef TEST
#define PAUSE 200000
int main(void) {
	int i, j;
	char teststring[]="Pimoroni Rainbow Hat 2016 v1.0   ............ ";

	if (!setup_HT16K33())
		return -19;

	for(j=0; j<3; j++) {
	    for (i=0; i<40; i++)
		scroll_HT16K33(teststring[i], PAUSE);
	    for (i=32; i<128; i++)
		scroll_HT16K33(i, PAUSE);
	}
	for(j=0; j<4; j++)
		scroll_HT16K33(32, PAUSE);
	return 0;
}
#endif
