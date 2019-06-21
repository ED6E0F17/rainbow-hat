/***************************************************
  This is a library for the Adafruit TTL JPEG Camera (VC0706 chipset)

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/397

  These displays use Serial to communicate, 2 pins are required to interface

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#include "ssdv.c"
#include "vc0706.h"
#include "wiringSerial.h"
#include "stdio.h"

uint8_t camerabuff[CAMERASIZE];

int serial_begin() {
	uartfile = serialOpen("/dev/ttyAMA0", 38400);
	if (uartfile)
		return 1;
	printf("Uart not found.\n");
	return 0;
}

/*
uint8_t getImageSize() {
	uint8_t args[] = {0x4, 0x4, 0x1, 0x00, 0x19};
	if ( !runCommand( VC0706_READ_DATA, args, sizeof( args ), 6 ) ) {
		return -1;
	}
	return camerabuff[5];
}

int setImageSize( uint8_t x ) {
	uint8_t args[] = {0x05, 0x04, 0x01, 0x00, 0x19, x};

	return runCommand( VC0706_WRITE_DATA, args, sizeof( args ), 5 );
}

uint8_t getDownsize( void ) {
	uint8_t args[] = {0x0};
	if ( !runCommand( VC0706_DOWNSIZE_STATUS, args, 1, 6 ) ) {
		return -1;
	}

	return camerabuff[5];
}

int setDownsize( uint8_t newsize ) {
	uint8_t args[] = {0x01, newsize};

	return runCommand( VC0706_DOWNSIZE_CTRL, args, 2, 5 );
}
*/

// check camera exists
void getVersion( void ) {
	uint8_t args[] = {0x00};

	if (!runCommand( VC0706_GEN_VERSION, args, 1, 16 )) {
		fprintf(stderr, "Bad camera version command.\n");
		return;
	}
	camerabuff[5+11] = 0;
	printf("%s\n", &camerabuff[5]);
}

/*
int setCompression( uint8_t c ) {
	uint8_t args[] = {0x5, 0x1, 0x1, 0x12, 0x04, c};
	return runCommand( VC0706_WRITE_DATA, args, sizeof( args ), 5 );
}

uint8_t getCompression( void ) {
	uint8_t args[] = {0x4, 0x1, 0x1, 0x12, 0x04};
	runCommand( VC0706_READ_DATA, args, sizeof( args ), 6 );
	printBuff();
	return camerabuff[5];
}
*/


int takePicture() {
	frameptr = 0;
	return frameBuffCtrl( VC0706_STOPCURRENTFRAME );
}

int resumeVideo() {
	return frameBuffCtrl( VC0706_RESUMEFRAME );
}

int TVout(uint8_t state) {
	uint8_t args[] = {0x1, state};
	return runCommand( VC0706_TVOUT_CTRL, args, sizeof( args ), 5 );
}

int frameBuffCtrl( uint8_t command ) {
	uint8_t args[] = {0x1, command};
	return runCommand( VC0706_FBUF_CTRL, args, sizeof( args ), 5 );
}

uint32_t frameLength( void ) {
	uint8_t args[] = {0x01, 0x00};
	if ( !runCommand( VC0706_GET_FBUF_LEN, args, sizeof( args ), 9 ) ) {
		printf("Get length failed.\n");
		return 0;
	}

	return ((1 & camerabuff[6]) << 16) + (camerabuff[7] << 8) + (camerabuff[8]);
}

/* Power ctrl may not be implemented
int powerCtrl( uint8_t activeLow ) {
	uint8_t args[] = {0x3, 0x0, 0x1, activeLow};
	return runCommand( VC0706_POWER_CTRL, args, sizeof( args ), 5 );
}

// "Manual" is a bit sketchy on what features get powered off
int powerMode( uint8_t fbuf_or_jpg ) {
	uint8_t args[] = {0x4, 0x1, fbuf_or_jpg, 0x0, 0x0};
	return runCommand( VC0706_POWER_CTRL, args, sizeof( args ), 5 );
}

int powerOff(void) {
	int err = 0;

	err += powerMode(1) <<0; // jpeg
	err += powerCtrl(1) <<1; //  off
	err += powerMode(0) <<2; // fbuf
	err += powerCtrl(1) <<3; //  off
	if( err )
		fprintf(stderr, "Powerdown command failed:%0x.\n",err);
	return err;
}
*/

uint8_t available( void ) {
	return bufferLen;
}

/* Warning - block size limited to 8 bit: 250 bytes */
uint8_t *readPicture( uint8_t n ) {
	uint8_t args[] = {0x0C, 0x0, 0x0A,
			  0, 0, frameptr >> 8, frameptr & 0xFF,
			  0, 0, 0, n,
			  CAMERADELAY >> 8, CAMERADELAY & 0xFF};

	if ( !runCommand( VC0706_READ_FBUF, args, sizeof( args ), 5 ) ) {
		return 0;
	}
	if ( readResponse( n + 5, 250 ) == 0 ) {
		return 0;
	}
	frameptr += n;

	return camerabuff;
}

/**************** low level commands */
void flush(void) {
	serialFlush( uartfile );
}

int runCommand( uint8_t cmd, uint8_t *args, uint8_t argn,  uint8_t resplen ) {
	int len;
	sendCommand( cmd, args, argn );
	len = readResponse( resplen, 200 );
	// printf("CMD:%02x, response len %d\n", cmd, len);
	if ( len  != resplen ) {
		printf("CMD:%02x, bad length %d\n", cmd, len);
		return 0;
	}
	if ( !verifyResponse( cmd ) ) {
		printf("CMD:%02x,bad response\n", cmd);
		return 0;
	}
	return 1;
}

#define SERIALOUT(X) serialPutchar(uartfile,X)
void sendCommand( uint8_t cmd, uint8_t args[], uint8_t argn ) {
	int i;

	SERIALOUT( 0x56 );
	SERIALOUT( SERIALNUM );
	SERIALOUT( cmd );
	for ( i = 0; i < argn; i++ )
		SERIALOUT( args[i] );
}

uint8_t readResponse( uint8_t numbytes, uint8_t timeout1ms ) {
	uint8_t counter = 0;
	bufferLen = 0;

	while ((bufferLen < numbytes) && (counter++ < timeout1ms )) {
		usleep(1000);
		bufferLen = serialDataAvail(uartfile);
	}
	if (bufferLen > numbytes) // for image data it could be
		bufferLen = numbytes;
	counter = 0;
	while (counter < bufferLen) { // get all
		int in = serialGetchar(uartfile); 
		if ( in < 0 ) // can not happen
			return counter;
		camerabuff[counter++] = (uint8_t)in;
	}
	return bufferLen; 
}

int verifyResponse( uint8_t command ) {
	if ( ( camerabuff[0] != 0x76 ) ||
			( camerabuff[1] != SERIALNUM ) ||
			( camerabuff[2] != command ) ||
			( camerabuff[3] != 0x0 ) ) {
		return 0;
	}
	return 1;
}

uint8_t* imagebuffer;
static int vc0706bytes = 0;

void camera_reset()
{
	uint8_t args = 0;

	vc0706bytes = 0;
	sendCommand( VC0706_RESET, &args, 1 );
	usleep(2000*1000);
}

void camera_vgaoff() {
	/* First thing called after reset */
	flush();
	getVersion(); // "VC0703 1.00"
	if( !TVout(0) )
		printf("Video out not disabled\n");
}

void camera_photo(void) {
	printf("\nTaking Picture.\n");
	takePicture();
	vc0706bytes = frameLength();
	printf("%d bytes to read\n", vc0706bytes);
}

void camera_EOI(void) {		// flush input and start camera
	vc0706bytes = 0;	// discard short reads
	resumeVideo();		// reload framebuffer at 30 fps
	usleep(200*1000);	// TODO: not needed if Tx has a delay
}

#define LOW_DISCARD 4
uint8_t camera_read(uint8_t size) {
	if (vc0706bytes < size)
		size = vc0706bytes;
	// size = (size + 3) & ~3;		// spec says read 4 bytes at a time
	vc0706bytes -= size;
	imagebuffer = readPicture( size ); // returns null for failure
	if (vc0706bytes < LOW_DISCARD)	// Leave small sizes unread
		camera_EOI();
	
	return size;
}

/* Nothing to do if power-save control is not implemented -	*
 *  taking next photo freezes framebuffer at end of each image	*
 *  and VGA out is disabled at every reset.			*/

void camera_close(void) {
	takePicture();	// stop framebuffer is all we can do
}

/* hadie - High Altitude Balloon flight software              */
/*============================================================*/
/* Copyright (C)2010 Philip Heron <phil@sanslogic.co.uk>      */
/*                                                            */
/* This program is distributed under the terms of the GNU     */
/* General Public License, version 2. You may use, modify,    */
/* and redistribute it under the terms of this license. A     */
/* copy should be included with this source.                  */


#define CALLSIGN "TEST"
//#include "config.h"

#define MAX_RESETS	10
#define NEEDS_RESET	0
#define WAS_RESET	1
#define NEEDS_SETUP	2
#define IS_SETUP	3
int fill_image_packet(uint8_t *pkt)
{
	static int setup = NEEDS_RESET;
	static uint8_t img_id = 0;
	static ssdv_t ssdv;
	static int reset_count = 0;
	size_t errcode;

	/* multiple resets suggest low battery, or low temperature failure */
	if (reset_count > MAX_RESETS)
		return -1;

	if (setup == NEEDS_RESET) {		// first run
		camera_reset();
		setup = WAS_RESET;
		return -1;		// reset needs delay
	}

	if (setup == WAS_RESET) {
		camera_vgaoff();	// lower power maybe
		setup = NEEDS_SETUP;
	}

	if (setup == NEEDS_SETUP) {	// start new image
		setup = IS_SETUP;
		ssdv_enc_init(&ssdv, SSDV_TYPE_NOFEC, CALLSIGN, ++img_id, 1 + SSDV_QUALITY_NORMAL);
		ssdv_enc_set_buffer(&ssdv, pkt);
	}
	
	while((errcode = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
	{
		size_t len;
		if (!vc0706bytes)
			camera_photo();
		len = camera_read(CAMREADSIZE);
		if (len == 0) {
			printf("Zero size read.\n");
			break;
		}
		ssdv_enc_feed(&ssdv, camerabuff, len);
	}

	if(errcode == SSDV_OK) {
		if (!vc0706bytes) {	// Force new image before EOI
			setup = NEEDS_SETUP;
			printf("Forcing next image start early.");
		}
		return img_id;	// Wrote a packet
	}

	if(errcode == SSDV_EOI) {	// Last packet
		printf("SSDV End of image.\n");
		camera_EOI();
		setup = NEEDS_SETUP;
		return img_id;
	}

	printf("Resetting: errcode %zu\n", errcode);
	/* Something went wrong */
	setup = WAS_RESET;
	reset_count++;
	if (reset_count > MAX_RESETS)
		camera_close();
	else
		camera_reset();
	return -1;
}

#if 1
//gcc hadie.c -l wiringPi -l pthread -o hadie

FILE *imgfile;
char filename[20];

int main(void) {
	uint8_t main_buffer[256];
	int status, i = 0;
	int errorcount = -1;

	if (!serial_begin())
		return 0; // oops
	while(i++ < 1000) {
		status = fill_image_packet(main_buffer);
		if (status < 0)
			errorcount++;
		else {
			sprintf(filename, "hadie%03x.ssdv", status);
			imgfile = fopen(filename,"a+");
			fwrite(main_buffer, 1, 256, imgfile);
			fclose(imgfile);
		}
	}
	printf("Errorcount:%d", errorcount);
	camera_close();
	usleep(1000*1000); // wiringpi uses threaded callbacks
	serialClose(uartfile);
}
#endif
