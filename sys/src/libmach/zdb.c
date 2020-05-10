#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "../cmd/zc/z.out.h"

static char *riscvexcep(Map*, Rgetter);

/*
 * RISCV-specific debugger interface
 */

typedef struct	Instr	Instr;
struct	Instr
{
	Map	*map;
	ulong	w;
	uvlong	addr;
	int	op;
	int	func3;
	int	func7;
	char	rs1, rs2, rs3, rd;
	long	imm;

	char*	curr;			/* fill point in buffer */
	char*	end;			/* end of buffer */
};

typedef struct Optab	Optab;
struct Optab {
	int	func7;
	int	op[8];
};
		
typedef struct Opclass	Opclass;
struct Opclass {
	char	*fmt;
	Optab	tab[4];
};

/* Major opcodes */
enum {
	OLOAD,	 OLOAD_FP,  Ocustom_0,	OMISC_MEM, OOP_IMM, OAUIPC, OOP_IMM_32,	O48b,
	OSTORE,	 OSTORE_FP, Ocustom_1,	OAMO,	   OOP,	    OLUI,   OOP_32,	O64b,
	OMADD,	 OMSUB,	    ONMSUB,	ONMADD,	   OOP_FP,  Ores_0, Ocustom_2,	O48b_2,
	OBRANCH, OJALR,	    Ores_1,	OJAL,	   OSYSTEM, Ores_2, Ocustom_3,	O80b
};

/* copy anames from compiler */
static
#include "../cmd/zc/enam.c"

static Opclass opOLOAD = {
	"A,D",
	0,	AMOVB,	AMOVH,	AMOVW,	AMOVL,	AMOVBU,	AMOVHU,	AMOVWU,	0,
};
static Opclass opOLOAD_FP = {
	"A,D",
	0,	0,	0,	AMOVF,	AMOVD,	0,	0,	0,	0,
};
static Opclass opOMISC_MEM = {
	"",
	0,	AFENCE,	AFENCE_I,0,	0,	0,	0,	0,	0,
};
static Opclass opOOP_IMM = {
	"$I,S,D",
	0x20,	0,	0,	0,	0,	0,	ASRA,	0,	0,
	0,	AADD,	ASLL,	ASLT,	ASLTU,	AXOR,	ASRL,	AOR,	AAND,
};
static Opclass opOAUIPC = {
	"$I(PC),D",
	0,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,
};
static Opclass opOOP_IMM_32 = {
	"$I,S,D",
	0x20,	0,	0,	0,	0,	0,	ASRAW,	0,	0,
	0,	AADDW,	ASLLW,	0,	0,	0,	ASRLW,	0,	0,
};
static Opclass opOSTORE = {
	"2,A",
	0,	AMOVB,	AMOVH,	AMOVW,	AMOVL,	0,	0,	0,	0,
};
static Opclass opOSTORE_FP = {
	"2,A",
	0,	0,	0,	AMOVF,	AMOVD,	0,	0,	0,	0,
};
static Opclass opOAMO = {
	"7,2,S,D",
	0x04,	0,	0,	ASWAP_W,ASWAP_D,0,	0,	0,	0,
	0x08,	0,	0,	ALR_W,	ALR_D,	0,	0,	0,	0,
	0x0C,	0,	0,	ASC_W,	ASC_D,	0,	0,	0,	0,
	0,	0,	0,	AAMO_W,	AAMO_D,	0,	0,	0,	0,
};
static Opclass opOOP = {
	"2,S,D",
	0x01,	AMUL,	AMULH,	AMULHSU,AMULHU,	ADIV,	ADIVU,	AREM,	AREMU,
	0x20,	ASUB,	0,	0,	0,	0,	ASRA,	0,	0,
	0,	AADD,	ASLL,	ASLT,	ASLTU,	AXOR,	ASRL,	AOR,	AAND,
};
static Opclass opOLUI = {
	"$I,D",
	0,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,	ALUI,
};
static Opclass opOOP_32 = {
	"2,S,D",
	0x01,	AMULW,	0,	0,	0,	ADIVW,	ADIVUW,	AREMW,	AREMUW,
	0x20,	ASUBW,	0,	0,	0,	0,	ASRAW,	0,	0,
	0,	AADDW,	ASLLW,	0,	0,	0,	ASRLW,	0,	0,
};
static Opclass opOBRANCH = {
	"2,S,p",
	0,	ABEQ,	ABNE,	0,	0,	ABLT,	ABGE,	ABLTU,	ABGEU,
};
static Opclass opOJALR = {
	"D,A",
	0,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,	AJALR,
};
static Opclass opOJAL = {
	"D,p",
	0,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,	AJAL,
};
static Opclass opOSYSTEM = {
	"$I",
	0,	ASYS,	ACSRRW,	ACSRRS,	ACSRRC,	0,	ACSRRWI,ACSRRSI,ACSRRCI,
};

/* map major opcodes to opclass table */
static Opclass *opclass[32] = {
	[OLOAD]		&opOLOAD,
	[OLOAD_FP]	&opOLOAD_FP,
	[OMISC_MEM]	&opOMISC_MEM,
	[OOP_IMM]	&opOOP_IMM,
	[OAUIPC]	&opOAUIPC,
	[OOP_IMM_32]	&opOOP_IMM_32,
	[OSTORE]	&opOSTORE,
	[OSTORE_FP]	&opOSTORE_FP,
	[OAMO]		&opOAMO,
	[OOP]		&opOOP,
	[OLUI]		&opOLUI,
	[OOP_32]	&opOOP_32,
	[OBRANCH]	&opOBRANCH,
	[OJALR]		&opOJALR,
	[OJAL]		&opOJAL,
	[OSYSTEM]	&opOSYSTEM,
};

/*
 * Print value v as name[+offset]
 */
static int
gsymoff(char *buf, int n, ulong v, int space)
{
	Symbol s;
	int r;
	long delta;

	r = delta = 0;		/* to shut compiler up */
	if (v) {
		r = findsym(v, space, &s);
		if (r)
			delta = v-s.value;
		if (delta < 0)
			delta = -delta;
	}
	if (v == 0 || r == 0 || delta >= 4096)
		return snprint(buf, n, "#%lux", v);
	if (strcmp(s.name, ".string") == 0)
		return snprint(buf, n, "#%lux", v);
	if (!delta)
		return snprint(buf, n, "%s", s.name);
	if (s.type != 't' && s.type != 'T')
		return snprint(buf, n, "%s+%llux", s.name, v-s.value);
	else
		return snprint(buf, n, "#%lux", v);
}

#pragma	varargck	argpos	bprint		2

static void
bprint(Instr *i, char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	i->curr = vseprint(i->curr, i->end, fmt, arg);
	va_end(arg);
}

static void
format(Instr *i, char *mnemonic, char *f)
{
	int c;
	long imm;

	bprint(i, "%s", mnemonic);
	if(f == 0)
		return;
	bprint(i, "\t");
	for(; (c = *f); f++){
		switch(c){
		default:
			bprint(i, "%c", c);
			break;
		case 'S':
			bprint(i, "R%d", i->rs1);
			break;
		case '2':
			bprint(i, "R%d", i->rs2);
			break;
		case '3':
			bprint(i, "R%d", i->rs3);
			break;
		case 'D':
			bprint(i, "R%d", i->rd);
			break;
		case 'I':
			imm = i->imm;
			if(imm < 0)
				bprint(i, "-%lux", -imm);
			else
				bprint(i, "%lux", imm);
			break;
		case 'p':
			imm = i->addr + i->imm;
			i->curr += gsymoff(i->curr, i->end-i->curr, imm, CANY);
			break;
		case 'A':
			bprint(i, "%lx(R%d)", i->imm, i->rs1);
			break;
		case '7':
			bprint(i, "%ux", i->func7);
			break;
		}
	}
}

static int
badinst(Instr *i)
{
	format(i, "???", 0);
	return 4;
}

static int
decode(Map *map, uvlong pc, Instr *i)
{
	ulong w;
	int op;

	if(get4(map, pc, &w) < 0) {
		werrstr("can't read instruction: %r");
		return -1;
	}
	i->w = w;
	op = (w&0x7F);
	i->op = op;
	i->func3 = (w>>12)&0x7;
	i->func7 = (w>>25)&0x7F;
	i->rs1 = (w>>15)&0x1F;
	i->rs2 = (w>>20)&0x1F;
	i->rs3 = (2>>27)&0x1F;
	i->rd = (w>>7)&0x1F;
#define FIELD(hi,lo,off)	(w>>(lo-off))&(((1<<(hi-lo+1))-1)<<off)
#define LFIELD(hi,lo,off)	(w<<(off-lo))&(((1<<(hi-lo+1))-1)<<off)
#define SFIELD(lo,off)		((long)(w&((~0)<<lo))>>(lo-off))
	switch(op>>2) {
	case OSTORE:	/* S-type */
	case OSTORE_FP:
		i->imm = SFIELD(25,5) | FIELD(11,7,0);
		break;
	case OBRANCH:	/* B-type */
		i->imm = SFIELD(31,12) | LFIELD(7,7,11) | FIELD(30,25,5) | FIELD(11,8,1);
		break;
	case OOP_IMM:	/* I-type */
	case OOP_IMM_32:
		if(i->func3 == 5){		/* special case ASL/ASR */
			i->imm = FIELD(24,20,0);
			break;
		}
	/* fall through */
	case OLOAD:
	case OLOAD_FP:
	case OMISC_MEM:
	case OJALR:
	case OSYSTEM:
		i->imm = SFIELD(20,0);
		break;
	case OAUIPC:	/* U-type */
	case OLUI:
		i->imm = SFIELD(12,12);
		break;
	case OJAL:	/* J-type */
		i->imm = SFIELD(31,20) | FIELD(19,12,12) | FIELD(20,20,11) | FIELD(30,21,1);
		break;
	}
	i->addr = pc;
	i->map = map;
	return 1;
}

static int
pseudo(Instr *i, int aop)
{
	switch(aop){
	case AJAL:
		if(i->rd == 0){
			format(i, "JMP",	"p");
			return 1;
		}
		break;
	case AJALR:
		if(i->rd == 0){
			format(i, "JMP", "A");
			return 1;
		}
		break;
	case AADD:
		if((i->op>>2) == OOP_IMM){
			if(i->rs1 == 0)
				format(i, "MOVW", "$I,D");
			else if(i->imm == 0)
				format(i, "MOVW", "R,D");
			else break;
			return 1;
		}
		break;
	}
	return 0;
}

static int
riscvdas(Map *map, uvlong pc, char modifier, char *buf, int n)
{
	Instr i;
	Opclass *oc;
	Optab *o;

	USED(modifier);
	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	if((i.op&0x3) != 0x3)
		return badinst(&i);
	oc = opclass[i.op>>2];
	if(oc == 0)
		return badinst(&i);
	o = oc->tab;
	while(o->func7 != 0 && (i.func7 != o->func7 || o->op[i.func3] == 0))
		o++;
	if(o->op[i.func3] == 0)
		return badinst(&i);
	if(pseudo(&i, o->op[i.func3]))
		return 4;
	format(&i, anames[o->op[i.func3]], oc->fmt);
	return 4;
}

static int
riscvhexinst(Map *map, uvlong pc, char *buf, int n)
{
	Instr i;

	i.curr = buf;
	i.end = buf+n;
	if(decode(map, pc, &i) < 0)
		return -1;
	if(i.end-i.curr > 8)
		i.curr = _hexify(buf, i.w, 7);
	*i.curr = 0;
	return 4;
}

static int
riscvinstlen(Map *map, uvlong pc)
{
	Instr i;

	if(decode(map, pc, &i) < 0)
		return -1;
	return 4;
}

static char*
riscvexcep(Map*, Rgetter)
{
	return "Trap";
}

/*
 *	Debugger interface
 */
Machdata riscvmach =
{
	{0x3a, 0xa0, 0x3d, 0x00},		/* break point */
	4,			/* break point size */

	leswab,			/* short to local byte order */
	leswal,			/* long to local byte order */
	leswav,			/* long to local byte order */
	risctrace,		/* C traceback */
	riscframe,		/* Frame finder */
	riscvexcep,		/* print exception */
	0,				/* breakpoint fixup */
	0,				/* single precision float printer */
	0,				/* double precisioin float printer */
	0,				/* following addresses */
	riscvdas,		/* symbolic disassembly */
	riscvhexinst,	/* hex disassembly */
	riscvinstlen,	/* instruction size */
};
