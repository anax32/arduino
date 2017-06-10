#include <avr/io.h>           /* PORT identifiers */
#include "../utils/uart.h"    /* serial out for debugging */

int main (void)
{
  uart_init();

  while (1)
  {
    printf ("Hello, little bear!\n\0");
  }

  return 0;
}
