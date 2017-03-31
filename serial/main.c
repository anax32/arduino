#include "../utils/uart.h"

int main (void)
{
	uart_init();

	while (1)
	{
		printf ("Hello, little bear!\n\0");
	}

	return 0;
}
