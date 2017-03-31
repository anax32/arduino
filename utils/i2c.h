#include <inttypes.h>
#include <compat/twi.h>

#define I2C_READ    1
#define I2C_WRITE   0

/* I2C clock in Hz */
#define SCL_CLOCK  100000L

// setup the i2c bus
#define i2c_init()					{TWSR=0; TWBR = ((F_CPU/SCL_CLOCK)-16)/2;}

// send a start condition
#define i2c_send_start()			{TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);}
// send a stop condition
#define i2c_send_stop()				{TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);}
// send data byte
#define i2c_send_data(x)			{TWDR=x; TWCR=(1<<TWINT)|(1<<TWEN);}
// send an address
#define i2c_send_addr(x)			{i2c_send_data(x)}

// wait for the control register
#define i2c_wait()					{while (!(TWCR & (1<<TWINT)));}
// loop while stop is in the control register
#define i2c_wait_stop()				{while (TWCR & (1<<TWSTO));}

// check status register is start or rep start
#define i2c_status_is_start()		((TW_STATUS!=TW_START)&&(TW_STATUS!=TW_REP_START))
// check status register is ack
#define i2c_status_is_ack()			((TW_STATUS==TW_MT_SLA_ACK)||(TW_STATUS==TW_MR_SLA_ACK))
// check status register is nack
#define i2c_status_is_nack()		((TW_STATUS==TW_MT_SLA_NACK)||(TW_STATUS==TW_MR_DATA_NACK))
// check status register is data ack
#define i2c_status_is_data_ack()	(TW_STATUS==TW_MT_DATA_ACK)

// get the data register value
#define i2c_data()					(TWDR)

// set control to int, enable and ack
#define i2c_control_ack()			(TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA))
// set control to int, enable
#define i2c_control_nack()			(TWCR=(1<<TWINT)|(1<<TWEN))

// send a start condition to the bus
// send a device address to initiate coms
// return the presence of an ack bit in the status register
unsigned char i2c_start (unsigned char address)
{
	i2c_send_start ();
	i2c_wait ();

	if (i2c_status_is_start ())		{return 1;}

	i2c_send_addr (address);
	i2c_wait ();

	return (i2c_status_is_ack ());
}

// start writing to the device at the address
unsigned char i2c_start_write (unsigned char addr)		{return i2c_start ((addr<<1)|I2C_WRITE);}
// start reading from the device at the address
unsigned char i2c_start_read (unsigned char addr)		{return i2c_start ((addr<<1)|I2C_READ);}

// send a repeated start to the addressed device
unsigned char i2c_rep_start (unsigned char addr)		{return i2c_start (addr);}
// send a repeated start to the addressed device with a write flag
unsigned char i2c_rep_start_write (unsigned char addr)	{return i2c_start ((addr<<1)|I2C_WRITE);}
// send a repeated start to the addressed device with a read flag
unsigned char i2c_rep_start_read (unsigned char addr)	{return i2c_start ((addr<<1)|I2C_READ);}

// send a stop condition and wait for bus release
void i2c_stop (void)									{i2c_send_stop (); i2c_wait_stop ();}

// write a byte to the previously addressed device and wait for completion
// return true if the status register is ack, false otherwise
unsigned char i2c_write (unsigned char data)
{
	i2c_send_data (data);
	i2c_wait ();
	return i2c_status_is_data_ack ();
}

// FIXME: read the register before setting the flags incase
// we cause a new value to be written over during the wait
// call.
// FIXME: take a pointer to a buffer for storage.
// FIXME: take a length for the buffer and fill a range.
// return the contents of the data register and acknowledge
unsigned char i2c_read_ack (void)
{
	i2c_control_ack ();
	i2c_wait ();
	return i2c_data ();
}

// return the contents of the data register, do not acknowledge
unsigned char i2c_read_nak (void)
{
	i2c_control_nack ();
	i2c_wait ();
	return i2c_data ();
}

// return the contents of the data register, acknowledge if send_ack is != 0
unsigned char i2c_read (unsigned char send_ack)
{
	if (send_ack == 0)
		i2c_control_nack ();
	else
		i2c_control_ack ();

	i2c_wait ();
	return i2c_data ();
}

// attempt to start a connection with the device at address
// loop until possible
// this fn requires addr<<1 and WRITE or READ added
void i2c_start_wait (unsigned char addr)
{
    while (1)
    {
		i2c_send_start ();			// send START condition
		i2c_wait ();			    // wait until transmission completed

    	// check value of TWI Status Register. Mask prescaler bits.
    	if (!i2c_status_is_start ())	{continue;}

		i2c_send_addr (addr);	    // send device address
		i2c_wait ();				// wail until transmission completed

    	// check value of TWI Status Register. Mask prescaler bits.
    	if (i2c_status_is_nack ())
    	{
			i2c_send_stop ();    	// device busy, send stop condition to terminate write operation
			i2c_wait_stop ();		// wait until stop condition is executed and bus released
    	    continue;
    	}

    	break;
     }
}
