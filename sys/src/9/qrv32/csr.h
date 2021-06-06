// Supervisor Trap Setup
#define sstatus     0x100 /* Supervisor status register. */
#define sedeleg     0x102 /* Supervisor exception delegation register. */
#define sideleg     0x103 /* Supervisor interrupt delegation register. */
#define sie         0x104 /* Supervisor interrupt-enable register. */
#define stvec       0x105 /* Supervisor trap handler base address. */
#define scounteren  0x106 /* Supervisor counter enable. */

// Supervisor Trap Handling
#define sscratch    0x140 /* Scratch register for supervisor trap handlers. */
#define sepc        0x141 /* Supervisor exception program counter. */
#define scause      0x142 /* Supervisor trap cause. */
#define stval       0x143 /* Supervisor bad address or instruction. */
#define sip         0x144 /* Supervisor interrupt pending */

// Supervisor Protection and Translation
#define satp        0x180 /* Supervisor address translation and protection. */
