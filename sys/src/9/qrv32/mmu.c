#include "u.h"
#include "ureg.h"

#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define PGROUNDDOWN(x) ROUNDDN((x), BY2PG)
#define PGROUNDUP(x)   ROUNDUP((x), BY2PG)

typedef u32int u32;

PTE *toplevel_pagetable = UINT2PTR(ROOT_PAGE_TABLE);

#define PTE2PA(pte) (((pte) >> 10) << 12)

/*
 * This whole MMU implementation does not use huge tables. You cannot mix-and-match these functions with other MMU implementations.
*/

void print_pte(PTE e) {
	print("PTE 0x%p ", e);
	
	if (e & PTEVALID) {
		print("V");
	} else {
		print("(not valid)");
	}

	if (e & PTEEXECUTE) {
		print("X");
	}
	if (e & PTEWRITE) {
		print("W");
	}
	if (e & PTEREAD) {
		print("R");
	}
	if (e & PTEUMODE) {
		print("U");
	}

	if (!(e & (PTEEXECUTE | PTEWRITE | PTEREAD))) {
		print(" (pointer)");
	}

	u32 ppn1 = e >> 20;
	u32 ppn0 = (e >> 10) & 0x3FF;
	print("\t\tPPN[1]=0x%p PPN[0]=0x%p", ppn1, ppn0);
	
	print("\n");
}

/*
Assumes va, pa are page aligned
*/
void map_single_page(u32 va, u32 pa, u32 flags, PTE *l1_table) {
	u32 vpn1 = (va >> 22) & 0x3FF;
	u32 vpn0 = (va >> 12) & 0x3FF;

	u32 ppn1 = (pa >> 22) & 0x3FF;
	u32 ppn0 = (pa >> 12) & 0x3FF;
	
	PTE entry = l1_table[vpn1];
	if (entry & PTEVALID) {
		u32 mask = PTEEXECUTE | PTEWRITE | PTEREAD;
		if (entry & mask) {
			panic("Top level remap va = 0x%p\n", va);
		}

		// Now we know entry is a pointer to next level
		PTE *l0_table = UINT2PTR(PTE2PA(entry));
		if (l0_table[vpn0] & PTEVALID) {
			print_pte(l0_table[vpn0]);
			panic("L0 remap va = 0x%p\n", va);
		}
		PTE leaf_entry = (ppn1 << 20) | (ppn0 << 10) | flags | PTEVALID;
		l0_table[vpn0] = leaf_entry;
	} else {
		// Create new l0 table and entry
		Page *page = newpage(1, 0, 0);

		// Fill the entry in the l0 table
		PTE leaf_entry = (ppn1 << 20) | (ppn0 << 10) | flags | PTEVALID;
		PTE *l0_table = UINT2PTR(page->pa);
		// print("entry in new l0: ");
		// print_pte(leaf_entry);
		l0_table[vpn0] = leaf_entry;

		// Write the new page to root level table
		u32 ppn_full = page->pa;
		assert((ppn_full & 0xFFFFF000) == ppn_full);
		PTE l1_entry = (ppn_full >> 2) | PTEVALID;
		// print("entry in l1: ");
		// print_pte(l1_entry);
		l1_table[vpn1] = l1_entry;
	}

	if (va < BY2PG) {
		print("WARNING: mapped zero page (va %#p)\n", va);
	}
}

/*
Map all pages in the address range va to va + size
*/
void kernelmap(u32 va, u32 pa, u32 size, u32 flags) {
	print("kernelmap va=0x%p pa=0x%p size=0x%p flags=0x%p\n", va, pa, size, flags);
	assert(flags & (PTEREAD | PTEWRITE | PTEEXECUTE));
	
	if (va & 0xFFF) {
		panic("kernelmap: non-page-aligned va: 0x%p\n", va);
	}
	if (pa & 0xFFF) {
		panic("kernelmap: non-page-aligned pa: 0x%p\n", pa);
	}
	if (size & 0xFFF) {
		print("kernelmap: Warning: non-page-aligned size 0x%p (rounding up)\n", size);
		size = PGROUNDUP(size);
	}
	for (u32 i = 0; i < size; i += BY2PG) {
		map_single_page(va + i, pa + i, flags, toplevel_pagetable);
	}
}

// TODO: these need to go in the kernel segment
void map_mmio() {
	print("TODO map_mmio()\n");

	// uart registers
	//kernelmap(UART0, UART0, BY2PG, PTEREAD | PTEWRITE);

	// virtio mmio disk interface
	//kernelmap(VIRTIO0, VIRTIO0, BY2PG, PTEREAD | PTEWRITE);

	// CLINT
	//kernelmap(CLINT, CLINT, 0x10000, PTEREAD | PTEWRITE);

	// PLIC
	//kernelmap(PLIC, PLIC, 0x400000, PTEREAD | PTEWRITE);
}

void mmuinit() {
	memset(toplevel_pagetable, 0, BY2PG);

	map_mmio();

	// Ideally we could use a more granular mapping
	// Contrary to what the linked commands indicate I don't think the kernel segments
	// are properly page aligned
	kernelmap(RAMZERO, RAMZERO, MEMSIZE, PTEREAD | PTEWRITE | PTEEXECUTE);

	uintptr satp = BIT(31) | (ROOT_PAGE_TABLE >> 12);
	print("write_satp(0x%p)\n", satp);
	write_satp(satp);
	set_sstatus_sum_bit();
}

u32 interpret_fixfault_flags(u32 flags) {
	assert(flags & PTEVALID);
	u32 actual_flags = 0;

	return PTEVALID | PTEUMODE | PTEREAD | PTEEXECUTE | PTEWRITE; // TODO be more specific re. R/W/X
}

/*
Be careful with pa given here, it has all sorts of flags mixed in from port/fault.c
Use the lower 12 for flags, use page->pa for pa
*/
void putmmu(uintptr va, uintptr pa, Page* page) {
	u32 flags = pa & 0xFFF;
	//print("putmmu va:0x%p pa:0x%p pa(page):0x%p flags:0x%p\n", va, pa, page->pa, flags);
	pa = page->pa;

	assert(va < USTKTOP);
	assert(va >= BY2PG);

	u32 our_actual_flags = interpret_fixfault_flags(flags);

	map_single_page(va, pa, our_actual_flags, toplevel_pagetable);

	coherence();
}

// BCM
void checkmmu(uintptr va, uintptr pa)
{
	USED(va);
	USED(pa);
}

void flush_userspace() {
	print("flush_userspace\n");

	u32 va = UZERO;

	while (va < KZERO) {
		u32 vpn1 = (va >> 22) & 0x3FF;
		u32 vpn0 = (va >> 12) & 0x3FF;

		PTE entry = toplevel_pagetable[vpn1];
		if (entry & PTEVALID) {
			PTE *l0_table = UINT2PTR(PTE2PA(entry));
			memset(l0_table, 0, BY2PG);
		}

		// Skip over all 1024 page entries in one l0 table
		va += BY2PG * 1024;
	}
	
}

/*
 * Flush all the user-space and device-mapping mmu info
 * for this process, because something has been deleted.
 * It will be paged back in on demand.
 */
void flushmmu() {
	print("pid %d flushmmu\n", up->pid);
	int s = splhi();
	flush_userspace();
	sfence_vma();
	splx(s);
}

void mmurelease(Proc* proc) {
	print("mmurelease\n");
	flushmmu();
}

void mmuswitch(Proc* proc) {
	print("mmuswitch pid %d kstack %#p\n", proc->pid, proc->kstack);
	
	flush_userspace();
	sfence_vma();
}

/*
 * Return the number of bytes that can be accessed via KADDR(pa).
 * If pa is not a valid argument to KADDR, return 0.
 * 
 * BCM with mod.
 */
uintptr cankaddr(uintptr pa) {
	if(pa < RAMZERO + MEMSIZE) {
		return RAMZERO + MEMSIZE - pa;
	}
	return 0;
}
