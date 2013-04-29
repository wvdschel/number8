#ifndef AI_LIB_H
#define AI_LIB_H

#include "sumolib/dwengoADC.h"

#define DIR_BACK 	1
#define DIR_FORWARD 2
#define DIR_LEFT 	4
#define DIR_RIGHT 	8

#define PROGRESS_HISTORY_SIZE	((int)15)
// State switching macro
#define SWITCH_STATE(nState)	{					\
		printInt(__LINE__);							\
		printString(": Switching to new state ");	\
		printInt(nState);							\
		puts("");									\
		currState = nState;							\
		stateTimer = 0;								\
		stateProgress = 0;							\
	}

// Global vars
#ifndef AI_LIB
extern int* sensor; 				// Sensor readings
extern int currState;				// Current state of the robot
extern int stateTimer;				// How long have we been in the current state?
extern int stateProgress;			// How much distance have we covered in this state?
extern int currDirection;			// Current direction of the engines
extern int currSpeedMotorLeft;		// Direction of each motor
extern int currSpeedMotorRight;
extern int* initialGroundReading;	// Initial readings from the ground sensors - this is our reference for "black"
extern int initialEyeLeft;			// Initial readings for long distance sensors, to account for noise etc.
extern int initialEyeRight;
extern int initialEyeAvg;
extern int progress;				// Progress over the past PROGRESS_HISTORY_SIZE turns combined.
extern int* progressHistory;		// Circular buffer of some turns raw input from the progress wheel.
extern int progressHistoryIndex;	// Current end of the circular buffer.
extern int progressRaw;				// Progress as reported by the progress wheel. This value is reset each turn.
#endif

void ailib_init(void);
void ailib_isr(void);

void initState(int direction);
void initSensors(void);
void readSensors(void);
void doMove(void);
void setMotors(int direction, int speed);
int groundSensor(unsigned sensor);
int distanceSensor(unsigned sensor);
int pushSensor(unsigned sensor);
void printState(void);
int survivalCheck(void);
void wipeProgressHistory(void);

#endif