/**
 * Motor driver
 * part of Dwengo library
 * 
 * Control of motor driver on the Dwengo board
 *
 * Version: 1.0.$Revision: 1618 $
 * Date: $Date: 2010-01-04 22:19:02 +0100 (ma, 04 jan 2010) $
 * (c) Dwengo vzw - http://www.dwengo.org
 */

#ifndef DWENGO_MOTOR_H
#define DWENGO_MOTOR_H

// Definitions
#define FORWARD     1
#define BACKWARD    0

// Functions
void initMotors(void);

// Attention: speed has to be in the range [-1023,1023]
void setSpeedMotor1(short speed);
void setSpeedMotor2(short speed);

void stopMotors(void);


#endif // DWENGO_MOTOR_H