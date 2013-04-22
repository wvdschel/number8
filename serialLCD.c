/**
 * LCD Display
 * part of Dwengo library
 * 
 * Control of LCD display on the Dwengo board
 *
 * Version: 1.0.$Revision: 1567 $
 * Date: $Date: 2009-12-21 21:38:03 +0100 (ma, 21 dec 2009) $
 * (c) Dwengo vzw - http://www.dwengo.org
 */

#include "sumolib/dwengoLCD.h"
#include "serialDebug.h"
#include <stdlib.h>

void initLCD(void) {
	initDebug();
}

void clearLCD(void) {
	//puts("");
}

void commandLCD(const BYTE c) {}


// set cursor at position p of line l
void setCursorLCD(BYTE l, BYTE p) {}


void appendCharToLCD(const char c) {
	printChar(c);
}


void printCharToLCD(const char c, BYTE l, BYTE p) {
	printChar(c);
}

void appendStringToLCD_(const far rom char* message) {
	puts(message);
}

void printStringToLCD(char* message, BYTE l, BYTE p) {
	puts(message);
}

void appendStringToLCDcharptr(char* message) {
  puts(message);
}

void appendIntToLCD(int i) {
  printInt(i);
}

void printIntToLCD(int i, BYTE l, BYTE p) {
	printInt(i);
}

