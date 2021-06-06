/*
 * This is the same as the C programme:
 *
 *	void
 *	main(void)
 *	{
 *		startboot();
 *	}
 *
 * It is in assembler because SB needs to be
 * set and doing this in C drags in too many
 * other routines.
 */


TEXT _main(SB), $-4
	// set static base
	MOVW $setSB(SB), R3

	// call startboot
	JAL R1, startboot(SB)
	RET
