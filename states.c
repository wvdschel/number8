#include "states.h"
#include "aiLib.h"
#include "serialDebug.h"


#define DIFF_THRESHOLD 		(int)70 	// Maximum difference between two long range sensors, if the diff is higher, the robot will reallign.
#define DISTANCE_CLOSE		(int)985	// Anything lower than this value is considered close enough to attack, anything higher is considered too far away
#define DISTANCE_VERY_CLOSE	(int)600	// This is close enough to ignore the white lines

#define SEEK_ROTATE_TIME 	(int)150 	// How many iterations should we wait before moving the robot to change the search space.
							 			// To disable this and just turn left all the time, set to 0.
#define SEEK_MOVE_DISTANCE	(int)50		// How long the robot should move forward before looking around again.
#define MIN_PROGRESS		(int)5		// Minimal progress to continue pushing during an attack
#define SURVIVE_CLEARANCE	(int)30		// Minimal progress before leaving survival

#define FLANK_ROTATE_TIME	(int)100
#define FLANK_BACK_DISTANCE	(int)-20

#define MAX_SPEED			(int)1023
#define SAFE_SPEED			(int)950
#define SCAN_SPEED			(int)800

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
		setMotors(DIR_FORWARD, MAX_SPEED);
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
		setMotors(DIR_FORWARD, SAFE_SPEED);
	} else {
		setMotors(DIR_FORWARD, MAX_SPEED);
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

void doAttackState()
{
	int distanceLeft, distanceRight, distanceAvg;

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	if(distanceAvg > DISTANCE_VERY_CLOSE && survivalCheck())
	{
		return;
	}

#ifdef PROGRESS_WHEEL
	if(stateTimer == 1) // Reset counter if we just started pushing
	{
		wipeProgressHistory();
	}
	// Once we have filled up the history, and our combined progress is insufficient, consider flanking
	if(progress < MIN_PROGRESS && stateTimer > PROGRESS_HISTORY_SIZE)
	{
		SWITCH_STATE(STATE_FLANK_TURN);
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
	int distanceLeft, distanceRight, distanceAvg;
	char frontLeft = groundSensor(DIR_FORWARD | DIR_LEFT), frontRight = groundSensor(DIR_FORWARD | DIR_RIGHT),
			rearLeft = groundSensor(DIR_BACK | DIR_LEFT), rearRight = groundSensor(DIR_BACK | DIR_RIGHT);

	distanceLeft = distanceSensor(DIR_LEFT);
	distanceRight = distanceSensor(DIR_RIGHT);
	distanceAvg = (distanceLeft + distanceRight) / 2;

	// First, check if we still see white. Getting away from white is our first priority
	if(frontLeft && frontRight)
	{
		// Full frontal white line, drive back full speed
		stateProgress = 0;
		setMotors(DIR_BACK, MAX_SPEED);
		return;
	} else if(rearRight && rearLeft) {
		// White line underneath the entire back end, drive forward full speed
		stateProgress = 0;
		setMotors(DIR_FORWARD, MAX_SPEED);
		return;
	} else if(rearLeft) {
		// White line under the back left (and possibly the front left): drive away turning to the right
		stateProgress = 0;
		setMotors(DIR_FORWARD | DIR_RIGHT, MAX_SPEED);
		return;
	} else if(rearRight) {
		// White line under the back right (and possibly the front right): drive away turning to the left
		stateProgress = 0;
		setMotors(DIR_FORWARD | DIR_LEFT, MAX_SPEED);
		return;
	} else if(frontRight) {
		// White line under the front right: drive away in reverse turning to the left
		stateProgress = 0;
		setMotors(DIR_BACK | DIR_LEFT, MAX_SPEED);
		return;
	} else if(frontLeft) {
		// White line under the front left: drive away in reverse turning to the right
		stateProgress = 0;
		setMotors(DIR_BACK | DIR_RIGHT, MAX_SPEED);
		return;
	} else {
		// No more white line: stop turning and speed away
		int direction = (currDirection & DIR_FORWARD) ? DIR_FORWARD : DIR_BACK;
		setMotors(direction, MAX_SPEED);
	}

	// Next, check if we see an enemy. If an enemy is visible while we're putting some
	// distance between the edge and our robot, we still want to engage.
	if(enemyCheck(distanceLeft, distanceRight))
		return;
	
	// If no white and no enemy is present, and we've cleared enough distance, go back to
	// looking for the enemy.
	if(stateProgress >= SURVIVE_CLEARANCE)
	{
		SWITCH_STATE(STATE_SCAN);
	}
}

void doFlankAwayState()
{
	setMotors(DIR_BACK | DIR_LEFT, MAX_SPEED);
	if(stateProgress <= FLANK_BACK_DISTANCE)
		SWITCH_STATE(STATE_SCAN);
}

void doFlankTurnState()
{
	setMotors(DIR_BACK | DIR_LEFT, MAX_SPEED);
}

void doAttackRearState()
{
	//setMotors(DIR_BACK, MAX_SPEED);
	setMotors(DIR_BACK, MAX_SPEED);

	if(!pushSensor(DIR_BACK))
		SWITCH_STATE(STATE_SCAN);
}