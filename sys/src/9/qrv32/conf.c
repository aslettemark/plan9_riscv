#include "u.h"
#include "tos.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include <pool.h>

extern char confname[MAXCONF][KNAMELEN];
extern char confval[MAXCONF][MAXCONFLINE];
extern int nconf;

void confinit()
{
	conf.nmach = 1;

	uintptr base = PGROUND(PTR2UINT(end));
	uintptr limit = base + MEMSIZE;
	usize npage = (limit - base) / BY2PG;

	conf.mem[0].base = base;
	conf.mem[0].limit = limit;

	conf.mem[0].npage = npage;
	conf.npage = npage;

	conf.nproc = 100 + ((conf.npage * BY2PG) / MB) * 5; /* BCM */

	/* BCM */
	conf.upages = (conf.npage * 80) / 100;
	conf.ialloc = ((conf.npage - conf.upages) / 2) * BY2PG;
	conf.nswap = conf.npage * 3;
	conf.nswppo = 4096;
	conf.nimage = 200;

	/*
	conf.copymode = ;
	conf.pipeqsize = ;
	conf.hz = ;
	conf.mhz = ;
	conf.monitor = ;
    */

	// BCM
	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for
	 * (probably ~300KB).
	 */
	ulong kpages = conf.npage - conf.upages;
	ulong kpagebytes = kpages * BY2PG;
	kpagebytes -= conf.upages * sizeof(Page) + conf.nproc * sizeof(Proc) + conf.nimage * sizeof(Image) + conf.nswap + conf.nswppo * sizeof(Page);
	mainmem->maxsize = kpagebytes;
	if (!cpuserver)
	{
		/*
		 * give terminals lots of image memory, too; the dynamic
		 * allocation will balance the load properly, hopefully.
		 * be careful with 32-bit overflow.
		 */
		imagmem->maxsize = kpagebytes;
	}

	print("conf base = 0x%lux\n", base);
	print("conf npage = %ud\n", npage);
}

static int findconf(char *name)
{
	int i;

	for (i = 0; i < nconf; i++)
		if (cistrcmp(confname[i], name) == 0)
			return i;
	return -1;
}

char *getconf(char *name)
{
	int i = findconf(name);
	if (i >= 0)
		return confval[i];
	return nil;
}
