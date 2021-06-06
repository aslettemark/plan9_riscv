#define HZ          (100)       /*! clock frequency */
#define MS2HZ       (1000/HZ)   /*! millisec per clock tick */
#define TK2SEC(t)   ((t)/HZ)    /*! ticks to seconds */
#define MS2TK(t)    ((t)/MS2HZ) /*! milliseconds to ticks */

#define MACHP(n)    (n == 0 ? (Mach*)(MACHADDR) : (Mach*)0)
#define AOUT_MAGIC	Z_MAGIC

typedef struct Lock Lock;
typedef struct Ureg Ureg;
typedef struct Label Label;
typedef struct FPenv FPenv;
typedef struct Mach Mach;
typedef struct FPU FPU;
typedef ulong Instr;
typedef struct Conf Conf;
typedef struct Confmem Confmem;

// Mostly "stolen" values from bcm, etc
#define MAXSYSARG 5
#define MAXMACH 1
typedef uvlong Tval;

#define MAXCONF 64
#define MAXCONFLINE 160

typedef ulong FPsave;
typedef struct sulong Notsave;
#define NCOLOR 1
typedef struct PMMU PMMU;
typedef struct Page Page;
typedef struct Proc Proc;
typedef u32int PTE;

struct PMMU // BCM
{
	int dummy;
};

struct sulong
{
	ulong l;
};

// BCM lock
struct Lock
{
	ulong key;
	u32int sr;
	uintptr pc;
	Proc *p;
	Mach *m;
	int isilock;
};

struct Label
{
	ulong sp; /* known to l.s */
	ulong pc; /* known to l.s */
};

enum /* FPenv.status */
{
	FPINIT,
	FPACTIVE,
	FPINACTIVE
};

struct FPenv
{
	int x;
};

struct FPU
{
	FPenv env;
};

// BCM conf + confmem
struct Confmem
{
	uintptr base;
	usize npage;
	uintptr limit;
	uintptr kbase;
	uintptr klimit;
};

struct Conf
{
	ulong nmach;	 /* processors */
	ulong nproc;	 /* processes */
	Confmem mem[1];	 /* physical memory */
	ulong npage;	 /* total physical pages of memory */
	usize upages;	 /* user page pool */
	ulong copymode;	 /* 0 is copy on write, 1 is copy on reference */
	ulong ialloc;	 /* max interrupt time allocation in bytes */
	ulong pipeqsize; /* size in bytes of pipe queues */
	ulong nimage;	 /* number of page cache image headers */
	ulong nswap;	 /* number of swap pages */
	int nswppo;		 /* max # of pageouts per segment pass */
	ulong hz;		 /* processor cycle freq */
	ulong mhz;
};

#include "../port/portdat.h"

struct Mach
{
	ulong splpc; /* pc of last caller to splhi */
	int machno;	 /* physical id of processor */
	ulong ticks; /* of the clock since boot time */
	Proc *proc;	 /* current process on this processor */
	Label sched; /* scheduler wakeup */

	/* stats, needed for devcons */
	int tlbfault;
	int tlbpurge;
	int pfault;
	int cs;
	int syscall;
	int load;
	Perf perf; /* performance counters */
	int intr;
	int ilockdepth;

	int flushmmu;	  /* flush current proc mmu state */
	Proc *readied;	  /* for runproc */
	ulong schedticks; /* next forced context switch */

	uvlong cyclefreq; /* Frequency of user readable cycle counter */
};

struct
{
	Lock;
	int machs;	 /* bitmap of active CPUs */
	int exiting; /* shutdown */
	int ispanic; /* shutdown in response to a panic */
} active;

extern Mach *m;
extern Proc *up;

/*
 * Fake kmap.
 */
extern uintptr kseg0;
typedef void KMap;
#define VA(k) ((uintptr)(k))
#define kmap(p) (KMap *)((p)->pa | kseg0)
#define kunmap(k)

/*
 * FPsave.fpstate
 */
enum
{
	FPinit,
	FPactive,
	FPinactive,
	FPemu,

	/* bits or'd with the state */
	FPillegal = 0x100,
};
