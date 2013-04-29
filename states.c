#include "states.h"
#include "aiLib.h"
#include "serialDebug.h"


#define DIFF_THRESHOLD 		100			// Maximum difference between two long range sensors, if the diff is higher, the robot will reallign.
#define DISTANCE_CLOSE		800			// Anything lower than this value is considered close enough to attack, anything higher is considered too far away
#define SEEK_ROTATE_TIME 	(int)150 	// How many iterations should we wait before moving the robot to change the search space.
							 			// To disable this and just turn left all the time, set to 0.
#define SEEK_MOVE_DISTANCE	(int)100	// How long the robot should move forward before looking around again.
#define MIN_PROGRESS		5			// Minimal progress to continue pushing.
#define SURVIVE_CLEARANCE	100			// Minimal progress before leaving survival

#define MAX_SPEED			1023
#define SAFE_SPEED			850

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
		SWITCH_STATE(STATE_ATTACK);
		return TRUE;
	}
	// If our rear is pressed, there's probably an enemy in the back
	if(pushSensor(DIR_BACK))
	{
		SWITCH_STATE(STATE_ATTACK_REAR);
		setMotors(0, 0);
		return TRUE;
	}
	return FALSE;
}

unsigned char edgeDetectedCount = 0;
void doMoveState()
{
	int distanceLeft, distanceRight, distanceAvg;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	if(survivalCheck() || enemyCheck(distanceLeft, distanceRight))
	{
		return;
	}

	// Reset our edge-vision counter when we start moving
	if(stateTimer == 0)
		edgeDetectedCount = 0;
	// If we (probably) see a ledge, increment the counter, if not, reset the counter
	if(distanceLeft - initialEyeLeft >= 20)
	{
		edgeDetectedCount++;
	} else {
		edgeDetectedCount = 0;
	}
	// If we've seen a ledge for 3 turns, reduce speed, if not, ram speed!
	if(edgeDetectedCount >= 3)
	{
		setMotors(DIR_FORWARD, SAFE_SPEED);
	} else {
		setMotors(DIR_FORWARD, MAX_SPEED);
	}
	// If we 
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

	if(survivalCheck() || enemyCheck(distanceLeft, distanceRight))
	{
		return;
	}

	setMotors(DIR_LEFT, MAX_SPEED);

	if(stateTimer >= SEEK_ROTATE_TIME)
	{
		SWITCH_STATE(STATE_MOVE);
		return;
	}
}

void doAttackState()
{
	int distanceLeft, distanceRight, distanceAvg;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	if(survivalCheck() || enemyCheck(distanceLeft, distanceRight))
	{
		return;
	}

#ifdef PROGRESS_WHEEL
	if(stateTimer == 0) // Reset counter if we just started pushing
	{
		wipeProgressHistory();
	}
	// Once we have filled up the history, and our combined progress is insufficient, consider flanking
	if(progress < MIN_PROGRESS && stateTimer > PROGRESS_HISTORY_SIZE)
	{
		SWITCH_STATE(STATE_FLANK_AWAY);
		return
	}
#endif
	if(distanceLeft > DISTANCE_CLOSE && distanceRight > DISTANCE_CLOSE)
	{
		// Target lost, go back to searching
		SWITCH_STATE(STATE_SCAN);
		return;
	}
	if(ABS(distanceLeft - distanceRight) > DIFF_THRESHOLD)
	{
		// Sensors are appart, reallign.
		if(distanceLeft > distanceRight)
			setMotors(DIR_FORWARD | DIR_RIGHT, MAX_SPEED);
		else
			setMotors(DIR_FORWARD | DIR_LEFT, MAX_SPEED);
	} else // Target straight ahead
		setMotors(DIR_FORWARD, MAX_SPEED);
	return;
}

void doSurviveState()
{
	// First, check if we still see white. Getting away from white is our first priority
	/* TODO */
	// Next, check if we see an enemy. If an enemy is visible while we're putting some
	// distance between the edge and our robot, we still want to engage.
	/* TODO */
	// If no white and no enemy is present, and we've cleared enough distance, go back to
	// looking for the enemy.
	if(stateProgress >= SURVIVE_CLEARANCE)
	{
		SWITCH_STATE(STATE_SCAN);
	}
}

void doFlankAwayState()
{}

void doFlankTurnState()
{}

void doFlankForwardState()
{}

void doFlankScanState()
{}

void doAttackRearState()
{
	setMotors(DIR_BACK, MAX_SPEED);
}