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

#include <stdint.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>

#define VC0706_RESET  0x26
#define VC0706_GEN_VERSION 0x11
#define VC0706_SET_PORT 0x24
#define VC0706_READ_FBUF 0x32
#define VC0706_GET_FBUF_LEN 0x34
#define VC0706_FBUF_CTRL 0x36
#define VC0706_DOWNSIZE_CTRL 0x54
#define VC0706_DOWNSIZE_STATUS 0x55
#define VC0706_READ_DATA 0x30
#define VC0706_WRITE_DATA 0x31
#define VC0706_COMM_MOTION_CTRL 0x37
#define VC0706_COMM_MOTION_STATUS 0x38
#define VC0706_COMM_MOTION_DETECTED 0x39
#define VC0706_MOTION_CTRL 0x42
#define VC0706_MOTION_STATUS 0x43
#define VC0706_TVOUT_CTRL 0x44
#define VC0706_OSD_ADD_CHAR 0x45

#define VC0706_STOPCURRENTFRAME 0x0
#define VC0706_STOPNEXTFRAME 0x1
#define VC0706_RESUMEFRAME 0x3
#define VC0706_STEPFRAME 0x2

#define VC0706_640x480 0x00
#define VC0706_320x240 0x11
#define VC0706_160x120 0x22

#define VC0706_MOTIONCONTROL 0x0
#define VC0706_UARTMOTION 0x01
#define VC0706_ACTIVATEMOTION 0x01

#define VC0706_SET_ZOOM 0x52
#define VC0706_GET_ZOOM 0x53

/* Camera buffer 256 bytes */
#define CAMREADSIZE 240
#define CAMERASIZE 256
#define CAMERADELAY 10


int begin( void );
void reset( void );
int TVon( int state );
int takePicture( void );
uint8_t *readPicture( uint8_t n );
int resumeVideo( void );
uint32_t frameLength( void );
void getVersion( void );
uint8_t available();
uint8_t getDownsize( void );
int setDownsize( uint8_t );
uint8_t getImageSize();
int setImageSize( uint8_t );
int frameBuffCtrl( uint8_t command );
uint8_t getCompression();
int setCompression( uint8_t c );

uint8_t serialNum = 0;
uint8_t camerabuff[CAMERASIZE];
uint8_t bufferLen;
uint16_t frameptr;
int	uartfile;

void flush(void);
int runCommand( uint8_t cmd, uint8_t args[], uint8_t argn, uint8_t resp );
void sendCommand( uint8_t cmd, uint8_t args[], uint8_t argn );
uint8_t readResponse( uint8_t numbytes, uint8_t timeout );
int verifyResponse( uint8_t command );