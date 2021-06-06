#define WFI     WORD $0x10500073    /* wait for interrupt */
#define SRET    WORD $0x10200073      /* return from supervisor mode handling */

#define R_0 0u
#define R_1 1u
#define R_2 2u
#define R_3 3u
#define R_4 4u
#define R_5 5u
#define R_6 6u
#define R_7 7u
#define R_8 8u
#define R_9 9u
#define R_10 10u
#define R_11 11u
#define R_12 12u
#define R_13 13u
#define R_14 14u
#define R_15 15u
#define R_16 16u
#define R_17 17u
#define R_18 18u
#define R_19 19u
#define R_20 20u
#define R_21 21u
#define R_22 22u
#define R_23 23u
#define R_24 24u
#define R_25 25u
#define R_26 26u
#define R_27 27u
#define R_28 28u
#define R_29 29u
#define R_30 30u
#define R_31 31u

/* Atomics */
#define AMO_opcode 0x2F /* 0101111 */
#define W_width 0x2
#define SWAP_W 1
#define AMO_instr_w(funct5, aq, rl, rs2, rs1, rd) WORD $(AMO_opcode | (rd << 7) | (W_width << 12) | (rs1 << 15) | (rs2 << 20) | (rl << 25) | (aq << 26) | (funct5 << 27))

#define SFENCE_VMA(rs2, rs1)    WORD $(0x12000073 | (rs2 << 20) | (rs1 << 15))
#define SFENCE_VMA_ALL          SFENCE_VMA(R_0, R_0)
