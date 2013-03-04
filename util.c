#include "util.h"
#include "serialDebug.h"

int pow(int num, int exp) {
	int result = 1;
	while(exp > 0) {
		result *= num;
		exp--;
	}
	return result;
}

int mod(int num, int base) {
	printInt(num);
	printString(" % ");
	printInt(base);
	printString(" = ");
	while(num > base)
		num -= base;
	printInt(num);
	puts("");
	return num;
}