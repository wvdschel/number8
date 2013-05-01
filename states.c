#include "states.h"
#include "aiLib.h"
#include "serialDebug.h"

#define DISTANCE_CLOSE		(int)900	// Anything lower than this value is considered close enough to attack, anything higher is considered too far away
#define DISTANCE_VERY_CLOSE	(int)780	// This is close enough to ignore the white lines

#define SEEK_ROTATE_TIME 	(int)80 	// How many iterations should we wait before moving the robot to change the search space.
							 			// To disable this and just turn left all the time, set to 0.
#define SEEK_MOVE_DISTANCE	(int)50		// How long the robot should move forward before looking around again.
#define MIN_PROGRESS		(int)2		// Minimal progress to continue pushing during an attack
#define SURVIVE_CLEARANCE	(int)30		// Minimal progress before leaving survival

#define FLANK_ROTATE_TIME	(int)30
#define FLANK_BACK_DISTANCE	(int)-4
#define FLANK_REAR_DISTANCE	(int)18
#define FLANK_REAR_TIME		(int)25

#define MAX_SPEED			(int)1023
#define SAFE_SPEED			(int)800
#define VERY_SAFE_SPEED		(int)650
//#define SAFE_SPEED			(int)0
#define SCAN_SPEED			(int)800
//#define SCAN_SPEED			(int)0

// Check if we see a white line, and switch to survival state if we do.
int survivalCheck()
{
	if(groundSensor(DIR_FORWARD | DIR_LEFT) || groundSensor(DIR_FORWARD | DIR_RIGHT)
		|| groundSensor(DIR_BACK | DIR_LEFT) || groundSensor(DIR_BACK | DIR_RIGHT))
	{
		SWITCH_STATE(STATE_SURVIVE);
		return TRUE;
	}
	return FALSE;
}

// Check if we detect an enemy (front or rear), and switch state accordingly
int enemyCheck(int distanceLeft, int distanceRight)
{
	// If something is close, assume there's an enemy in front
	if(distanceLeft < DISTANCE_CLOSE || distanceRight < DISTANCE_CLOSE)
	{
		if(currState != STATE_ATTACK)
			SWITCH_STATE(STATE_ATTACK);
		return TRUE;
	}
	// If our rear is pressed, there's probably an enemy in the back
	if(pushSensor(DIR_BACK))
	{
		if(currState != STATE_ATTACK_REAR)
			SWITCH_STATE(STATE_ATTACK_REAR);
		setMotors(0, 0);
		return TRUE;
	}
	return FALSE;
}

static int edgeDetectedCount = 0;
void doMoveState()
{
	int distanceLeft, distanceRight, distanceAvg;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	if(enemyCheck(distanceLeft, distanceRight) || survivalCheck())
	{
		return;
	}

	// Reset our edge-vision counter when we start moving
	if(stateTimer == 1)
	{
		edgeDetectedCount = 0;
		//setMotors(DIR_FORWARD, MAX_SPEED);
		setMotors(DIR_FORWARD, SAFE_SPEED);
	}
	// If we (probably) see a ledge, increment the counter, if not, reset the counter
	if(distanceLeft >= 1044)
	{
		edgeDetectedCount++;
	} else {
		edgeDetectedCount = 0;
	}
	// If we (think we) see the edge, slow down.
	if(edgeDetectedCount >= 3)
	{
		setMotors(DIR_FORWARD, VERY_SAFE_SPEED);
	} else {
		//setMotors(DIR_FORWARD, MAX_SPEED);
		setMotors(DIR_FORWARD, SAFE_SPEED);
	}
	// If we've covered enough distance, then go back to scanning
	if(stateProgress >= SEEK_MOVE_DISTANCE)
	{
		SWITCH_STATE(STATE_SCAN);
		return;
	}
}

void doScanState()
{
	int distanceLeft, distanceRight, distanceAvg;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	if(enemyCheck(distanceLeft, distanceRight) || survivalCheck())
	{
		return;
	}

	setMotors(DIR_LEFT, SCAN_SPEED);

	if(stateTimer >= SEEK_ROTATE_TIME)
	{
		SWITCH_STATE(STATE_MOVE);
		return;
	}
}

static int straightAheadTime = 0;
void doAttackState()
{
	int distanceLeft, distanceRight, distanceAvg;
	if(stateTimer == 1)
		straightAheadTime = 0;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

#ifdef PROGRESS_WHEEL
	// Once we have filled up the history, and our combined progress is insufficient, consider flanking
	if(progress < MIN_PROGRESS && straightAheadTime > PROGRESS_HISTORY_SIZE)
	{
		SWITCH_STATE(STATE_FLANK_TURN);
		return;
	}
#endif
	if(distanceLeft > DISTANCE_CLOSE && distanceRight > DISTANCE_CLOSE)
	{
		// Target lost, go back to searching
		SWITCH_STATE(STATE_SCAN);
		return;
	}
	if(distanceLeft > DISTANCE_CLOSE || distanceRight > DISTANCE_CLOSE)
	{
		straightAheadTime = 0;
		// Sensors are appart, reallign.
		if(distanceLeft > distanceRight)
			setMotors(DIR_FORWARD | DIR_RIGHT, MAX_SPEED);
		else
			setMotors(DIR_FORWARD | DIR_LEFT, MAX_SPEED);
	}
	else if(distanceAvg > DISTANCE_VERY_CLOSE)
	{
		// Target straight ahead but not very close
		if(survivalCheck())
			return;
		straightAheadTime++;
		setMotors(DIR_FORWARD, SAFE_SPEED);
	} 
	else
	{
		// Target straight ahead and very close
		straightAheadTime++;
		setMotors(DIR_FORWARD, MAX_SPEED);
	}
	return;
}

void doSurviveState()
{
	int distanceLeft, distanceRight, distanceAvg;
	char frontLeft = groundSensor(DIR_FORWARD | DIR_LEFT), frontRight = groundSensor(DIR_FORWARD | DIR_RIGHT),
			rearLeft = groundSensor(DIR_BACK | DIR_LEFT), rearRight = groundSensor(DIR_BACK | DIR_RIGHT);

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	// First, check if we still see white. Getting away from white is our first priority
	if(frontLeft && frontRight)
	{
		//puts("Full frontal white line, drive back full speed");
		stateProgress = 0;
		setMotors(DIR_BACK, MAX_SPEED);
		return;
	} else if(rearRight && rearLeft) {
		//puts("White line underneath the entire back end, drive forward full speed");
		stateProgress = 0;
		setMotors(DIR_FORWARD, MAX_SPEED);
		return;
	} else if(rearLeft) {
		//puts("White line under the back left (and possibly the front left): drive away turning to the right");
		stateProgress = 0;
		setMotors(DIR_FORWARD | DIR_RIGHT, MAX_SPEED);
		return;
	} else if(rearRight) {
		//puts("White line under the back right (and possibly the front right): drive away turning to the left");
		stateProgress = 0;
		setMotors(DIR_FORWARD | DIR_LEFT, MAX_SPEED);
		return;
	} else if(frontRight) {
		//puts("White line under the front right: drive away in reverse turning to the left");
		stateProgress = 0;
		setMotors(DIR_BACK | DIR_LEFT, MAX_SPEED);
		return;
	} else if(frontLeft) {
		//puts("White line under the front left: drive away in reverse turning to the right");
		stateProgress = 0;
		setMotors(DIR_BACK | DIR_RIGHT, MAX_SPEED);
		return;
	} else {
		int direction = (currDirection & DIR_FORWARD) ? DIR_FORWARD : DIR_BACK;
		//printString("No more white line: stop turning and speed away ");
		printInt(direction);
		puts("");
		setMotors(direction, SAFE_SPEED);
	}

	// Next, check if we see an enemy. If an enemy is visible while we're putting some
	// distance between the edge and our robot, we still want to engage.
	if(enemyCheck(distanceLeft, distanceRight))
		return;
	
	// If no white and no enemy is present, and we've cleared enough distance, go back to
	// looking for the enemy.
	if(ABS(stateProgress) >= SURVIVE_CLEARANCE)
	{
		SWITCH_STATE(STATE_SCAN);
	}
}

void doFlankAwayState()
{
	setMotors(DIR_BACK, MAX_SPEED);
	if(stateProgress <= FLANK_BACK_DISTANCE)
		SWITCH_STATE(STATE_SCAN);
	survivalCheck();
}

void doFlankTurnState()
{
	setMotors(DIR_BACK | DIR_LEFT, MAX_SPEED);
	if(stateTimer > FLANK_ROTATE_TIME)
		SWITCH_STATE(STATE_FLANK_AWAY);
	survivalCheck();
}

void doAttackRearState()
{
	//setMotors(DIR_BACK, 0);
	//return;

	setMotors(DIR_BACK, MAX_SPEED);
	
	if(!pushSensor(DIR_BACK))
	{
		SWITCH_STATE(STATE_SCAN);
		return;
	}

#ifdef PROGRESS_WHEEL
	// Once we have filled up the history, and our combined progress is insufficient, consider flanking
	if(progress > -MIN_PROGRESS && stateTimer > PROGRESS_HISTORY_SIZE)
	{
		SWITCH_STATE(STATE_REAR_FLANK);
		return;
	}
#endif
}

void doRearFlankState()
{
	if(survivalCheck())
		return;
	setMotors(DIR_FORWARD | DIR_LEFT, MAX_SPEED);
	if(stateProgress >= FLANK_REAR_DISTANCE)
		SWITCH_STATE(STATE_SCAN);
}