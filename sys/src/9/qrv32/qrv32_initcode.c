/*
 * IMPORTANT!  DO NOT ADD LIBRARY CALLS TO THIS FILE.
 * The entire text image must fit on one page
 */

#include <u.h>
#include <libc.h>

char cons[] = "#c/cons";
char dev[] = "/dev";
char c[] = "#c";
char e[] = "#e";
char ec[] = "#ec";
char s[] = "#s";
char srv[] = "/srv";
char env[] = "/env";

char *boot_cmd[] = {"/boot/boot", "boot", nil};

void startboot()
{
	char buf[200];	/* keep this fairly large to capture error details */

	/* in case boot is a shell script */
	open(cons, OREAD);
	open(cons, OWRITE);
	open(cons, OWRITE);
	bind(c, dev, MAFTER);
	bind(ec, env, MAFTER);
	bind(e, env, MCREATE|MAFTER);
	bind(s, srv, MREPL|MCREATE);

	exec(boot_cmd[0], boot_cmd);

	rerrstr(buf, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	_exits(buf);
}
