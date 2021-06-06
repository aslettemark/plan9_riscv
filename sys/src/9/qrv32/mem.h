#define KiB     1024u       /*! Kibi 0x0000000000000400 */
#define MiB     1048576u    /*! Mebi 0x0000000000100000 */
#define GiB     1073741824u /*! Gibi 000000000040000000 */
// 0x80M0P000

/*

To be safe, we don't want to touch any memory below 0x80200000
It's not properly documented, but it's the assumption Linux uses
when booting in S-mode, so it's probably..
a) safe to assume
b) wise to stick to
See: https://github.com/riscv/opensbi/issues/70

*/

#define RAMZERO      0x80000000
#define KZERO        0x80000000                     /*! kernel address space */
#define USABLE_KZERO 0x80200000                     /* See note above */
#define	KTZERO		 0x80400000		                /* kernel text start (mkfile loadaddr, sbi jump addr, ..) */
#define MEMSIZE      (256 * MiB)                    /* TODO: inflexible */
#define PHYSRAMTOP  (RAMZERO + MEMSIZE)

#define	PGSHIFT		12			/* log(BY2PG) */
#define BY2PG       (4*KiB)                 /*! bytes per page */
#define	BY2WD		4			            /* bytes per word */
#define BY2V        8                       /*! only used in xalloc.c */

#define UREGSIZE            256                         /* I think it's actually (31 + 1 + 4) * BY2WD ish */
#define MACHSIZE            128                         /* overestimate */

// Memory area between 0x80200000 and 0x80400000
// We manage this manually and carefully
// Everything else should go in kernel text, data, bss, or allocated after bss (base from conf)
#define KSTKSIZE            (8 * KiB)
#define KSTACK_LOW_END      USABLE_KZERO                       /* Lower end of kernel stack */
#define INTR_STK_SIZE       KSTKSIZE
#define INTR_STK_LOW_END    (KSTACK_LOW_END + KSTKSIZE)
#define INTR_STK_TOP        (INTR_STK_LOW_END + INTR_STK_SIZE - 4)  /* subtracting 4 for convenience of getting a writeable memory address */
#define MACHADDR            (INTR_STK_LOW_END + INTR_STK_SIZE)          /*! Mach structure, placed after kernel stack and intr stack */
#define UREGADDR            (MACHADDR + MACHSIZE)
#define ROOT_PAGE_TABLE     0x803FF000 /* Last page of manually managed */

#define KSTACK              KSTKSIZE                    /* legacy? */

// QEMU RV32 Virt machine
#define UART0 0x10000000L
#define VIRTIO0 0x10001000
#define CLINT 0x2000000L
#define PLIC 0x0c000000L

// Stolen values
#define PTEMAPMEM	(1024*1024)
#define PTEPERTAB	(PTEMAPMEM/BY2PG)
#define	SEGMAPSIZE	1984
#define SSEGMAPSIZE	16
#define BLOCKALIGN	32			/* only used in allocb.c */

#define PPN(x)		((x)&~(BY2PG-1))
#define STACKALIGN(sp)	((sp) & ~3)     /* Probably? */


// RISC-V SV32 spec
#define	PTEVALID	(1<<0)
#define PTEREAD     (1<<1)
#define	PTEWRITE	(1<<2)
#define PTEEXECUTE  (1<<3)
#define PTEUMODE    (1<<4)
#define PTEGLOBAL   (1<<5)
#define PTEACCESSED (1<<6)
#define PTEDIRTY    (1<<7)

// DEFINITELY NOT from the RISC-V SV32 spec
// These bits are supposed to be reserved, and not ever actually written to a PTE
// But they need a value to be able to detangle the clusterfuck mess made by port/fault.c and
// the kernel implementations assuming way too much about what a PTE looks like instead of giving
// reasonable arguments to putmmu
#define	PTEUNCACHED	(1<<8)
#define	PTERONLY	(1<<9)

#define	USTKTOP		0x80000000		    /* user segment end +1 */

#define	UZERO		0			        /* user segment */
#define	UTZERO		(UZERO+BY2PG)		/* user text start */
#define UTROUND(t)	ROUNDUP((t), BY2PG)
#define	USTKSIZE	(8*1024*1024)		/* user stack size */
#define	TSTKTOP		(USTKTOP-USTKSIZE)	/* sysexec temporary stack */
#define	TSTKSIZ	 	4096

#define	getpgcolor(a)	0
