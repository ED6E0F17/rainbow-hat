
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>

int dig_T[4];
int dig_P[10];
// t_fine carries fine temperature as global value: read temperature at least once before pressure.
double t_fine = 0;

// Returns temperature in DegC, resolution is 0.1 DegC. Output value of "25.6" equals 25.6 DegC.
// t_fine carries fine temperature as global value: read temperature at least once before pressure.
double bmp280_calc_temperature( int adc_T ) {
	double var0, var1, var2;
	var0 = ( (double)adc_T / 16.0 - (double)dig_T[1] ) / 1024.0;
	var1 = var0 * (double)dig_T[2];
	var2 = var0 * var0 * (double)dig_T[3] / 64.0;
	t_fine = (var1 + var2);
	return t_fine / 5120.0;
}

// Returns pressure in Pa. Output value of "96386" equals 96386 Pa = 963.86 hPa
double bmp280_calc_pressure( int adc_P ) {
	double var0, var1, var2, p;
	var1 = t_fine * 0.5  - 64000.0;
	var2 = var1 * var1 * (double)dig_P[6] / 32768.0;
	var2 = var2 + var1 * (double)dig_P[5] * 2.0;
	var2 = var2 * 0.25 + (double)dig_P[4] * 65536.0;

	var0 = var1 / 524288.0;
	var1 = (double)dig_P[3] * var0 * var0 + (double)dig_P[2] * var0;
	var1 = ( 1.0 + var1 / 32768.0 ) * (double)dig_P[1];
	if ( var1 == 0.0 ) {
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)adc_P - var2 / 4096.0;
	p = p  * 6250.0 / var1;
	var1 = (double)dig_P[9] * p * p / 2147483648.0;
	var2 = (double)dig_P[8] * p / 32768.0;
	p = p + ( var1 + var2 + (double)dig_P[7] ) / 16.0;
	return p;
}

#define BMP280 0x77
#define REG_ID280 0xD0
#define REG_DIG_T 0x88
#define REG_TEMP  0xFA
#define REG_PRESS 0xF7
#define REG_CONFIG 0xF5
#define REG_CONTROL 0xF4

// wiringPi I2C file handle
int bmp280_fd = -1;

// Read I2C 16 bit little endian register - compensation data is little endian
uint16_t readShort(int reg) {
	int msb, lsb;

	lsb = 0xFF & wiringPiI2CReadReg8( bmp280_fd, reg );
	msb = 0xFF & wiringPiI2CReadReg8( bmp280_fd, reg + 1 );
	return (msb << 8) | lsb;
}

// Read I2C 20 bit big endian register - measured data is big endian
int readLong(int reg) {
	int msb, lsb, xls, result;
	msb = 0xFF & wiringPiI2CReadReg8( bmp280_fd, reg );
	lsb = 0xFF & wiringPiI2CReadReg8( bmp280_fd, reg + 1 );
	xls = 0xF0 & wiringPiI2CReadReg8( bmp280_fd, reg + 2 );

	result =  msb << 12;
	result |= lsb << 4;
	result |= xls >> 4;
	return result;
}

int setup_bmp280() {
	int i, r;
	uint16_t reg;

	bmp280_fd = wiringPiI2CSetup( BMP280 );
	if ( bmp280_fd < 0 ) {
		return 0;
	}
	r = wiringPiI2CReadReg8( bmp280_fd, REG_ID280 );
	if ( r != 0x58 ) {
		return 0;
	}

	// set 4 Hz  samplerate, continuous mode
	wiringPiI2CWriteReg8( bmp280_fd, REG_CONTROL, (3<<5)|(3<<2)|0 ); // sleep mode
	wiringPiI2CWriteReg8( bmp280_fd, REG_CONFIG , (3<<5)|(3<<2)|0 ); // spi off

	// read factory measured offsets
	for (i = 0; i < 12; i++) {
		reg = readShort( REG_DIG_T + i*2 );
		if ((i == 0) || (i == 3))
			r = (int)(uint32_t)reg; // cast up from U16
		else
			r = (int)(int16_t)reg; // cast up from S16
	//	printf ("reg: %d\n", r);
		if (i < 3)
			dig_T[i+1] = r;
		else
			dig_P[i-2] = r;
	}
	wiringPiI2CWriteReg8( bmp280_fd, REG_CONTROL, (3<<5)|(3<<2)|3 ); // continuous mode

	return 1;
}

double bmp280temperature() {
	int temp;
	temp = readLong(REG_TEMP);
	return bmp280_calc_temperature(temp);
}

double bmp280pressure() {
	int press;
	press = readLong(REG_PRESS);
	return bmp280_calc_pressure(press);
}
