#include "../port/portfns.h"

char* getconf(char*);
#define	kmapinval()

#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define userureg(ur) 1

#define countpagerefs(a, b)

#define procsave(p)
#define procrestore(p)

#define PTR2UINT(p)	((uintptr)(p))
#define UINT2PTR(i)	((void*)(i))

#define KADDR(p) ((uintptr) p)
#define PADDR(p) ((uintptr) p)

#define BIT(n) (1u << (n))

// SBI
#define SBI_ECALL(__num, __a0, __a1, __a2) opensbi_ecall(__a0, __a1, __a2, __num)
#define SBI_ECALL_0(__num) SBI_ECALL(__num, 0, 0, 0)
#define SBI_ECALL_1(__num, __a0) SBI_ECALL(__num, __a0, 0, 0)
#define SBI_ECALL_2(__num, __a0, __a1) SBI_ECALL(__num, __a0, __a1, 0)
void sbi_putchar(char c);
void sbi_printstr(const char *str);
char sbi_readchar();
void sbi_screenputs(char *str, int len);
int opensbi_ecall(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long num);
void crude_bin_print(uint);
void crude_hex_print(uint);

// Low level asm
void wfi();
int read_scause();
int debug_csr();
void write_stvec(uintptr);
void write_satp(uintptr);
int	tas(void*);
void stvec_asm();
void sfence_vma(void);
void touser(uintptr);
void coherence(void);
int get_sstatus(void);
int set_sstatus_sum_bit(void);

// trap.c
void trapinit();
void printureg(Ureg *);

// mmu.c
void mmuinit();

// syscall.c
void syscall(Ureg *);
