#define NPRIVATES	16

TEXT	_main(SB), 1, $(4*4 + NPRIVATES*4)

	MOVW	$setSB(SB), R3
	MOVW	R8, _tos(SB)

	MOVW	$p-64(SP), R9
	MOVW	R9, _privates(SB)
	MOVW	$NPRIVATES, R9
	MOVW	R9, _nprivates(SB)

	MOVW	inargc-4(FP), R8
	MOVW	$inargv+0(FP), R10
	MOVW	R8, 4(R2)
	MOVW	R10, 8(R2)
	JAL	R1, main(SB)
loop:
	MOVW	$_exitstr<>(SB), R8
	MOVW	R8, 4(R2)
	JAL	R1, exits(SB)
	JMP	loop

DATA	_exitstr<>+0(SB)/4, $"main"
GLOBL	_exitstr<>+0(SB), $5
