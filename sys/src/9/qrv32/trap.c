#include "u.h"
#include "ureg.h"
#include "tos.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

// RISC-V ISA manual vol II
// Table 4.2: Supervisor cause register (scause) values after trap.
#define IRQ_BIT BIT(31)

#define USoftwareIRQ	(IRQ_BIT | 0) 	/* User software interrupt */
#define SSoftwareIRQ	(IRQ_BIT | 1) 	/* Supervisor software interrupt */
#define UTimerIRQ		(IRQ_BIT | 4) 	/* User timer interrupt */
#define STimerIRQ		(IRQ_BIT | 5) 	/* Supervisor timer interrupt */
#define UExternalIRQ	(IRQ_BIT | 8) 	/* User external interrupt */
#define SExternalIRQ	(IRQ_BIT | 9)	/* Supervisor external interrupt */

#define ErrInstrAlignment		0		/* Instruction address misaligned */
#define ErrInstrAccessFault		1		/* Instruction access fault */
#define ErrIllegalInstr			2		/* Illegal instruction */
#define Breakpoint				3		/* Breakpoint */
#define ErrLoadAddrAlignment	4		/* Load address misaligned */
#define ErrLoadAccessFault		5		/* Load access fault */
#define ErrStoreAddrAlignment	6		/* Store/AMO address misaligned */
#define ErrStoreAccessFault		7		/* Store/AMO access fault */
#define UECALL					8		/* Environment call from U-mode */
#define SECALL					9		/* Environment call from S-mode */
#define ErrInstrPageFault		12		/* Instruction page fault */
#define ErrLoadPageFault		13		/* Load page fault */
#define ErrStorePageFault		15		/* Store/AMO page fault */

typedef u32int u32;

char *trapstring(u32 type) {
	switch (type) {
		case USoftwareIRQ: return "User software interrupt";
		case SSoftwareIRQ: return "Supervisor software interrupt";
		case UTimerIRQ: return "User timer interrupt";
		case STimerIRQ: return "Supervisor timer interrupt";
		case UExternalIRQ: return "User external interrupt";
		case SExternalIRQ: return "Supervisor external interrupt";

		case ErrInstrAlignment: return "Instruction address misaligned";
		case ErrInstrAccessFault: return "Instruction access fault";
		case ErrIllegalInstr: return "Illegal instruction";
		case Breakpoint: return "Breakpoint";
		case ErrLoadAddrAlignment: return "Load address misaligned";
		case ErrLoadAccessFault: return "Load access fault";
		case ErrStoreAddrAlignment: return "Store/AMO address misaligned";
		case ErrStoreAccessFault: return "Store/AMO access fault";
		case UECALL: return "Environment call from U-mode";
		case SECALL: return "Environment call from S-mode";
		case ErrInstrPageFault: return "Instruction page fault";
		case ErrLoadPageFault: return "Load page fault";
		case ErrStorePageFault: return "Store/AMO page fault";
	}
	return nil;
}

void printureg(Ureg *ureg) {
	print("Ureg.pc     = 0x%p\n", ureg->pc);
	print("Ureg.r1(ra) = 0x%p\n", ureg->r1);
	print("Ureg.r2(sp) = 0x%p\n", ureg->r2);
	print("Ureg.r3(sb) = 0x%p\n", ureg->r3);
	print("Ureg.r4     = 0x%p\n", ureg->r4);
	print("Ureg.r5     = 0x%p\n", ureg->r5);
	print("Ureg.r6     = 0x%p\n", ureg->r6);
	print("Ureg.r7     = 0x%p\n", ureg->r7);
	print("Ureg.r8     = 0x%p\n", ureg->r8);
	print("Ureg.r9     = 0x%p\n", ureg->r9);
	print("Ureg.r10    = 0x%p\n", ureg->r10);
	print("Ureg.r11    = 0x%p\n", ureg->r11);
	print("Ureg.r12    = 0x%p\n", ureg->r12);
	print("Ureg.r13    = 0x%p\n", ureg->r13);
	print("Ureg.r14    = 0x%p\n", ureg->r14);
	print("Ureg.r15    = 0x%p\n", ureg->r15);
	print("Ureg.r16    = 0x%p\n", ureg->r16);
	print("Ureg.r17    = 0x%p\n", ureg->r17);
	print("Ureg.r18    = 0x%p\n", ureg->r18);
	print("Ureg.r19    = 0x%p\n", ureg->r19);
	print("Ureg.r20    = 0x%p\n", ureg->r20);
	print("Ureg.r21    = 0x%p\n", ureg->r21);
	print("Ureg.r22    = 0x%p\n", ureg->r22);
	print("Ureg.r23    = 0x%p\n", ureg->r23);
	print("Ureg.r24    = 0x%p\n", ureg->r24);
	print("Ureg.r25    = 0x%p\n", ureg->r25);
	print("Ureg.r26    = 0x%p\n", ureg->r26);
	print("Ureg.r27    = 0x%p\n", ureg->r27);
	print("Ureg.r28    = 0x%p\n", ureg->r28);
	print("Ureg.r29    = 0x%p\n", ureg->r29);
	print("Ureg.r30    = 0x%p\n", ureg->r30);
	print("Ureg.r31    = 0x%p\n", ureg->r31);

	print("Ureg.status = 0x%p\n", ureg->status);
	print("Ureg.ie     = 0x%p\n", ureg->ie);
	print("Ureg.cause  = 0x%p\n", ureg->cause);
	print("Ureg.type   = 0x%p\n", ureg->type);
	print("Ureg.tval   = 0x%p\n", ureg->tval);
	print("Ureg.curmode= 0x%p\n", ureg->curmode);
}

static void faultriscv(Ureg *ureg, uintptr va, int user, int read) {
	int n, insyscall;
	char buf[ERRMAX];

	if(up == nil) {
		printureg(ureg);
		panic("fault: nil up in faultriscv, accessing %#p", va);
	}
	insyscall = up->insyscall;
	up->insyscall = 1;

	n = fault(va, read);
	if(n < 0){
		if (up->pid == 1) {
			print("pid 1 %s fault va %#p\n", read ? "read": "write", va);
			printureg(ureg);
			halt();
		}

		if(!user){
			printureg(ureg);
			panic("fault: kernel accessing %#p", va);
		}
		/* don't dump registers; programs suicide all the time */
		snprint(buf, sizeof(buf), "sys: trap: fault %s va=%#p", read ? "read": "write", va);
		postnote(up, 1, buf, NDebug);
	}
	up->insyscall = insyscall;
}

void print_interrupt_header(u32 cause, uintptr user_trap, Ureg *ureg_location, u32 is_page_fault) {
	if (is_page_fault) {
		return;
	}
	print("\n-----------------------------------------\n");
	char *trap_str = trapstring(cause);
	if (user_trap) {
		print("Interrupt (U mode): ");
	} else {
		print("Interrupt (S mode): ");
	}
	print("%s, ", trap_str);
	if (trap_str == nil) {
		print("Unrecognized trap cause: %#P, ", cause);
	}
	print("ureg copy location %#p\n", ureg_location);
}

Ureg *c_trap() {
	Ureg *ureg = (Ureg *) UREGADDR;
	uintptr spp = (ureg->status & 0x100);
	u32 scause = ureg->cause;
	u32 user_trap = !(spp >> 8);
	Ureg *ureg_copy_location;
	u32 is_page_fault = (scause == ErrInstrPageFault)
			|| (scause == ErrLoadPageFault)
			|| (scause == ErrStorePageFault);
	

	if (spp) {
		ureg_copy_location = UINT2PTR(INTR_STK_LOW_END);
	} else {
		ureg_copy_location = (Ureg *) up->kstack;
	}

	print_interrupt_header(scause, user_trap, ureg_copy_location, is_page_fault);

	if (!user_trap && !is_page_fault) {
		printureg(ureg);
		panic("Traps from S-mode not supported");
	}

	memmove(ureg_copy_location, ureg, sizeof(Ureg));
	ureg = ureg_copy_location;
	up->dbgreg = ureg;

	switch (scause) {
		case ErrInstrPageFault:
			faultriscv(ureg, ureg->tval, user_trap, 1);
			break;
		case ErrLoadPageFault:
			faultriscv(ureg, ureg->tval, user_trap, 1);
			break;
		case ErrStorePageFault:
			faultriscv(ureg, ureg->tval, user_trap, 0);
			break;
		case UECALL:
			syscall(ureg);
			break;
		case SECALL:
			panic("Environment call from S-mode");
			break;
		case Breakpoint:
			printureg(ureg);
			print("Continuing execution...\n");
			// TODO increment PC?
			break;

		default:
			print("Trap cause not yet handled\n");
			printureg(ureg);
			spin();
			break;
	}

	return ureg;
}

void trapinit() {
	write_stvec((uintptr) stvec_asm);
	spllo();
}
