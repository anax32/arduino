#include <avr/io.h>
	
#include "../utils/uart.h"

void adc_init(void)
{
    ADCSRA	|=	(1<<ADEN)|
				(1<<ADPS0)|
				(1<<ADPS1)|
				(1<<ADPS2);	//ENABLE ADC, PRESCALER 128

    ADMUX	|=	(1<<REFS0);	//PC0, AVcc AS REFERENCE VOLTAGE
}

uint16_t adc_read(uint8_t ch)
{
    ch&=0b00000111;				//ANDing to limit input to 7
    ADMUX = (ADMUX & 0xf8)|ch;	//Clear last 3 bits of ADMUX, OR with ch
    ADCSRA |= (1<<ADSC);		//START CONVERSION

    while((ADCSRA)&(1<<ADSC))	//WAIT UNTIL CONVERSION IS COMPLETE
		;

    return (ADC);				//RETURN ADC VALUE
}

int main (void)
{
    uint16_t x,y,z;

	uart_init();

	printf ("uart initialised\n\0");

    adc_init();         //INITIALIZE ADC

	printf ("adc initialiased\n\0");

    while (1)
    {
        x=adc_read(0);      //READ ADC VALUE FROM CHANNEL 0
        y=adc_read(1);      //READ ADC VALUE FROM CHANNEL 1
        z=adc_read(2);      //READ ADC VALUE FROM CHANNEL 2

		printf ("%i %i %i\n\0", x, y, z);
    }

	return 0;
}
