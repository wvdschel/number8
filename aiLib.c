#include "util.h"
#include "aiLib.h"
#include "dwengoMotor.h"
#include "serialDebug.h"

// State switching macro
#define SWITCH_STATE(nState)	{ printInt(__LINE__); printString(": Switching to new state "); printInt(nState); puts(""); currState = nState; stateTimer = 0; }
// Engine control macros
#define MOTOR1(dir)				currDirMotor1 = dir; setSpeedMotor1(dir);
#define MOTOR2(dir)				currDirMotor2 = dir; setSpeedMotor2(-(dir));
// Printing macros
#define ENGINE_STATE(dir)		(dir > 0 ? '^' : (dir < 0 ? 'V' : '-'))

// Possible states for the robot
#define STATE_SEEK		 	1		// Looking for the enemy
#define STATE_DESTROY 		2		// Enemy in front of robot, try to push it
#define STATE_DESTROY_OVER	3		// Pushing the enemy, a white line has been detected by our front side, so we must be close to the end of the ring
#define STATE_SURVIVE	 	4		// White edge detected, move away

// Sensor calibration related stuff
#define DIFF_THRESHOLD 		3		// Maximum difference between two long range sensors, if the diff is higher, the robot will reallign.
#define DISTANCE_CLOSE		30		// Anything lower than this value is considered close, anything higher is considered infinitely far away
#define SEEK_ROTATE_TIME 	(int)75 	// How many iterations should we wait before moving the robot to change the search space.
							 			// To disable this and just turn left all the time, set to 0.
#define SEEK_MOVE_TIME		(int)60	// How long the robot should move forward before looking around again.
#define SAMPLE_COUNT		10		// The amount of samples for each measurement
#define WHITE_BLACK_RATIO	0.3		// The ratio of readings on a black floor vs a white floor. white <= ratio * black.
#define SURVIVE_TIME		10		// How long the robot stays in survival mode after the "threat" has gone away.

// Global vars
int sensor[7]; 						// Sensor readings
int currState = STATE_SEEK;			// Current state of the robot
int stateTimer = -1;				// How long have we been in the current state? 
int currDirection = 0;				// Current direction of the engines
int currDirMotor1 = 0, currDirMotor2 = 0;	// Direction of each motor
int initialGroundReading[4];		// Initial readings from the ground sensors - this is our reference for "black"
int initialLeftEye = 0, initialRightEye = 0;
									// Initial readings for long distance sensors, to account for noise etc.

void doMove()
{
	int distanceLeft, distanceRight, distanceAvg;
	
	if(stateTimer >= 1000000)
		stateTimer = 0;
	stateTimer += 1;

#if 0	
	if(stateTimer % 50 == 0)
		if(stateTimer % 100 == 0)
			setMotors(DIR_FORWARD);
		else
			setMotors(DIR_BACK);
	delay_ms(DEBUG ? 1000 : 100);	
	return;
#endif

	readSensors();
	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;
	
	switch(currState) {
	case STATE_SEEK: // Turn right for a while, then turn left until you find an enemy.
		if(survivalCheck())
			break;
`		// If we just switched, start turning.
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
		//if(groundSensor(DIR_FORWARD | DIR_RIGHT) || groundSensor(DIR_FORWARD | DIR_LEFT))
		//{
		//	SWITCH_STATE(STATE_DESTROY_OVER);
		//}
		// No break: code shared with DESTROY_OVER
	case STATE_DESTROY_OVER:
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
		delay_ms(500);
	}

	LEDS = pow(2, 8-currState);
	if(DEBUG)
		printState();
	delay_ms(DEBUG ? 1000 : 100);
}

int survivalCheck()
{
	//return FALSE;
	if((groundSensor(DIR_FORWARD | DIR_LEFT) && groundSensor(DIR_FORWARD | DIR_RIGHT)) || pushSensor(DIR_BACK))
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

void initSensors()
{
	int i;

	// Init touch sensors
	TRISEbits.TRISE1 = INPUT;
	TRISEbits.TRISE2 = INPUT;
	TRISDbits.TRISD1 = INPUT;
	TRISDbits.TRISD2 = INPUT;

	// Get an initial ground sensor reading
	readSensors();
	for(i = 0; i < 4; i++)
		initialGroundReading[i] = sensor[2+i];
	printString("Initial black value: ");
	printInt(initialGroundReading);
	puts("");

	initialRightEye = sensor[0];
	initialLeftEye  = sensor[1];
	printString("Initial long distances sensors: ");
	printInt(initialLeftEye);
	printString(" ");
	printInt(initialRightEye);
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
}

void setMotors(int direction)
{
	// Assuming motor #1 is left, #2 is right.
	if(currDirection == direction)
		return; // Don't needlessly change the signal
	MOTOR1(1023);
	MOTOR2(1023);
	if(direction & DIR_FORWARD) 
	{
		if(direction & DIR_RIGHT)
		{
			MOTOR2(0);
		} else if(direction & DIR_LEFT) {
			MOTOR1(0);
		}	
	} else if(direction & DIR_BACK) {
		MOTOR1(-1023);
		MOTOR2(-1023);
		if(direction & DIR_RIGHT)
		{
			MOTOR1(0);
		} else if(direction & DIR_LEFT) {
			MOTOR2(0);
		}
	} else {
		// No real movement, may be turning in place
		if(direction & DIR_RIGHT)
		{
			MOTOR1(1023);
			MOTOR2(-1023);
		} else if(direction & DIR_LEFT) {
			MOTOR1(-1023);
			MOTOR2(1023);
		} else {
			MOTOR1(0);
			MOTOR2(0);
		}	
	}
	currDirection = direction;
}

int groundSensor(unsigned sensor_dir)
{
	if(sensor_dir & DIR_FORWARD) {
		//return sensor[3] < WHITE_BLACK_RATIO * initialGroundReading;
		if(sensor_dir & DIR_RIGHT)
			return sensor[2] < WHITE_BLACK_RATIO * initialGroundReading[0];
		else
			return sensor[3] < WHITE_BLACK_RATIO * initialGroundReading[1];
	} else {
		//return sensor[5] < WHITE_BLACK_RATIO * initialGroundReading;
		if(sensor_dir & DIR_LEFT)
			return sensor[4] < WHITE_BLACK_RATIO * initialGroundReading[2];
		else
			return sensor[5] < WHITE_BLACK_RATIO * initialGroundReading[3];
	}
}

int distanceSensor(unsigned sensor_dir)
{
	if(sensor_dir == DIR_RIGHT)
		return ((1024 + initialRightEye) - sensor[0])/30;
	else
		return ((1024 + initialLeftEye)  - sensor[1])/30;
}

int pushSensor(unsigned sensor_dir) {
	//PORT(D/E).R(D/E)(1/2)
	if(sensor_dir == DIR_FORWARD)
		return PORTDbits.RD1;
	else if(sensor_dir == DIR_RIGHT)
		return PORTDbits.RD2;
	else if(sensor_dir == DIR_BACK)
		return PORTEbits.RE1;
	else if(sensor_dir == DIR_BACK)
		return PORTEbits.RE2;
	else
		return FALSE;
}

void printState()
{
	int i;
	puts("-----------------------------------------------------");
	// Print out state
	printString("Robot has been ");
	switch(currState) {
	case STATE_SEEK:
		printString("SEEKING"); // (const rom far char *) ?
		break;
	case STATE_DESTROY:
		printString("DESTROY");
		break;
	case STATE_DESTROY_OVER:
		printString("DESTROYO");
		break;
	case STATE_SURVIVE:
		printString("SURVIVE");
		break;
	default:
		printString("INVALID");
	}
	printString(" for ");
	printInt(stateTimer);
	puts("");

	// Print out engine state
	printString("Current engine state: ");
	printChar(ENGINE_STATE(currDirMotor1));
	printChar(ENGINE_STATE(currDirMotor2));
	puts("");
	
	// Print ground sensor readings
	printString("Current ground sensors seeing white: ");
	if(groundSensor(DIR_FORWARD | DIR_RIGHT))
		printString("(FRONT RIGHT) ");
	if(groundSensor(DIR_BACK | DIR_RIGHT))
		printString("(REAR RIGHT) ");
	if(groundSensor(DIR_FORWARD | DIR_LEFT))
		printString("(FRONT LEFT) ");
	if(groundSensor(DIR_BACK | DIR_LEFT))
		printString("(REAR LEFT)");
	puts("");
	
	// Print push sensor readings
	printString("Current push sensors being pressed: ");
	if(pushSensor(DIR_FORWARD))
		printString("FRONT ");
	if(pushSensor(DIR_RIGHT))
		printString("RIGHT ");
	if(pushSensor(DIR_LEFT))
		printString("LEFT ");
	if(pushSensor(DIR_BACK))
		printString("REAR");
	puts("");	
	
	// Print long distance sensors
	printString("Distance sensors: ");
	printInt(distanceSensor(DIR_LEFT));
	printChar(' ');
	printInt(distanceSensor(DIR_RIGHT));
	puts("");
	printString("Raw sensor readings: ");
	for(i=0; i < 7; i++) {
		printString(" ");
		printInt(sensor[i]);
	}
	puts("");
	puts("-----------------------------------------------------");
}