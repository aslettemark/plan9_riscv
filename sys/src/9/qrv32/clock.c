#include "u.h"

uvlong fake_clock = 1;

uvlong fastticks(uvlong *hz)
{
	fake_clock++; // TODO
	if(hz)
		*hz = 500000000;
	return fake_clock;
}

void microdelay(int n)
{
	// TODO
}

// BCM
void delay(int n)
{
	while(--n >= 0)
		microdelay(1000);
}

ulong perfticks(void) {
	return 1; // TODO
}

void timerset(uvlong next) {
	// TODO
}

ulong µs(void) {
	// TODO
	return fastticks2us(fastticks(nil));
}

void cycles(uvlong *cycp) {
	// TODO
	//print("cycles: %#p\n", cycp);
	*cycp = fastticks(nil);
}
