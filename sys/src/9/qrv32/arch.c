#include "u.h"
#include "ureg.h"

#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

void setkernur(Ureg* ureg, Proc* p) {
	// TODO
}

void setregisters(Ureg* ureg, char* pureg, char* uva, int n) {
	// TODO
}

void dumpstack(void) {
	// TODO
}

/*
 *  Return the userpc the last trap happened at
 *  Slightly inaccurate if last trap was a syscall and we skipped ECALL
 */
uintptr userpc(void)
{
	Ureg *ureg = up->dbgreg;
	return ureg->pc;
}

long _xdec(long *p) {
	int s, v;

	s = splhi();
	v = --*p;
	splx(s);
	return v;
}

void _xinc(long *p) {
	int s;

	s = splhi();
	++*p;
	splx(s);
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void callwithureg(void (*fn)(Ureg*)) {
	Ureg ureg;

	ureg.pc = getcallerpc(&fn);
	ureg.sp = PTR2UINT(&fn);
	fn(&ureg);
}

/*
 *  pc output by dumpaproc
 */
uintptr dbgpc(Proc* p) {
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}

/*
 *  this is the body for all kproc's
 */
static void linkproc(void) {
	spllo();
	print("entry kproc pid %d\n", up->pid);
	up->kpfun(up->kparg);
	print("exit kproc pid %s\n", up->pid);
	pexit("kproc exiting", 0);
}

/*
 *  setup stack and initial PC for a new kernel proc.  This is architecture
 *  dependent because of the starting stack location
 */
void kprocchild(Proc *p, void (*func)(void*), void *arg) {
	p->sched.pc = PTR2UINT(linkproc);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-4);

	p->kpfun = func;
	p->kparg = arg;
}

/*
 * called in syscallfmt.c, sysfile.c, sysproc.c
 */
void validalign(uintptr addr, unsigned align)
{
	/*
	 * Plan 9 is a 32-bit O/S, and the hardware it runs on
	 * does not usually have instructions which move 64-bit
	 * quantities directly, synthesizing the operations
	 * with 32-bit move instructions. Therefore, the compiler
	 * (and hardware) usually only enforce 32-bit alignment,
	 * if at all.
	 *
	 * Take this out if the architecture warrants it.
	 */
	if(align == sizeof(vlong))
		align = sizeof(long);

	/*
	 * Check align is a power of 2, then addr alignment.
	 */
	if((align != 0 && !(align & (align-1))) && !(addr & (align-1)))
		return;
	postnote(up, 1, "sys: odd address", NDebug);
	error(Ebadarg);
	/*NOTREACHED*/
}

int cas32(void* addr, u32int old, u32int new) {
	int r, s;

	s = splhi();
	if(r = (*(u32int*) addr == old))
		*(u32int*)addr = new;
	splx(s);
	if (r)
		coherence();
	return r;
}

int cmpswap(long *addr, long old, long new) {
	return cas32(addr, old, new);
}

void archreboot(void) {
	// TODO
	for(;;)
		;
}

void procsetup(Proc *p) {
	p->fpstate = FPinit;
	//fpoff();
}
