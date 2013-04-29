#ifndef STATES_H
#define STATES_H

// Possible states for the robot
#define STATE_MOVE		 	1			// Looking for the enemy, moving to a new position
#define STATE_SCAN	 		2			// Looking for the enemy, looking around
#define STATE_ATTACK		3 			// Enemy visible in front (infrared)
#define STATE_SURVIVE	 	4			// White edge detected, move away
// Flanking states
#define STATE_FLANK_AWAY	5			// Back away from the enemy
#define STATE_FLANK_TURN	6 			// Turn away from the enemy
#define STATE_ATTACK_REAR	7			// Enemy is detected in our rear (button)

void doMoveState(void);
void doScanState(void);
void doAttackState(void);
void doSurviveState(void);
void doFlankAwayState(void);
void doFlankTurnState(void);
void doAttackRearState(void);

#endif
