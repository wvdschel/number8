#include "sumolib/dwengoBoard.h"
#include "util.h"
#include <usart.h>
#include "serialDebug.h"

short speedMotor1;
short speedMotor2;

#define USE_MOTORS 1

void initMotors()
{
	speedMotor1 = 0;
	speedMotor2 = 0;

	if(!USE_MOTORS)
		return;

	//CloseUSART();
	TRISCbits.TRISC7 = 1; // set TRISC<7>
	TRISCbits.TRISC6 = 0; // clr TRISC<6>

	OpenUSART(	USART_TX_INT_OFF &
				USART_RX_INT_OFF &
				USART_ASYNCH_MODE &
				USART_EIGHT_BIT &
				USART_CONT_RX &
				USART_BRGH_HIGH,
				155); // 48.000.000 / (16 * (155 + 1)) ~= 19231
}

void setSpeedMotor(short speed, char forward, char reverse)
{
	unsigned char speedData = (unsigned char)(abs(speed) >> 3); // clear sign bit and divide by 8

	if(DEBUG)
	{
		printString("Code to motor: ");
		printInt((int)(speed >= 0 ? (unsigned const char)forward : (unsigned const char)reverse));
		printString(" ");
		printInt((int)speedData);
		puts("");
	}

	if(!USE_MOTORS)
		return;
	while(BusyUSART());	
	putcUSART(0x80);
	while(BusyUSART());	
	putcUSART(0x07);
	while(BusyUSART());	
	putcUSART(speed >= 0 ? (unsigned const char)forward : (unsigned const char)reverse);
	while(BusyUSART());
	putcUSART(speedData);
}

void setSpeedMotor1(short speed)
{
	if(speed != speedMotor1)
	{
		if(DEBUG)
		{
			printString("Changing motor setting, now is [left = ");
			printInt(speed);
			printString("] and [right = ");
			printInt(speedMotor2);
			puts("]");
		}
		setSpeedMotor(speed, (unsigned const char)0x4E, (unsigned const char)0x4D);

		speedMotor1 = speed;
	}
}

void setSpeedMotor2(short speed)
{
	if(speed != speedMotor2)
	{
		if(DEBUG)
		{
			printString("Changing motor setting, now is [left = ");
			printInt(speedMotor1);
			printString("] and [right = ");
			printInt(speed);
			puts("]");
		}
		setSpeedMotor(speed, (unsigned const char)0x46, (unsigned const char)0x45);

		speedMotor2 = speed;
	}
}

void stopMotors(void)
{
	setSpeedMotor1(0);
	setSpeedMotor2(0);
}