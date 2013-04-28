#define QUADENC_PULLDOWN

#include "util.h"
#include "aiLib.h"
#include "sumolib/dwengoMotor.h"
#include "serialDebug.h"
#include "quadenclib/quadenc.h"

// State switching macro
#define SWITCH_STATE(nState)	{ printInt(__LINE__); printString(": Switching to new state "); printInt(nState); puts(""); currState = nState; stateTimer = 0; }
// Motor control macros
#define MOTOR_LEFT(dir)			currDirMotorLeft = dir; setSpeedMotor2(dir);
#define MOTOR_RIGHT(dir)		currDirMotorRight = dir; setSpeedMotor1(-(dir));

// Possible states for the robot
#define STATE_SEEK		 	1			// Looking for the enemy
#define STATE_DESTROY 		2			// Enemy in front of robot, try to push it
#define STATE_FLANK			3			// Pushing didn't work, try to get around the enemy.
#define STATE_SURVIVE	 	4			// White edge detected, move away


// Sensor calibration related stuff
#define DIFF_THRESHOLD 		5			// Maximum difference between two long range sensors, if the diff is higher, the robot will reallign.
#define DISTANCE_CLOSE		45			// Anything lower than this value is considered close, anything higher is considered infinitely far away
#define SEEK_ROTATE_TIME 	(int)150 	// How many iterations should we wait before moving the robot to change the search space.
							 			// To disable this and just turn left all the time, set to 0.
#define SEEK_MOVE_TIME		(int)100	// How long the robot should move forward before looking around again.
#define SAMPLE_COUNT		10			// The number of samples for each measurement

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

#define PROGRESS_WHEEL
#define PROGRESS_WHEEL_TIMER 58036		// Internal clock frequency is 48Mhz because of the PLL settings. Using fosc/4 and
										// a prescaler of 1/8 for timer 1 in 16 bit mode, this value gives us ?? ms between
										// interrupts, not including interrupt processing.
#define MIN_PROGRESS		5			// Minimal progress to continue pushing.

#define SLEEP_TIME			((int)20)

// Global vars
static int sensor[7]; 					// Sensor readings
static int currState = STATE_SEEK;		// Current state of the robot
static int stateTimer = -1;				// How long have we been in the current state?
static int currDirection = 0;			// Current direction of the engines
static int currDirMotorLeft = 0;		// Direction of each motor
static int currDirMotorRight = 0;
static int initialGroundReading[4];		// Initial readings from the ground sensors - this is our reference for "black"
static int initialEyeLeft = 0;			// Initial readings for long distance sensors, to account for noise etc.
static int initialEyeRight = 0;
static int initialEyeAvg = 0;
static int progress = 0;				// Progress over the past 100 turns combined.
#define PROGRESS_HISTORY_SIZE	((int)15)
static int progressHistory[PROGRESS_HISTORY_SIZE] = {0};
										// Circular buffer of 100 turns raw input from the progress wheel.
static int progressHistoryIndex = 0;	// Current end of the circular buffer.
static int progressRaw = 0;				// Progress as reported by the progress wheel. This value is reset each turn.

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

#include "tests.c"

void doMove()
{
	int distanceLeft, distanceRight, distanceAvg;

	flankTheBox();

	if(stateTimer >= 1000000)
		stateTimer = 0;
	stateTimer += 1;

	readSensors();
	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;
	
	switch(currState) {
	case STATE_SEEK: // Turn right for a while, then turn left until you find an enemy.
		if(survivalCheck())
			break;
		// If we just switched, start turning.
		if(stateTimer == 1)
		{
			if(currDirection & DIR_RIGHT)
				setMotors(DIR_RIGHT);
			else
				setMotors(DIR_LEFT);
		}
		if(abs(distanceLeft - distanceRight) > DIFF_THRESHOLD)
		{
			// Sensors are far apart each other, one sensor might have found something, turn toward that sensor
			if(distanceLeft < distanceRight)
				setMotors(DIR_LEFT);
			else
				setMotors(DIR_RIGHT);
			break; // No additional checks are needed, on to the next iteration.
		} else if(distanceAvg < DISTANCE_CLOSE) {
			// Sensors are close to each other, and indicate an object is close by: target found.
			SWITCH_STATE(STATE_DESTROY);
			setMotors(DIR_FORWARD);
			break;
		}
		{
			int currentStage = stateTimer % (SEEK_MOVE_TIME+SEEK_ROTATE_TIME);
			// Nothing found, check if we need to move or rotate
			if(currDirection == DIR_FORWARD && currentStage == 0)
			{
				// Enough moving, lets search again.
				puts("LEFT!");
				setMotors(DIR_LEFT);
			}
			else if(currDirection != DIR_FORWARD && currentStage == SEEK_ROTATE_TIME)
			{
				// Enough searching, nothing found, move a little
				puts("FORWARD!");
				setMotors(DIR_FORWARD);
			}
		}
		break;
	case STATE_DESTROY:
		// Switch state if we're driving over the white line.
		if(survivalCheck())
			break;
#ifdef PROGRESS_WHEEL
		if(stateTimer == 0) // Reset counter if we just started pushing
		{
			wipeProgressHistory();
		}
		// Once we have filled up the history, and our combined progress is insufficient, consider flanking
		if(progress < MIN_PROGRESS && stateTimer > PROGRESS_HISTORY_SIZE)
		{
		}
#endif
		if(distanceLeft > DISTANCE_CLOSE && distanceRight > DISTANCE_CLOSE)
		{
			// Target lost, go back to searching
			SWITCH_STATE(STATE_SEEK);
			break;
		}
		if(abs(distanceLeft - distanceRight) > DIFF_THRESHOLD)
		{
			// Sensors are appart, reallign.
			if(distanceLeft > distanceRight)
				setMotors(DIR_FORWARD | DIR_RIGHT);
			else
				setMotors(DIR_FORWARD | DIR_LEFT);
		} else // Target straight ahead
			setMotors(DIR_FORWARD);
		break;
	case STATE_SURVIVE:
		if(!survivalCheck() && stateTimer > SURVIVE_TIME) {
			puts("Done with surviving, I welcome death.");
			SWITCH_STATE(STATE_SEEK);
		}
		break;
	default:
		puts("Illegal state");
		SWITCH_STATE(STATE_SEEK);
		delay_ms(SLEEP_TIME * 10);
	}

	LEDS = (LEDS & 0x1F) | (currState << 5);
	if(DEBUG && stateTimer % 25 == 0)
		printState();
	delay_ms(SLEEP_TIME);
}

int survivalCheck()
{
	if(pushSensor(DIR_BACK))
	{
		setMotors(0);
		return TRUE;
	}
	//return FALSE;
	if((groundSensor(DIR_FORWARD | DIR_LEFT) && groundSensor(DIR_FORWARD | DIR_RIGHT)))
	{
		puts("Line in front");
		setMotors(DIR_BACK);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_BACK | DIR_LEFT) && groundSensor(DIR_BACK | DIR_RIGHT))
	{
		puts("Line at back");
		setMotors(DIR_FORWARD);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_BACK | DIR_LEFT) && groundSensor(DIR_FORWARD | DIR_LEFT))
	{
		puts("Line at left");
		setMotors(DIR_RIGHT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_BACK | DIR_RIGHT) && groundSensor(DIR_FORWARD | DIR_RIGHT))
	{
		puts("Line at right");
		setMotors(DIR_LEFT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_BACK | DIR_LEFT))
	{
		puts("Line at left back");
		setMotors(DIR_FORWARD | DIR_RIGHT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_BACK | DIR_RIGHT))
	{
		puts("Line at right back");
		setMotors(DIR_FORWARD | DIR_LEFT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_FORWARD | DIR_LEFT))
	{
		puts("Line in left front");
		setMotors(DIR_BACK | DIR_RIGHT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	if(groundSensor(DIR_FORWARD | DIR_RIGHT))
	{
		puts("Line in right front");
		setMotors(DIR_BACK | DIR_LEFT);
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	return FALSE;
}

void initState(int direction) {
	if(direction == DIR_FORWARD) {
		SWITCH_STATE(STATE_SEEK);
		stateTimer = SEEK_ROTATE_TIME-1;
	} else {
		SWITCH_STATE(STATE_SEEK);
		currDirection = 0x1000 | direction;
	}
}

void wipeProgressHistory()
{
	int i;
	puts("Wiping progress history.");
	//memset(progressHistory, 0, sizeof(progressHistory)); // This shit doesn't work.
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

void setMotors(int direction)
{
	if(currDirection == direction)
		return; // Don't needlessly change the signal
	MOTOR_LEFT(1023);
	MOTOR_RIGHT(1023);
	if(direction & DIR_FORWARD) 
	{
		if(direction & DIR_RIGHT)
		{
			MOTOR_RIGHT(0);
		} else if(direction & DIR_LEFT) {
			MOTOR_LEFT(0);
		}	
	} else if(direction & DIR_BACK) {
		MOTOR_LEFT(-1023);
		MOTOR_RIGHT(-1023);
		if(direction & DIR_RIGHT)
		{
			MOTOR_LEFT(0);
		} else if(direction & DIR_LEFT) {
			MOTOR_RIGHT(0);
		}
	} else {
		// No real movement, may be turning in place
		if(direction & DIR_RIGHT)
		{
			MOTOR_LEFT(1023);
			MOTOR_RIGHT(-1023);
		} else if(direction & DIR_LEFT) {
			MOTOR_LEFT(-1023);
			MOTOR_RIGHT(1023);
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
			return sensor[2] * WHITE_BLACK_RATIO > initialGroundReading[0];  // Front right
		else
			return sensor[4] * WHITE_BLACK_RATIO > initialGroundReading[2];  // Front left
	} else {
		if(sensor_dir & DIR_LEFT)
			return sensor[3] * WHITE_BLACK_RATIO > initialGroundReading[1];  // Rear left
		else
			return sensor[5] * WHITE_BLACK_RATIO > initialGroundReading[3];  // Rear right
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
		return ((1024 + initialEyeRight) - sensor[0])/20;
	else
		return ((1024 + initialEyeLeft)  - sensor[1])/20;
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
	case STATE_SEEK:
		printString("SEE"); // (const rom far char *) ?
		break;
	case STATE_DESTROY:
		printString("DES");
		break;
	case STATE_FLANK:
		printString("FLA");
	case STATE_SURVIVE:
		printString("SUR");
		break;
	default:
		printString("INV");
	}
	printChar(',');
	printInt(stateTimer);
	printChar(',');

	// Print out motor state
	printInt(currDirMotorLeft);
	printInt(currDirMotorRight);
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
	printString("Raw sensor readings: ");
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
	puts("");
}