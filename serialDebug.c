#include "serialDebug.h"
#include <usart.h>
#include "sumolib/dwengoBoard.h"

char buffer[2];

#if 0
static void printBase64(unsigned char sixbit)
{
	sixbit &= 63;
	if(sixbit < 26)
	{
		printChar('A'+sixbit);
	}
	else if(sixbit < 52)
	{
		printChar('a'+(sixbit-26));
	}
	else if(sixbit < 62)
	{
		printChar('0'+(sixbit-52));
	}
	else if(sixbit == 62)
	{
		printChar('+');
	}
	else if(sixbit == 63)
	{
		printChar('/');
	}
}
#endif

void initDebug()
{
	if(!DEBUG)
		return;

//	CloseUSART();
	TRISCbits.TRISC7 = 1; // set TRISC<7>
	TRISCbits.TRISC6 = 0; // clr TRISC<6>

	PORTCbits.RC7 = 1;

	OpenUSART(	USART_TX_INT_OFF &
				USART_RX_INT_OFF &
				USART_ASYNCH_MODE &
				USART_EIGHT_BIT &
				USART_CONT_RX &
				USART_BRGH_HIGH,
				155);

	buffer[0] = '0';
	buffer[1] = '\0';

	LEDS = 8;

	while(BusyUSART());
	//putsUSART("hello world\n");
}

void puts_(char* message)
{
	if(!DEBUG)
		return;

	printString(message);
	printString("\r\n");
}

void printChar(char c)
{
	if(!DEBUG)
		return;

	/*i++;
	LEDS |= 32;
	if(c == 'S')
		LEDS |= 16;

	while(BusyUSART());
	/*if(i==1)
		putcUSART('#');
	else
		putcUSART('$');*/
	/*putcUSART(c);*/

	buffer[0] = c & 0x7F;
	putsUSART(buffer);
}

void printString_(char* message)
{
	if(!DEBUG)
		return;

	putrsUSART((const rom far char*)message);

	/*while(*message != 0)
		printChar(*(const rom far char *)message++);*/
}

static int pow(int num, int exp) {
	int result = 1;
	while(exp > 0) {
		result *= num;
		exp--;
	}
	return result;
}

void printInt(int number)
{
	int left, charsLeft = 0, factor, result;

	if(!DEBUG)
		return;

	if(number == 0)
	{
		printChar('0');
		return;
	}

	if(number < 0)
	{
		number = -number;
		printChar('-');
	}

	left = number;

	while(left > 0)
	{
		charsLeft++;
		left /= 10;
	}

	while(charsLeft > 0)
	{
		charsLeft--;
		factor = pow(10, charsLeft);
		result = number/factor;
		printChar('0' + result);
		number -= result * factor;
	}
}