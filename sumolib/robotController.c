/**
 * Robot Framework
 * uses Dwengo-library (www.dwengo.org)
 * 
 * Start from this framework to program your own intelligent robot
 * IMPORTANT: if (old) WELEK board is used add Project > Build Option > Project > MPLAB C18 macro WELEK
 *
 * Version: 1.4 (modified for WELEK sumo robot competition)
 * Date: 15-02-2011
 * (c) Francis wyffels
 */

#define NO_LCD

#include <p18f4550.h>			// include p18f4455 library
#include <stdlib.h>
#include <usart.h>
#include "dwengoBoard.h"
#include "dwengoConfig.h"
#include "dwengoMotor.h"
#include "dwengoADC.h"
#include "dwengoDelay.h"
//#include "dwengoLCD.h"
#include "aiLib.h"
#include "serialDebug.h"

// put your macro's here
#define MAX_TIMER_SERVO 64536 // maximal servo left position: DO NOT EDIT THIS
#define MIN_TIMER_SERVO 62086 // maximal servo right position: DO NOT EDIT THIS

// interrupt handlers
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();

// initalization of USART
void initUsart(void);

// global variables
unsigned int timerServo;	// determines position of servo DO NOT EDIT

// Bootloader occupy addresses 0x00-0x1FFF and thus cannot be used.
// Addresses 0x00, 0x08 and 0x18 have to be remapped
extern void _startup(void);
#pragma code REMAPPED_RESET_VECTOR = 0x2000
void _reset(void) {
	_asm
		goto _startup
	_endasm
}

#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = 0x2008
void Remapped_High_ISR(void) {
	_asm
		goto YourHighPriorityISRCode
	_endasm
}

#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = 0x2018
void Remapped_Low_ISR(void) {
	_asm
		goto YourLowPriorityISRCode
	_endasm
}

// mapping interrupt vectors
#pragma code high_vector=0x08
void high_vector() {
	_asm
		goto 0x2008
	_endasm
}
#pragma code

#pragma code low_vector=0x18
void low_vector() {
	_asm
		goto 0x2018
	_endasm
}
#pragma code

// interrupt handler routines DO NOT EDIT
#pragma interrupt YourHighPriorityISRCode
void YourHighPriorityISRCode() {
#ifdef SERVO
  if(PIR1bits.TMR1IF == TRUE) {	// check interrupt flag: from timer?
    if(PORTBbits.RB5 == 0) {
      TMR1H = (timerServo & 0xFF00) >> 8; // keep most significant byte
      TMR1L = timerServo & 0x00FF; // keep least significant byte
      PORTBbits.RB5 = 1; // high pulse
    } else {
      TMR1H = 0x8A;	// timer 1 interrupt after 83.333ns*8*(65536-35536) = +/- 20ms
      TMR1L = 0xD0;	// 35536 = 0x8AD0
      PORTBbits.RB5 = 0; // low pulse
    }
    PIR1bits.TMR1IF = FALSE; // reenable TMR1 interrupt
  }
#else
  ailib_isr();
#endif
}

#pragma interruptlow YourLowPriorityISRCode
void YourLowPriorityISRCode() {

}

void main(void) {
	BYTE started = FALSE;
	int sensor[3];
	int i;
	// add your own variables here: EDIT

	// start of initializations: DO NOT EDIT
	initBoard();
	initADC();
	initMotors();
	backlightOn();
	stopMotors();
	delay_ms(10);	// wait a bit after initialisation
	// end of initializations

	// start procedure necessary for the robot competition: DO NOT EDIT
	// robot has to move to the control loop when pressing switch S (new version) or S4 (old version)
	initLCD();
	clearLCD();

	appendStringToLCD("Press a key to calibrate sensors.");

	while(1) {
		stopMotors();
		if(SW_C == PRESSED) {
			LEDS = 0xFF;
			ailib_init();
			break;
		} else {
			delay_ms(50);
		}
	}
	delay_ms(300);
	LEDS = 0;

	appendStringToLCD("(S)tart me");
	while(!started) {
		stopMotors();
		if(SW_N == PRESSED || SW_S == PRESSED) {
			initState(DIR_FORWARD);
			started = TRUE;
		} else if(SW_E == PRESSED) {
			initState(DIR_LEFT);
			started = TRUE;
		} else if(SW_W == PRESSED) {
			initState(DIR_RIGHT);
			started = TRUE;
		} else {
			delay_ms(50);
		}
	}
	// ~5 seconds waiting
	clearLCD();
	appendStringToLCD("Start counting");
	LEDS = 0;
	for(i=0;i<5;i++) {
		LEDS *= 2;
		LEDS += 1;
		delay_ms(920);
	} // end of starting procedure
	clearLCD();
	appendStringToLCD("EXTERMINATE!");

#ifdef SERVO
	// routines for servo control DO NOT EDIT
	// servo pins
	INTCON2bits.NOT_RBPU = 1; // disable internal pull-ups: SWITCHES WON'T WORK ANYMORE
  	TRISBbits.TRISB5 = 0; // set RB5 as output
  	PORTBbits.RB5 = 0; // initialize with 0

  	// Timer 1 interrupt example, control one servo on pin RD0
  	// timings servo has to be within 0.7 ms up to 2.3 ms
  	// 0.7 ms: timer 1 interrupt after 83.333ns*8*(65536-64536) = 0.7ms (64486 = 0xFBE6)
  	// 2.3 ms: timer 1 interrupt after 83.333ns*8*(65536-62086) = 2.3ms (62086 = 0xF286)
  	timerServo = MIN_TIMER_SERVO; // in left position
  	PIR1bits.TMR1IF = 0;
  	RCONbits.IPEN = 1;
  	RCONbits.SBOREN = 0;	// BOR = off
  	TMR1H = (timerServo & 0xFF00) >> 8; // keep most significant byte
  	TMR1L = timerServo & 0x00FF; // keep least significant byte
  	T1CON = 0b00110001; // 48Mhz/4, prescaler 1/8
  	INTCONbits.GIEH = TRUE; // GIEH = TRUE
  	IPR1bits.TMR1IP = 1;
	PIE1bits.TMR1IE = 1;
	// end of servo control routines
#endif

	/********************** MODIFY CODE STARTING FROM HERE ***********************/
	// Write your controller inside this while-loop: YOU HAVE TO EDIT THIS
	ailib_init();
	while(TRUE) { // do forever
		doMove();
	}
}

