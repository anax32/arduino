#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <util/delay.h>

#include "../utils/uart.h"
#include "../utils/i2c.h"

typedef 	uint8_t	command_write_t[2];		// register / value pair
typedef 	uint8_t	command_read_t[2];		// register / number of bytes pair

// FIXME: devices have a fixed address, rather than enumerate the
// bus, just store the known device addresses and loop over those
typedef struct device_s
{
	uint8_t		*name;			// friendly name
	uint8_t		internal_id;	// internal identifier
	uint8_t 	addr;			// usual address

	uint8_t		id_register;	// reg which stores a known test value (WHO_AM_I)
	uint8_t		exp_id;			// expected response (known test value)

	command_write_t	init[4];		// init device command sequence
	command_read_t	read;			// read register comamnd sequence
} device_t;

// these are known ways to read from devices.
device_t devices[] = {
	{"NULL\0",			0x00,				// name, internal id
						0x00,				// usual i2c address
						0x00, 0x00,			// id register, expected response
						{{0x00, 0x00}},		// init commands (max of four)
						{0x00, 0x00}},		// read command (max of one)
	{"BMP180-PRES\0",	0x01,
						0x77,
						0xD0, 0x55,
						{{0x34, 1}},
						{0x00, 1}},
	{"BMP180-TEMP\0",	0x02,
						0x77,
						0xD0, 0x55,
						{{0x2E, 1}},
						{0x00, 1}},
	{"MAG3110-MAG\0",	0x03,
						0x0E,
						0x07, 0xC4,
//						{{0x11, 0x80}, {0x10, 0x40 | 0x10 | 0x08 | 0x01}},
						{{0x11, 0x80}, {0x10, 0x01}},
						{0x01, 6}},
	{"MAG3110-TEMP\0",	0x04,
						0x0E,
						0x07, 0xC4,
						{{0x00, 0x00}},	// initialisation is done by the first dev in list
						{0x0F, 1}}
//	{"TMP102\0",		0x05,
//						
};

// number of known devices in the library
#define DEVICE_COUNT	(sizeof(devices)/sizeof(device_t))

#define ERR_DEVICE_NO_ERROR				0
#define ERR_DEVICE_WRONG_ADDRESS		1
#define ERR_DEVICE_START_WRITE			2
#define ERR_DEVICE_WRITE_REGISTER		3
#define ERR_DEVICE_REP_START_READ		4
#define ERR_DEVICE_BAD_DEVICE_ID		5

uint8_t device_check_id (uint8_t addr, device_t *device)
{
	if (addr != device->addr)					{return ERR_DEVICE_WRONG_ADDRESS;}
	if (i2c_start_write (addr) == 0)			{i2c_stop (); return ERR_DEVICE_START_WRITE;}
	if (i2c_write (device->id_register) == 0)	{i2c_stop (); return ERR_DEVICE_WRITE_REGISTER;}
	if (i2c_rep_start_read (addr) == 0)			{i2c_stop (); return ERR_DEVICE_REP_START_READ;}
	if (i2c_read_nak () != device->exp_id)		{i2c_stop (); return ERR_DEVICE_BAD_DEVICE_ID;}

	i2c_stop ();
	return ERR_DEVICE_NO_ERROR;
}

uint8_t device_init (device_t *device)
{
	uint8_t	addr = device->addr;
	uint8_t	cnt = sizeof (device->init)/sizeof (command_write_t);
	uint8_t	id;
	command_write_t	*cmds = device->init;

	for (id=0; id<cnt; id++)
	{
		if (cmds[id][0] == 0x00)		{continue;}

		i2c_start_write (addr);
		i2c_write (cmds[id][0]);
		i2c_write (cmds[id][1]);
		i2c_stop ();
	}

	return ERR_DEVICE_NO_ERROR;
}

void device_read (device_t *device, uint8_t *buf)
{
	uint8_t	addr = device->addr;
	uint8_t	cmd_id = device->read[0];
	uint8_t	ret_len = device->read[1];	// return length in bytes
	uint8_t	i;
	uint8_t	*b = buf;

	i2c_start_write (addr);
	i2c_write (cmd_id);

	i2c_start_read (addr);

	// read cnt bytes
	for (i=0; i<ret_len; i++, b++)
	{
		*b = i2c_read ((ret_len-1)-i);
	}

	i2c_stop ();
}

void device_decode_name (device_t *device)
{
	switch (device->internal_id)
	{
		case 0x00:			printf ("%s\0", device->name);	break;
		case 0x01:			printf ("%s\0", device->name);	break;
		case 0x02:			printf ("%s\0", device->name);	break;
		case 0x03:
		{
			printf ("%i-x %i-y %i-z\0", device->internal_id,
										device->internal_id,
										device->internal_id);
			break;
		}
		case 0x04:
		{
			printf ("%i-temp\0", device->internal_id);
			break;
		}
	}
}

void device_decode_buffer (device_t *device, uint8_t *buf)
{
	switch (device->internal_id)
	{
		case 0x00:			printf ("%s\n\0", device->name); break;
		case 0x01:			printf ("%s\n\0", device->name); break;
		case 0x02:			printf ("%s\n\0", device->name); break;
		case 0x03:
		{
			printf ("%d %d %d\0", (((int16_t)buf[0])<<8)|buf[1],
								  (((int16_t)buf[2])<<8)|buf[3],
								  (((int16_t)buf[4])<<8)|buf[5]);
			break;
		}
		case 0x04:
		{
			printf ("%d\0", ~(int8_t)buf[0]);
			break;
		}
	}
}

void main (void)
{
	uint8_t		addr;
	device_t	*active[16] = {0};
	uint8_t		active_len = 0;

	uart_init ();
	i2c_init ();

	printf ("scanning\n\0");

	for (addr=0;addr<128;addr++)
	{
		uint8_t		dev_id;

		if (i2c_start_write (addr) == 0)			{i2c_stop (); continue;}

		i2c_stop ();

		printf ("DEV on %03d\n\0", addr);

		for (dev_id=0; dev_id<DEVICE_COUNT; dev_id++)
		{

			if (device_check_id (addr, &devices[dev_id]) != ERR_DEVICE_NO_ERROR)		{continue;}

			// do the initialisation routine
			if (device_init (&devices[dev_id]) != ERR_DEVICE_NO_ERROR)					{continue;}

			// add the device to the active device list
			active[active_len++] = &devices[dev_id];

			// keep going, we have multiple definitionso of the same device
		}

		// FIXME: how to we note unknown devices?
	}

	// read from the devices forever
	uint8_t 	i;
	uint8_t		*buf = NULL;
	uint8_t		buf_len = 0;

	// output the headers
	for (i=0; i<active_len; i++)
	{
		device_t	*dev = active[i];

		device_decode_name (dev);
		printf (" \0");
	}

	printf ("\n\0");

	// FIXME: find the maximum buffer needed over all the devices
	for (i=0; i<active_len; i++)
	{
		device_t	*dev = active[i];
		buf_len = (dev->read[1]>buf_len)?dev->read[1]:buf_len;
	}

	// alloc the read buffer
	buf = malloc (buf_len);

	while (1)
	{
		for (i=0; i<active_len; i++)
		{
			device_t	*dev = active[i];

			// alloc a read buffer
			memset (buf, 0, dev->read[1]);

			// do the read routine
			device_read (dev, buf);

			// print the buffer
			device_decode_buffer (dev, buf);
			printf (" \0");
		}

		printf ("\n\0");
	}

	// cleanup
	free (buf);
}
