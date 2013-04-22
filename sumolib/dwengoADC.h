/**
 * Sensors
 * part of Dwengo library
 * 
 * Analog/Digital conversion, AN0-4
 *
 * Version: 1.0.$Revision: 1618 $
 * Date: $Date: 2010-01-04 22:19:02 +0100 (ma, 04 jan 2010) $
 * (c) Dwengo vzw - http://www.dwengo.org
 */

#ifndef DWENGO_ADC_H
#define DWENGO_ADC_H

#include "sumolib/dwengoBoard.h"

// Functions
void initADC(void);   // Call this function to initialise the analog inputs of the board
int readADC(BYTE address);   // Returns the analog value on channel <address>

#endif // DWENGO_ADC_H
