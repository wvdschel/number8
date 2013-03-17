#ifndef AI_LIB_H
#define AI_LIB_H

#include "dwengoADC.h"

#define DIR_BACK 	1
#define DIR_FORWARD 2
#define DIR_LEFT 	4
#define DIR_RIGHT 	8

void ailib_init(void);
void ailib_isr(void);

void initState(int direction);
void initSensors(void);
void readSensors(void);
void doMove(void);
void setMotors(int direction);
int groundSensor(unsigned sensor);
int distanceSensor(unsigned sensor);
int pushSensor(unsigned sensor);
void printState(void);
int survivalCheck(void);

#endif