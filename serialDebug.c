#include "serialDebug.h"
#include "util.h"
#include <usart.h>
#include "sumolib/dwengoBoard.h"

char buffer[2];

void initDebug() {
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

void puts_(char* message) {
	if(!DEBUG)
		return;

	printString_(message);
	printString("\r\n");
}

void printChar(char c) {
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

	buffer[0] = c;
	putsUSART(buffer);
}

void printString_(char* message) {
	if(!DEBUG)
		return;

	putrsUSART((const rom far char*)message);

	/*while(*message != 0)
		printChar(*(const rom far char *)message++);*/
}

void printInt(int number) {
	int left, charsLeft = 0, factor, result;

	if(!DEBUG)
		return;

	if(number == 0) {
		printChar('0');
		return;
	}

	if(number < 0) {
		number = -number;
		printChar('-');
	}

	left = number;

	while(left > 0) {
		charsLeft++;
		left /= 10;
	}

	while(charsLeft > 0) {
		charsLeft--;
		factor = pow(10, charsLeft);
		result = number/factor;
		printChar('0' + result);
		number -= result * factor;
	}
}