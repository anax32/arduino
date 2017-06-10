#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
  TCCR1A |= (1<<COM1A1) |
            (1<<WGM11);       /* non-inverting mode for OC1A */
  TCCR1B |= (1<<WGM13) |
            (1<<WGM12) |
            (1<<CS11);        /* Mode 14, Prescaler 8 */

  ICR1 = 40000;               /* 320000 / 8 = 40000 */

  DDRB |= (1<<PB1);           /* OC1A set to output */

  int i=0;

  for (i=0;i<4000;i+=100)
  {
    OCR1A = i;                /* set to 0° --> pulsewidth = 1ms */
    _delay_ms(400);

#if 0
    /* alternatives */
    OCR1A = 3000;             /* set to 90° --> pulsewdith = 1.5ms */
    _delay_ms(1000);

    OCR1A = 4000;             /* set to 180° --> pulsewidth = 2ms */
    _delay_ms(1000);
#endif
  }
}
