#include "u.h"
#include "ureg.h"
#include "tos.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "qrv32_init.h"

typedef u32int u32;

uintptr kseg0 = KZERO;

Proc *up = 0;
Conf conf;
Mach *m = (Mach*) MACHADDR;

uintptr user_sp = 0;

/* store plan9.ini contents here at least until we stash them in #ec */
char confname[MAXCONF][KNAMELEN];
char confval[MAXCONF][MAXCONFLINE];
int nconf;

void spin() {
	// print segs
	print("pid %d segments\n", up->pid);
	for (int i = 0; i < NSEG; i++) {
		if (up->seg[i]) {
			print("\t%d: %#p to %#p\n", i, up->seg[i]->base, up->seg[i]->top);
		} else {
			print("\t%d: nil\n", i);
		}
	}
	print("mach nsyscalls %d\n", m->syscall);
	print("Spinning\n");
	while (1) {}
}

static void shutdown(int ispanic) {
	// TODO
	spin();
}

/*
 *  exit kernel either on a panic or user request
 */
void exit(int code)
{
	shutdown(code);
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and entry point `entry'.
 */
void reboot(void *entry, void *code, ulong size) {
	// TODO
}

void halt() {
	print("halt: ");
	spin();
}

void idlehands(void) {
	print("idlehands: ");
	spin();
}

void machinit(void) {
	m->machno = 0;

	m->ticks = 1;
	m->perf.period = 1;
	m->cyclefreq = 500000000; // guess 500 MHz for qemu

	conf.nmach = 1;

	active.machs = 1;
	active.exiting = 0;

	up = nil;
}

void spl_test() {
	print("\nShould be equal:\n");
	crude_bin_print(islo());
	int i = spllo();
	crude_bin_print(i);

	print("\nShould be equal:\n");
	crude_bin_print(islo());
	i = splhi();
	crude_bin_print(i);

	print("\nShould be equal:\n");
	crude_bin_print(islo());
	i = islo();
	splx(i);
	crude_bin_print(i);
	crude_bin_print(islo());

	spllo();

	print("\nShould be equal:\n");
	crude_bin_print(islo());
	i = islo();
	splx(i);
	crude_bin_print(i);
	crude_bin_print(islo());

	splhi();
}

void print_mem_info() {
	void _start();
	void main();

	print("------------------------\n");

	print("etext: 0x%p\n", etext);
	//print("bdata: 0x%p\n", bdata);
	print("edata: %#p\n", edata);
	print("end:   0x%p\n", end);
	print("_start:0x%p\n", _start);
	print("main:  0x%p\n", main);
	print("wfi:   0x%p\n", wfi);
	print("stvec_asm:  0x%p\n", stvec_asm);
	print("KSTACK_LOW_END:   0x%p\n", KSTACK_LOW_END);
	print("KSTKSIZE: 0x%p\n", KSTKSIZE);
	print("MACHADDR: 0x%p\n", m);
	print("sizeof(Ureg) = %d\n", sizeof(Ureg));
	print("sizeof(Mach) = %d\n", sizeof(Mach));
	print("write_satp: 0x%p\n", write_satp);

	print("------------------------\n");
}

void test_xalloc() {
	int *mem = xalloc(8);
	print("xalloc 1: ");
	crude_hex_print(PTR2UINT(mem));

	print("xalloc 2: ");
	crude_hex_print(PTR2UINT(xalloc(8)));
	*mem = 1;
}

void init0() {
	int i;
	char buf[2*KNAMELEN];

	up->nerrlab = 0;
	coherence();
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	chandevinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", "RISC-V", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "riscv", 0);
		//ksetenv("nobootprompt", "tcp", 0); // Remove to be prompted for boot method
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		
		
		/* convert plan9.ini variables to #e and #ec */
		for(i = 0; i < nconf; i++) {
			ksetenv(confname[i], confval[i], 0);
			ksetenv(confname[i], confval[i], 1);
		}
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	touser(user_sp);
	assert(0);			/* shouldn't have returned */
}

static uintptr stackspace(uintptr base) {
	print("stackspace base %#p\n", base);

	// Space for syscall args, Tos struct and return PC
	u32 ustk_headroom = sizeof(Sargs) + sizeof(Tos) + sizeof(uintptr);
	print("ustk_headroom = %#p\n", ustk_headroom);

	// Canary value for debugging stack
	for (int j = 0; j < (BY2PG - ustk_headroom); j += 4) {
		u32 *a = UINT2PTR(base + j);
		*a = 0xdeadbeef;
	}

	uintptr sp = USTKTOP - ustk_headroom - 16;

	print("stackspace sp = %#p\n", sp);
	return sp;
}

/*
 *  create the first process
 */
void userinit(void) {
	/* no processes yet */
	up = nil;

	Proc *proc = newproc();
	proc->pgrp = newpgrp();
	proc->egrp = smalloc(sizeof(Egrp));
	proc->egrp->ref = 1;
	proc->fgrp = dupfgrp(nil);
	proc->rgrp = newrgrp();
	proc->procmode = 0640;

	kstrdup(&eve, "");
	kstrdup(&proc->text, "*init*");
	kstrdup(&proc->user, eve);

	/*
	 * Kernel Stack
	 */
	proc->sched.pc = PTR2UINT(init0);
	proc->sched.sp = PTR2UINT(proc->kstack+KSTACK-sizeof(up->s.args)-sizeof(uintptr));

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	Segment *seg = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	seg->flushme++;
	proc->seg[SSEG] = seg;
	Page *pg = newpage(1, 0, USTKTOP-BY2PG);
	segpage(seg, pg);
	user_sp = stackspace(pg->pa);

	/*
	 * Text
	 */
	seg = newseg(SG_TEXT, UTZERO, 1);
	proc->seg[TSEG] = seg;
	pg = newpage(1, 0, UTZERO);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(seg, pg);
	uintptr text_pa = seg->map[0]->pages[0]->pa;

	// Custom initcode
	print("memmove initcode %#p\n", UINT2PTR(text_pa));
	print("sizeof initcode = %#p\n", sizeof(qrv32_initcode));
	assert(sizeof(qrv32_initcode) < BY2PG);
	for (int i = 0; i < 0x20; i++) {
		print("\t%x %x %x %x\n", qrv32_initcode[i], qrv32_initcode[++i], qrv32_initcode[++i], qrv32_initcode[++i]);
	}
	print("\n");

	memmove(UINT2PTR(text_pa), qrv32_initcode, sizeof(qrv32_initcode));

	ready(proc);
}

void main() {
	screenputs = sbi_screenputs;

	print("\nPlan 9 Qemu RISC-V 32\n");
	
	memset(edata, 0, end - edata);
	memset(m, 0, sizeof(Mach));
	print("memset zero %#p size %#p\n", edata, end - edata);
	print("memset zero %#p size %#p\n", m, sizeof(Mach));

	machinit();
	quotefmtinstall();
	confinit();
	xinit();
	test_xalloc();

	trapinit();
	printinit();
	timersinit();
	procinit0();
	initseg();
	links();
	pageinit();

	print_mem_info();

	mmuinit();
	print("mmuinit() complete\n");
	
	userinit();
	print("userinit() complete\n");
	schedinit(); // Doesn't return

	assert(0);
}
