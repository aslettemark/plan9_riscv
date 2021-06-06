#include "u.h"
#include "tos.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "external/sbi.h"

void crude_bin_print(uint k) {
	char buf[33];
	buf[32] = '\0';
	for (int i = 0; i < 32; i++) {
		if ((k & BIT(i)) != 0) {
			buf[31 - i] = '1';
		} else {
			buf[31 - i] = '0';
		}
	}
	sbi_printstr("0b");
	sbi_printstr(buf);
	sbi_printstr("\n");
}

char *hexchar = "0123456789ABCDEF";
void crude_hex_print(uint k) {
	char buf[9];
	buf[8] = '\0';
	for (int i = 0; i < 8; i++) {
		uint mask = 0xF << (i * 4);
		uint index = (k & mask) >> (i * 4);
		buf[7 - i] = hexchar[index];
	}

	sbi_printstr("0x");
	sbi_printstr(buf);
	sbi_printstr("\n");
}

void sbi_putchar(char c) {
	SBI_ECALL_1(SBI_EXT_0_1_CONSOLE_PUTCHAR, c);
}

void sbi_printstr(const char *str) {
	while (str && *str)
		SBI_ECALL_1(SBI_EXT_0_1_CONSOLE_PUTCHAR, (*str++));
}

char sbi_readchar() {
	int i = SBI_ECALL_0(SBI_EXT_0_1_CONSOLE_GETCHAR);
	if (i == SBI_ERR_FAILED) {
		//sbi_printstr("sbi_readchar: SBI_ERR_FAILED\n");
		return 0;
	}
	if (i == SBI_ERR_NOT_SUPPORTED) {
		sbi_printstr("sbi_readchar: SBI_ERR_NOT_SUPPORTED");
		return 0;
	}

	char c = (char) i;
	return c;
}

void sbi_screenputs(char *str, int len) {
	for (int i = 0; i < len; i++) {
		sbi_putchar(str[i]);
	}
}
