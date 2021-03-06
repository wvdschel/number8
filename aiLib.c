#define QUADENC_PULLDOWN

#define AI_LIB

#include "aiLib.h"
#include "states.h"
#include "sumolib/dwengoMotor.h"
#include "serialDebug.h"
#include "quadenclib/quadenc.h"

// Motor control macros
#define MOTOR_LEFT(dir)			if(currSpeedMotorLeft != dir)  { currSpeedMotorLeft = dir; setSpeedMotor2(dir); }
#define MOTOR_RIGHT(dir)		if(currSpeedMotorRight != dir) { currSpeedMotorRight = dir; setSpeedMotor1(-(dir)); }

#define SAMPLE_COUNT		10			// The number of samples for each sensor measurement

//#define BLACK_EDGE_WHITE_BOARD
#ifdef BLACK_EDGE_WHITE_BOARD
// In my living room, I use a white-ish board with black edges.
// White values range from 370 to almost 600, black values are always over 800.
#define WHITE_BLACK_RATIO	0.75
#else
// By default, we're using a black board with a white edge.
#define WHITE_BLACK_RATIO	0.3			// The ratio of readings on a black floor vs a white floor. white <= ratio * black.
#endif

#define SURVIVE_TIME		40			// How long the robot stays in survival mode after the "threat" has gone away.

#define PROGRESS_WHEEL_TIMER 61036		// Internal clock frequency is 48Mhz because of the PLL settings. Using fosc/4 and
										// a prescaler of 1/8 for timer 1 in 16 bit mode, this value gives us 3 ms between
										// interrupts, not including interrupt processing.

#define SLEEP_TIME			((int)20)

// Global vars
int sensor[7]; 					// Sensor readings
int currState = STATE_SCAN;		// Current state of the robot
int stateTimer = -1;			// How long have we been in the current state?
int stateProgress = 0;			// How much distance have we covered in this state?
int currDirection = 0;			// Current direction of the engines
int currSpeedMotorLeft = 0;		// Speed of each motor
int currSpeedMotorRight = 0;
int initialGroundReading[4];	// Initial readings from the ground sensors - this is our reference for "black"
int initialEyeLeft = 0;			// Initial readings for long distance sensors, to account for noise etc.
int initialEyeRight = 0;
int initialEyeAvg = 0;
int progress = 0;				// Progress over the past 100 turns combined.
int progressHistory[PROGRESS_HISTORY_SIZE] = {0};
								// Circular buffer of 100 turns raw input from the progress wheel.
int progressHistoryIndex = 0;	// Current end of the circular buffer.
int progressRaw = 0;			// Progress as reported by the progress wheel. This value is reset each turn.

void ailib_init()
{
	LEDS = 0;
	initSensors();

#ifdef PROGRESS_WHEEL
	quadenc_init();

	/*
	 * This code was copied and adapted from the servo control routine.
	 * TIMER1 is a 16 bit timer. We might be able to use TIMER0, an 8-bit
	 * timer, if we use the right prescaler.
	 */
	INTCON2bits.NOT_RBPU = 1;			// disable internal pull-ups
										// WHY?

	PIR1bits.TMR1IF = 0;				// clear timer 1 interrupt flag
	RCONbits.IPEN = 1;					// enable priority levels on interrupts
	RCONbits.SBOREN = 0;				// turn off software brown-out reset
										// WHY?

	TMR1H = (PROGRESS_WHEEL_TIMER & 0xFF00) >> 8;	// set high byte of timer 1
	TMR1L = (PROGRESS_WHEEL_TIMER & 0x00FF); 		// set low byte of timer 1

	T1CON = 0b00110001; 				// 48Mhz/4, prescaler 1/8, timer 1 enabled
	INTCONbits.GIEH = TRUE;				// enable all high priority interrupts
	IPR1bits.TMR1IP = 1;				// set timer 1 overflow interrupt priority to high
	PIE1bits.TMR1IE = 1;				// enable timer 1 interrupt
										// DIFFERENCE BETWEEN THIS AND T1CON TIMER ENABLED?
#endif
}

void ailib_isr()
{
#ifdef PROGRESS_WHEEL
	if(PIR1bits.TMR1IF) // check if interrupt came from timer 1
	{
		quadenc_isr();

		TMR1H = (PROGRESS_WHEEL_TIMER & 0xFF00) >> 8;	// set high byte of timer 1
		TMR1L = (PROGRESS_WHEEL_TIMER & 0x00FF); 		// set low byte of timer 1

		PIR1bits.TMR1IF = 0; // re-enable timer 1
	}
#endif
}

//#include "tests.c"

void doMove()
{
	//flankTheBox();
	//oneEyedBrake();
	//drive100Clicks();

	if(stateTimer >= 1000000)
		stateTimer = 0;
	stateTimer += 1;

	if(DEBUG && stateTimer % 3 == 0)
		printState();

	readSensors();
	stateProgress += progressRaw;
	
	switch(currState) {
	case STATE_MOVE:
		doMoveState(); break;
	case STATE_SCAN:
		doScanState(); break;
	case STATE_ATTACK:
		doAttackState(); break;
	case STATE_SURVIVE:
		doSurviveState(); break;
	case STATE_FLANK_AWAY:
		doFlankAwayState(); break;
	case STATE_FLANK_TURN:
		doFlankTurnState(); break;
	case STATE_ATTACK_REAR:
		doAttackRearState(); break;
	case STATE_REAR_FLANK:
		doRearFlankState(); break;
	default:
		printString("Illegal state: ");
		printInt(currState);
		puts("");
		SWITCH_STATE(STATE_SCAN);
		delay_ms(SLEEP_TIME * 10);
	}

	if(!DEBUG)
		delay_ms(SLEEP_TIME);
}

void initState(int direction)
{
	currState = STATE_SCAN;
	stateTimer = 0;
	stateProgress = 0;

	if(direction == DIR_FORWARD) {
		SWITCH_STATE(STATE_MOVE);
	} else {
		SWITCH_STATE(STATE_SCAN);
	}
}

void wipeProgressHistory()
{
	int i;
	puts("Wiping progress history.");
	for(i = 0; i < PROGRESS_HISTORY_SIZE; i++)
		progressHistory[i] = 0;
	progress = 0;
	progressHistoryIndex = 0;
}

void initSensors()
{
	int i;
	wipeProgressHistory();

	// Init touch sensors and/or progress wheel
	TRISEbits.TRISE1 = INPUT;
	TRISEbits.TRISE2 = INPUT;
	TRISDbits.TRISD1 = INPUT;
	TRISDbits.TRISD2 = INPUT;

	// Get an initial ground sensor reading
	readSensors();
	for(i = 0; i < 4; i++)
		initialGroundReading[i] = sensor[2+i];
	printString("Initial black value: ");
	for(i = 0; i < 4; i++)
	{
		printInt(initialGroundReading[i]);
		printString(" ");
	}
	puts("");

	initialEyeRight = sensor[0];
	initialEyeLeft  = sensor[1];
	initialEyeAvg   = (initialEyeRight + initialEyeLeft) / 2;
	printString("Initial long distances sensors: ");
	printInt(initialEyeLeft);
	printString(" ");
	printInt(initialEyeRight);
	puts("");
}

void readSensors()
{
	int i, j, total;

	//initADC();
	// Read Sensors #0-5
	for(i=0;i<7;i++)
	{
		total = 0;
		for(j = 0; j < SAMPLE_COUNT; j++)
			total += readADC(i);	// reading analog sensors
		sensor[i] = total / SAMPLE_COUNT;
	}

	progressRaw = 0;
	quadenc_getLastChangeCount(&progressRaw);
	if(progressRaw > 127)
		progressRaw = progressRaw - 256;
	progressRaw = -progressRaw;	// driving forward gives a negative progress, so we negate it here.

	progress -= progressHistory[progressHistoryIndex];
	progress += progressRaw;
	progressHistory[progressHistoryIndex] = progressRaw;
	progressHistoryIndex++;
	if(progressHistoryIndex >= PROGRESS_HISTORY_SIZE)
		progressHistoryIndex = 0;
}

void setMotors(int direction, int speed)
{
	if(speed > 1023)
		speed = 1023;
	if(speed < 0)
		speed = 0;
	if(direction & DIR_FORWARD) 
	{
		if(direction & DIR_RIGHT)
		{
			MOTOR_RIGHT(0);
			MOTOR_LEFT(speed);
		} else if(direction & DIR_LEFT) {
			MOTOR_LEFT(0);
			MOTOR_RIGHT(speed);
		} else {
			MOTOR_LEFT(speed);
			MOTOR_RIGHT(speed);
		}
	} else if(direction & DIR_BACK) {
		if(direction & DIR_RIGHT)
		{
			MOTOR_RIGHT(0);
			MOTOR_LEFT(-speed);
		} else if(direction & DIR_LEFT) {
			MOTOR_LEFT(0);
			MOTOR_RIGHT(-speed);
		} else {
			MOTOR_LEFT(-speed);
			MOTOR_RIGHT(-speed);
		}
	} else {
		// No real movement, may be turning in place
		if(direction & DIR_RIGHT)
		{
			MOTOR_LEFT(speed);
			MOTOR_RIGHT(-speed);
		} else if(direction & DIR_LEFT) {
			MOTOR_LEFT(-speed);
			MOTOR_RIGHT(speed);
		} else {
			MOTOR_LEFT(0);
			MOTOR_RIGHT(0);
		}	
	}
	currDirection = direction;
}

int groundSensor(unsigned sensor_dir)
{
#ifdef BLACK_EDGE_WHITE_BOARD
	if(sensor_dir & DIR_FORWARD) {
		if(sensor_dir & DIR_RIGHT)
			return ABS(sensor[2]-initialGroundReading[0]) > 110;  // Front right
		else
			return ABS(sensor[4]-initialGroundReading[2]) > 110;  // Front left
	} else {
		if(sensor_dir & DIR_LEFT)
			return ABS(sensor[3]-initialGroundReading[1]) > 110;  // Rear left
		else
			return ABS(sensor[5]-initialGroundReading[3]) > 110;  // Rear right
	}
#else
	if(sensor_dir & DIR_FORWARD) {
		if(sensor_dir & DIR_RIGHT)
			return sensor[2] < WHITE_BLACK_RATIO * initialGroundReading[0];
		else
			return sensor[4] < WHITE_BLACK_RATIO * initialGroundReading[2];
	} else {
		if(sensor_dir & DIR_LEFT)
			return sensor[3] < WHITE_BLACK_RATIO * initialGroundReading[1];
		else
			return sensor[5] < WHITE_BLACK_RATIO * initialGroundReading[3];
	}
#endif
}

int distanceSensor(unsigned sensor_dir)
{
	if(sensor_dir == DIR_RIGHT)
		return ((1024 + initialEyeRight) - sensor[0]);
	else
		return ((1024 + initialEyeLeft)  - sensor[1]);
}

int pushSensor(unsigned sensor_dir) {
	//PORT(D/E).R(D/E)(1/2)
	if(sensor_dir == DIR_BACK)
		return PORTEbits.RE1;
	else if(sensor_dir == DIR_FORWARD)
		return PORTEbits.RE2;
#ifndef PROGRESS_WHEEL
	else if(sensor_dir == DIR_RIGHT)
		return PORTDbits.RD1;
	else if(sensor_dir == DIR_LEFT)
		return PORTDbits.RD2;
#endif
	else
		return FALSE;
}

void printState()
{
	int i;
	puts("");
	// Print out state
	switch(currState) {
	case STATE_MOVE:
		printString("MOV");
		break;
	case STATE_SCAN:
		printString("SCN");
		break;
	case STATE_ATTACK:
		printString("ATK");
		break;
	case STATE_SURVIVE:
		printString("SUR");
		break;
	case STATE_FLANK_AWAY:
		printString("FLA");
		break;
	case STATE_FLANK_TURN:
		printString("FLT");
		break;
	case STATE_ATTACK_REAR:
		printString("BSX");
		break;
	case STATE_REAR_FLANK:
		printString("RFL");
		break;
	default:
		printString("INV");
	}
	printChar(',');
	printInt(stateTimer);
	printChar(',');

	// Print out motor state
	printInt(currSpeedMotorLeft);
	printChar(',');
	printInt(currSpeedMotorRight);
	printChar(',');
	
	// Print ground sensor readings
	if(groundSensor(DIR_FORWARD | DIR_RIGHT))
		printString("1,");
	else
		printString("0,");
	if(groundSensor(DIR_BACK | DIR_RIGHT))
		printString("1,");
	else
		printString("0,");
	if(groundSensor(DIR_FORWARD | DIR_LEFT))
		printString("1,");
	else
		printString("0,");
	if(groundSensor(DIR_BACK | DIR_LEFT))
		printString("1,");
	else
		printString("0,");
	
	// Print push sensor readings
	if(pushSensor(DIR_BACK))
		printString("1,");
	else
		printString("0,");
	
	// Print long distance sensors
	printInt(distanceSensor(DIR_LEFT));
	printChar(',');
	printInt(distanceSensor(DIR_RIGHT));
	for(i=0; i < 7; i++) {
		printString(",");
		printInt(sensor[i]);
	}
	printChar(',');

	// Print progress wheel data
	printInt(progress);
	printChar(',');
	printInt(progressRaw);
	printChar(',');
	printInt(stateProgress);
	puts("");
}