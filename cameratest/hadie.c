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

//gcc hadie.c -l wiringPi -l pthread -o hadie

#include "vc0706.h"
#include "wiringSerial.h"

int begin() {
	uartfile = serialOpen("/dev/ttyAMA0", 38400);
	if (!uartfile)
	       return 0;
	reset();
	return 1;
}

void reset() {
	uint8_t args = 0;
	sendCommand( VC0706_RESET, &args, 1 );
	usleep(2000*1000);
	flush();
}

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

/*

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
	fprintf(stderr, "%s\n", &camerabuff[5]);
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
		fprintf(stderr, "Get length failed.\n");
		return 0;
	}

	return ((1 & camerabuff[6]) << 16) + (camerabuff[7] << 8) + (camerabuff[8]);
}


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
	// fprintf(stderr, "CMD:%02x, response len %d\n", cmd, len);
	if ( len  != resplen ) {
		fprintf(stderr, "CMD:%02x, bad length %d\n", cmd, len);
		return 0;
	}
	if ( !verifyResponse( cmd ) ) {
		fprintf(stderr, "CMD:%02x,bad response\n", cmd);
		return 0;
	}
	return 1;
}

#define SERIALOUT(X) serialPutchar(uartfile,X)
void sendCommand( uint8_t cmd, uint8_t args[], uint8_t argn ) {
	int i;

	SERIALOUT( 0x56 );
	SERIALOUT( serialNum );
	SERIALOUT( cmd );
	for ( i = 0; i < argn; i++ )
		SERIALOUT( args[i] );
}

uint8_t readResponse( uint8_t numbytes, uint8_t timeout1ms ) {
	uint8_t counter = 0;
	size_t in;
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
		if ( in < 0 )
			return bufferLen;
		camerabuff[counter++] = (uint8_t)in;
	}
	return bufferLen; 
}

int verifyResponse( uint8_t command ) {
	if ( ( camerabuff[0] != 0x76 ) ||
			( camerabuff[1] != serialNum ) ||
			( camerabuff[2] != command ) ||
			( camerabuff[3] != 0x0 ) ) {
		return 0;
	}
	return 1;
}

uint8_t* imagebuffer;
static int vc0706bytes;
int camera_reset()
{
	if( !begin()) {
		fprintf(stderr, "Camera not found\n");
		return 0;
	}

	getVersion(); // "VC0706 1.00"
	if( !TVout(0) )
		fprintf(stderr, "Video out not disabled\n");

	return 1;
}

int camera_photo(void) {
	fprintf(stderr, "\nTaking Picture.\n");
	takePicture();
	vc0706bytes = frameLength();
	fprintf(stderr, "%d bytes to read\n", vc0706bytes);

	// if( not lowpower() ):
	fprintf(stderr, "Framebuffer not disabled\n");
	return vc0706bytes;
}

uint8_t camera_read(uint8_t size) {
	if (!vc0706bytes)
		camera_photo();
	if (vc0706bytes < size)
		size = vc0706bytes;
	vc0706bytes -= size;
	imagebuffer = readPicture( size );
	if (!vc0706bytes) {
		resumeVideo(); // reload framebuffer at 30 fps
		usleep(200*1000);  // TODO: not needed if Tx has a delay
	}
	
	return size;
}

void camera_close(void) {
	serialClose(uartfile);
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
#include <stdio.h>
#include <string.h>
#include "ssdv.c"

/* Jpeg Image Pointer */
const uint8_t *img;



uint8_t fill_image_packet(uint8_t *pkt)
{
	static uint8_t setup = 0;
	static uint8_t img_id = 0;
	static ssdv_t ssdv;
	static size_t errcode;

	if(!setup)
	{
		setup = 1;
		ssdv_enc_init(&ssdv, SSDV_TYPE_NOFEC, CALLSIGN, img_id++, 1 + SSDV_QUALITY_NORMAL);
		ssdv_enc_set_buffer(&ssdv, pkt);
	}
	
	while((errcode = ssdv_enc_get_packet(&ssdv)) == SSDV_FEED_ME)
	{
		size_t len;
		if (!vc0706bytes)
			camera_photo();
		len = camera_read(CAMREADSIZE);
		if (len == 0)
			break; // image incomplete ?
		ssdv_enc_feed(&ssdv, camerabuff, len);
	}
	
	if(errcode != SSDV_OK)
	{
		/* Something went wrong! */
		fprintf(stderr, "\nSSDV returned errorcode %d\n",errcode);
		setup = 0;
		return 0;
	}
	
	if(ssdv.state == S_EOI)
		return 2; /* The end of the image has been reached */
	
	return 1; /* Got the packet */
}

int main(void) {
	char main_buffer[256];
	int i = 0;
	int errorcount = 0;

	if (!camera_reset())
		return 0; // oops
	while(i++ < 500) {
		if(!fill_image_packet(main_buffer)) {
			errorcount++;
			fprintf(stderr, "SSDV error %d", errorcount);
			// TODO: reset camera logic
		} else
			fwrite(main_buffer, 1, 256, stdout);
	}
	camera_close();
}
