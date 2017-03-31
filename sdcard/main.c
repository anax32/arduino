
#include <util/delay.h>
#include "../utils/uart.h"

//Ports
int PIN_CS = PINB2;      // chip select
int PIN_MOSI = PINB3;    // master out slave in
int PIN_MISO = PINB4;    // master in slave out
int PIN_CLOCK = PINB5;   // clock

#define	MAX_RETRIES		127

#define delay(x) 	_delay_ms(x)

void fatal_error (const char *msg) {while (1) {printf (msg);}}

/********************** SPI SECTION BELOW **********************/
unsigned char spi_cmd (volatile unsigned char data)
{
	SPDR = data;					// write initiates transmission
	while (!(SPSR&(1<<SPIF)));		// wait until SPI status is true
	return (SPDR);					// read the data register
}

char spi_initialize (void)
{
	int	tmp;

	tmp = 0;						// spi control register
	tmp |= (0<<SPIE);				// use spi interrupts
	tmp |= (1<<SPE);				// spi enabled (SPI)
	tmp |= (0<<DORD);				// byte order (MSB)
	tmp |= (1<<MSTR);				// master
	tmp |= (0<<CPOL);				// SCK low when idle
	tmp |= (0<<CPHA);				// sample leading edge
	tmp |= (0<<SPR1)|(0<<SPR0);		// clock speed fosc/4
	SPCR = tmp;						// set the control register

	// dummy reads to clear previous results
	tmp = SPSR;						// status register
	tmp = SPDR;						// data register

	return (0x00);
}

/********************** SD CARD SECTION BELOW **********************/

// SD Card variables
#define	BLOCK_SZ		512			// block size (default 512 bytes)
char	vBlock[BLOCK_SZ];			// write buffer
char	vBuffer[16];				// read buffer

#define	GO_IDLE_STATE		0x00	// resets the SD card
#define SD_IDENTIFY			0x08	// identify the sd card
#define SEND_CSD			0x09	// sends card-specific data
#define SEND_CID			0x0A	// sends card identification
#define READ_SINGLE_BLOCK	0x11	// reads a block at byte address
#define WRITE_BLOCK			0x18	// writes a block at byte address
#define SEND_OP_COND		0x29	// starts card initialization
#define APP_CMD				0x37	// prefix for application command
#define READ_OCR			0x3A

void sdc_pin_assert (void)
{
	PORTB &= ~(1<<PIN_CS);
}

void sdc_pin_deassert (void)
{
	PORTB |= (1<<PIN_CS);
}

unsigned char sdc_crc (unsigned char *chr, unsigned char cnt)
{
	unsigned char	i, a;
	unsigned char	v;
	unsigned char	crc;

	crc = 0;

	for (a=0; a<cnt; a++)
	{
		v = chr[a];

		for (i=0; i<8; i++)
		{
			crc <<= 1;

			if ((v&0x80)^(crc&0x80))
			{
				crc ^= 0x09;
			}

			v <<= 1;
		}
	}

	return (crc&0x7F);
}
// Send a SD command, num is the actual index, NOT OR'ed with 0x40.
// arg is all four bytes of the argument
unsigned char sdc_cmd (unsigned char commandIndex, unsigned long arg)
{
	unsigned char crc;
	unsigned char ret;


	printf ("sdc_cmd (0x%08x, 0x%08x)\n\0", commandIndex, arg);

//	sdc_pin_assert ();				// assert chip select for the card

	//printf ("sdc_cmd () dummy byte\n\0");
	spi_cmd (0xFF);					// send a dummy byte

	crc = 0x95; //(sdc_crc (&arg, 5) << 1) | 1;

	commandIndex |= 0x40;
	spi_cmd (commandIndex);			// send the command
	spi_cmd (arg >> 24);			// send the argument msb
	spi_cmd (arg >> 16);
	spi_cmd (arg >>  8);
	spi_cmd (arg >>  0);
	spi_cmd (crc);				// send the crc

	spi_cmd (0xFF);					// dummy byte

	while ((ret = spi_cmd (0xFF)) == 0xFF);		// ret val

	printf ("sdc_cmd () return 0x%08x\n\0", ret);

	delay (100);

	return ret;
}

// initialize SD card
// retuns 1 if successful
char sdc_initialize (void)
{
	int	i;

	SPCR |=  (1<<SPR1) | (1<<SPR0);	// set slow clock: 1/128 base frequency (125Khz in this case)
	SPSR &= ~(1<<SPI2X);			// no doubled clock frequency

	printf ("going\n\0");

// WAKE UP SD CARD
	sdc_pin_deassert ();			// deasserts card for warmup
//	PORTB |=  (1<<PIN_MOSI);		// set MOSI high

	// send 80 clock pulse warmups to the card
//	for (i=0; i<10; i++)			{spi_cmd (0xFF);}

	while (0x01 != sdc_cmd (GO_IDLE_STATE, 0));
	while (0x01 != sdc_cmd (SD_IDENTIFY, 0x000001AA));
	while (sdc_cmd (APP_CMD, 0) && sdc_cmd (SEND_OP_COND, 0x4000000));
	while (0x00 != sdc_cmd (READ_OCR, 0));

	sdc_pin_assert ();				// assert chip select for the card

	printf ("complete\n\0");

	// wait 16 clocks
	for (i=0; i<2; i++)				{spi_cmd (0xFF);}

// TEST CARD TYPE
//	unsigned char	type = 0;
//	type = spi_cmd (0x7F);		// CMD8

// SET IDLE MODE
//	fatal_error ("set 4\n\0");
	for (i=0; (!(sdc_cmd (GO_IDLE_STATE, 0) && 0x01)); i++)
	{
		// while SD card is not in idle state
		if (i == MAX_RETRIES)
		{
			printf ("Cancelling go idle attempt\n\0");
			return (0x01); // timed out!
		}

		sdc_cmd (0xFF, 0);
		sdc_cmd (0xFF, 0);

		// wait...
		printf ("Attempting to GO_IDLE\n\0");
		delay (10);
	}

	// at this stage, the card is in idle mode and ready for start up
	for (i=0; i<MAX_RETRIES; i++)
	{
		sdc_cmd (APP_CMD, 0);
		sdc_cmd (SEND_OP_COND, 0);

		if (sdc_cmd (0xFF, 0) == 0x01)
		{
			break;
		}

		delay (5);
	}

	if (i==MAX_RETRIES)
	{
		fatal_error ("bums\n\0");
		return 0x03;
	}

	// set fast clock, 1/4 CPU clock frequency (4Mhz in this case)
	// Clock frequency: f_OSC / 4
	SPCR &= ~((1<<SPR1) | (1<<SPR0));
	// Doubled clock frequency: f_OSC / 2
	SPSR |=  (1<<SPI2X);

	return (0x00);	// return NO_ERROR
}

// clear block content
// FIXME: replace with a marker indicating the next free byte
void sdc_clearVector (void)	{int i;	for (i=0; i<BLOCK_SZ; i++) {vBlock[i] = 0;}}

// get nbr of blocks on SD memory card from
long sdc_totalNbrBlocks (void)
{
	long	C_Size;
	long	C_Mult;

	/*sdc_readRegister (SEND_CSD);
*/
	// compute size
	C_Size = ((vBuffer[0x08] & 0xC0) >> 6) | ((vBuffer[0x07] & 0xFF) << 2) | ((vBuffer[0x06] & 0x03) << 10);
	C_Mult = ((vBuffer[0x08] & 0x80) >> 7) | ((vBuffer[0x08] & 0x03) << 2);

	return ((C_Size+1) << (C_Mult+2));
}

// read SD card register content and store it in vBuffer
void sdc_readRegister (char sentCommand)
{
	char	retries = 0;
	char	res = 0;
	int	i;

	// send the command
	res = sdc_cmd (sentCommand, 0);

	// try a bunch of no-ops to test business
	while (res != 0)
	{
		delay (1);

		if (retries++ >= 0xFF)
		{
			return;	// timed out!
		}

		res = spi_cmd (0xFF);	// retry
  	}

	// wait for data token
	while (spi_cmd (0xFF) != 0xFE)
		;;

	// read data
	// FIXME: use blocksize instead of 16?
	for (i=0; i<16; i++)
	{
		vBuffer[i] = spi_cmd(0xFF);
	}

	// read CRC (lost results in blue sky)
	spi_cmd (0xFF);	// LSB
	spi_cmd (0xFF);	// MSB
}

// write block on SD card
// addr is the address in bytes (multiples of block size)
void sdc_writeBlock (long blockIndex)
{
	unsigned char	retries = 0;
	int	i;

	printf ("sdc_writeBlock (%i)\n\0", blockIndex);

	while (sdc_cmd (WRITE_BLOCK, blockIndex * BLOCK_SZ) != 0x00)
	{
		delay (1);

		if (retries++ == 0xFF-1)
		{
			printf ("sdc_writeBlock () timed out\n\0");
			return;	// timed out!
		}
	}

	printf ("sdc_writeBlock () sending dummy byte\n\0");

	spi_cmd (0xFF); // dummy byte (at least one)

	// send data packet (includes data token, data block and CRC)
 	// data token
	spi_cmd (0xFE);

	// copy block data
	for (i=0; i<BLOCK_SZ; i++)
	{
		spi_cmd (vBlock[i]);
	}

	// write CRC (lost results in blue sky)
	spi_cmd (0xFF); // LSB
	spi_cmd (0xFF); // MSB

	// wait until write is finished
	while (spi_cmd (0xFF) != 0xFF)
	{
		delay (1); // kind of NOP
	}
}

// read block on SD card and copy data in block vector
// retuns 1 if successful
// FIXME: merge this with the readRegister function passing
// in block and blocksize parameters
void sdc_readBlock (long blockIndex)
{
	char	retries = 0;
	char	res = 0;
	int	i;

	res = sdc_cmd (READ_SINGLE_BLOCK,  (blockIndex * BLOCK_SZ));

	while (res != 0)
	{
		// wait...
		delay (1);

		if (retries++ >= 0xFF)
		{
			return; // timed out!
		}

		res = spi_cmd (0xFF); // retry
	}

	// read data packet (includes data token, data block and CRC)
 	// read data token
 	while (spi_cmd (0xFF) != 0xFE)
		;;

	// read data block
	for (i=0; i<BLOCK_SZ; i++)
	{
		vBlock[i] = spi_cmd (0xFF); // read data
	}

	// read CRC (lost results in blue sky)
	spi_cmd (0xFF); // LSB
	spi_cmd (0xFF); // MSB
}

/********************** MAIN ROUTINES SECTION  BELOW **********************/

int main (void)
{
	int	i, b;

	uart_init ();					// initialise the console output

	// Set ports
	DDRB &= ~(1<<PIN_MISO);			// Data in
	DDRB |=  (1<<PIN_CLOCK);		// Data out
	DDRB |=  (1<<PIN_CS);
	DDRB |=  (1<<PIN_MOSI);

	// Initialize SPI
	printf ("spi_initialize ()\n\0");
	if (spi_initialize () != 0x00)	{fatal_error ("spi_initialize () failed\n\0");}

	// init sd card
	printf ("sdc_initialize ()\n\0");
	if (sdc_initialize () != 0x00)	{fatal_error ("sdc_initialize () failed\n\0");}

	// get some data about the card
	printf ("querying card\n\0");
	i = sdc_cmd (8, 0x000001AA);
	if (i == 0x05)
	{
		printf ("illegal command\n\0");
		return;
	}
	if (i && 0x01)
	{
		printf ("returned %i\n\0", i);
	}

while (1)
{
	printf ("SD library ready\n\0");
	printf ("Writing blocks...\n\0");

	for (b=0; b<255; b++)
	{
		printf ("Writing block %i\n\0", b);

		for (i=0; i<BLOCK_SZ; i++)
		{
			vBlock[i] = b; // write incremental data
		}

		printf ("finished block %i\n\0", b);

		sdc_writeBlock (b);         // copy vector of data on SD card

		printf ("sdc_writeBlock () complete\n\0");
	}

	printf ("Reading blocks...");

	for (b=0; b<255; b++)
	{
		printf ("Reading block ");
		sdc_readBlock(b);          // copy SD card block of data in vector of data
		//sdc_printVectorContent();  // print vector of data content
	}

}

	return 1;
}
