#include <avr/io.h>
#include <stdio.h>

#ifndef F_CPU
/*#pragma message ("F_CPU NOT SET, SETTING TO 16000000UL")*/
  #define F_CPU     16000000UL
#endif

#ifndef BAUD
/*#pragma message ("BAUD NOT SET, SETTING TO 9600")*/
  #define BAUD      9600
#endif
#include <util/setbaud.h>

/* http://www.cs.mun.ca/~rod/Winter2007/4723/notes/serial/serial.html */
/* https://github.com/tuupola/avr_demo/blob/master/blog/simple_usart/ */

void uart_putchar (char c, FILE *stream)
{
  loop_until_bit_is_set (UCSR0A, UDRE0);
  UDR0 = c;
}

char uart_getchar(FILE *stream)
{
  loop_until_bit_is_set (UCSR0A, RXC0);
  return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

/**
 * Set some register values and redirect the std files
 */
void uart_init (void)
{
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~(_BV(U2X0));
#endif
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */

  /* write the streams */
  stdout = &uart_output;
  stdin = &uart_input;
}
