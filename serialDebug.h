#ifndef SERIAL_DEBUG_H
#define SERIAL_DEBUG_H

// If debugging, serial will be used for printing debug info, otherwise motors will be controlled over serial
#define DEBUG 1
#define USE_MOTORS 1

#define ABS(x) ((x) < 0 ? (-(x)) : (x))

void initDebug(void);

#define puts(x)	puts_((char*)x)
#define printString(x)	printString_((char*)x)

void printChar(char c);
void printString_(char* message);
void printInt(int number);
void puts_(char* message);

#endif