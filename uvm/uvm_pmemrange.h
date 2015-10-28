/*	$OpenBSD: uvm_pmemrange.h,v 1.12 2015/02/05 23:51:06 mpi Exp $	*/

/*
 * Copyright (c) 2009 Ariane van der Steldt <ariane@stack.nl>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * uvm_pmemrange.h: describe and manage free physical memory.
 */

#ifndef _UVM_UVM_PMEMRANGE_H_
#define _UVM_UVM_PMEMRANGE_H_

RB_HEAD(uvm_pmr_addr, vm_page);
RB_HEAD(uvm_pmr_size, vm_page);

/*
 * Page types available:
 * - DIRTY: this page may contain random data.
 * - ZERO: this page has been zeroed.
 */
#define UVM_PMR_MEMTYPE_DIRTY	0
#define UVM_PMR_MEMTYPE_ZERO	1
#define UVM_PMR_MEMTYPE_MAX	2

/*
 * An address range of memory.
 */
struct uvm_pmemrange {
	struct	uvm_pmr_addr addr;	/* Free page chunks, sorted by addr. */
	struct	uvm_pmr_size size[UVM_PMR_MEMTYPE_MAX];
					/* Free page chunks, sorted by size. */
	TAILQ_HEAD(, vm_page) single[UVM_PMR_MEMTYPE_MAX];
					/* single page regions (uses pageq) */

	paddr_t	low;			/* Start of address range (pgno). */
	paddr_t	high;			/* End +1 (pgno). */
	int	use;			/* Use counter. */
	psize_t	nsegs;			/* Current range count. */

	TAILQ_ENTRY(uvm_pmemrange) pmr_use;
					/* pmr, sorted by use */
	RB_ENTRY(uvm_pmemrange) pmr_addr;
					/* pmr, sorted by address */
};

/*
 * Description of failing memory allocation.
 *
 * Two ways new pages can become available:
 * [1] page daemon drops them (we notice because they are freed)
 * [2] a process calls free
 *
 * The buffer cache and page daemon can decide that they don't have the
 * ability to make pages available in the requested range. In that case,
 * the FAIL bit will be set.
 * XXX There's a possibility that a page is no longer on the queues but
 * XXX has not yet been freed, or that a page was busy.
 * XXX Also, wired pages are not considered for paging, so they could
 * XXX cause a failure that may be recoverable.
 */
struct uvm_pmalloc {
	TAILQ_ENTRY(uvm_pmalloc) pmq;

	/*
	 * Allocation request parameters.
	 */
	struct uvm_constraint_range pm_constraint;
	psize_t	pm_size;

	/*
	 * State flags.
	 */
	int	pm_flags;
};

/*
 * uvm_pmalloc flags.
 */
#define UVM_PMA_LINKED	0x01	/* uvm_pmalloc is on list */
#define UVM_PMA_BUSY	0x02	/* entry is busy with fpageq unlocked */
#define UVM_PMA_FAIL	0x10	/* page daemon cannot free pages */
#define UVM_PMA_FREED	0x20	/* at least one page in the range was freed */

RB_HEAD(uvm_pmemrange_addr, uvm_pmemrange);
TAILQ_HEAD(uvm_pmemrange_use, uvm_pmemrange);

/*
 * pmr control structure. Contained in uvm.pmr_control.
 */
struct uvm_pmr_control {
	struct	uvm_pmemrange_addr addr;
	struct	uvm_pmemrange_use use;

	/* Only changed while fpageq is locked. */
	TAILQ_HEAD(, uvm_pmalloc) allocs;
};

void	uvm_pmr_freepages(struct vm_page *, psize_t);
void	uvm_pmr_freepageq(struct pglist *);
int	uvm_pmr_getpages(psize_t, paddr_t, paddr_t, paddr_t, paddr_t,
	    int, int, struct pglist *);
void	uvm_pmr_init(void);
int	uvm_wait_pla(paddr_t, paddr_t, paddr_t, int);
void	uvm_wakeup_pla(paddr_t, psize_t);

#if defined(DDB) || defined(DEBUG)
int	uvm_pmr_isfree(struct vm_page *pg);
#endif

/*
 * Internal tree logic.
 */

int	uvm_pmr_addr_cmp(struct vm_page *, struct vm_page *);
int	uvm_pmr_size_cmp(struct vm_page *, struct vm_page *);

RB_PROTOTYPE(uvm_pmr_addr, vm_page, objt, uvm_pmr_addr_cmp);
RB_PROTOTYPE(uvm_pmr_size, vm_page, objt, uvm_pmr_size_cmp);
RB_PROTOTYPE(uvm_pmemrange_addr, uvm_pmemrange, pmr_addr,
    uvm_pmemrange_addr_cmp);

struct vm_page		*uvm_pmr_insert_addr(struct uvm_pmemrange *,
			    struct vm_page *, int);
void			 uvm_pmr_insert_size(struct uvm_pmemrange *,
			    struct vm_page *);
struct vm_page		*uvm_pmr_insert(struct uvm_pmemrange *,
			    struct vm_page *, int);
void			 uvm_pmr_remove_addr(struct uvm_pmemrange *,
			    struct vm_page *);
void			 uvm_pmr_remove_size(struct uvm_pmemrange *,
			    struct vm_page *);
void			 uvm_pmr_remove(struct uvm_pmemrange *,
			    struct vm_page *);
struct vm_page		*uvm_pmr_extract_range(struct uvm_pmemrange *,
			    struct vm_page *, paddr_t, paddr_t,
			    struct pglist *);

#endif /* _UVM_UVM_PMEMRANGE_H_ */
