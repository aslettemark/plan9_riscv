#include "u.h"
#include <tos.h>
#include "ureg.h"

#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "../port/systab.h"

typedef u32int u32;

void syscall(Ureg *ureg) {
	m->syscall++;
	up->insyscall = 1;
	up->pc = ureg->pc;
	spllo();
	uintptr sp = ureg->sp;
	Sargs *sargs = (Sargs *) (sp + BY2WD);

	u32 syscallnr = ureg->r8;
	syscallfmt(syscallnr, ureg->pc, (va_list)(sargs));
	print("syscallfmt: %s\n", up->syscalltrace);

	u32 ret = -1;
	char *e = nil;
	if(!waserror()){
		if(syscallnr >= nsyscall){
			pprint("bad sys call number %d pc %#p\n", syscallnr, ureg->pc);
			postnote(up, 1, "sys: bad sys call", NDebug);
			error(Ebadarg);
		}

		if(sp < (USTKTOP-BY2PG) || sp > (USTKTOP-sizeof(Sargs)-BY2WD)) {
			validaddr(sp, sizeof(Sargs)+BY2WD, 0);
		}
		up->s = *(sargs);
		up->psstate = sysctab[syscallnr];

		ret = systab[syscallnr](up->s.args);
		poperror();
	} else {
		/* failure: save the error buffer for errstr */
		e = up->syserrstr;
		up->syserrstr = up->errstr;
		up->errstr = e;
	}

	up->insyscall = 0;
	splhi();

	ureg->r8 = ret;

	// Skip over ECALL instruction
	ureg->pc += 4;
}

void
forkchild(Proc *p, Ureg *ureg)
{
	print("TODO forkchild: ");
	spin();
}

/*
 * Set up ureg so that we start executing correctly when "returning" from this exec syscall
*/
long execregs(ulong entry, ulong ssize, ulong nargs) {
	Ureg *ureg = (Ureg *) up->kstack;

	print("execregs %#p %#p %#p ureg=%#p\n", entry, ssize, nargs, ureg);

	ulong *sp = (ulong*)(USTKTOP - ssize);
	*--sp = nargs;

	ureg->pc = entry;
	ureg->sp = PTR2UINT(sp);
	up->fpstate = FPinit;

	return USTKTOP - sizeof(Tos);
}
