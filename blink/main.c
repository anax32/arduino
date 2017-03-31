#include <avr/io.h>
#include <util/delay.h>

#define HIGH				0b00100000
#define LOW					0b00000001

#define IS_HIGH(x)			(x & HIGH)
#define IS_LOW(x)			(x & LOW)

#define SET_HIGH(x)			(x = HIGH)
#define SET_LOW(x)			(x = LOW)

int main (void)
{
	/* set PORTB for output*/
	DDRB = 0xFF;

	int		dir = 1;
	SET_HIGH (PORTB);

	while (1)
	{
		_delay_ms (100);

		if (IS_HIGH (PORTB) != 0)
			dir = 0;

		if (IS_LOW (PORTB) != 0)
			dir = 1;

		if (dir == 1)
		{
			PORTB = PORTB << 1;
		}
		else
		{
			PORTB = PORTB >> 1;
		}
	}

	return 1;
}
