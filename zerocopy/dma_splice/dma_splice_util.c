/* dma_splice_util.c
 *
 * A module for splicing data received via dma.
 * 
 * Author: Matthew Weaver <weaver@slac.stanford.edu>
 * 
 * 2008 (c) SLAC, Stanford University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/swap.h>

int pmd_huge(pmd_t pmd)
{
	return !!(pmd_val(pmd) & _PAGE_PSE);
}

struct page *
follow_huge_pmd(struct mm_struct *mm, unsigned long address,
		pmd_t *pmd, int write)
{
	struct page *page;

	page = pte_page(*(pte_t *)pmd);
	if (page)
		page += ((address & ~HPAGE_MASK) >> PAGE_SHIFT);
	return page;
}

static inline int is_cow_mapping(unsigned int flags)
{
	return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}

/*
 * vm_normal_page -- This function gets the "struct page" associated with a pte.
 *
 * "Special" mappings do not wish to be associated with a "struct page" (either
 * it doesn't exist, or it exists but they don't want to touch it). In this
 * case, NULL is returned here. "Normal" mappings do have a struct page.
 *
 * There are 2 broad cases. Firstly, an architecture may define a pte_special()
 * pte bit, in which case this function is trivial. Secondly, an architecture
 * may not have a spare pte bit, which requires a more complicated scheme,
 * described below.
 *
 * A raw VM_PFNMAP mapping (ie. one that is not COWed) is always considered a
 * special mapping (even if there are underlying and valid "struct pages").
 * COWed pages of a VM_PFNMAP are always normal.
 *
 * The way we recognize COWed pages within VM_PFNMAP mappings is through the
 * rules set up by "remap_pfn_range()": the vma will have the VM_PFNMAP bit
 * set, and the vm_pgoff will point to the first PFN mapped: thus every special
 * mapping will always honor the rule
 *
 *	pfn_of_page == vma->vm_pgoff + ((addr - vma->vm_start) >> PAGE_SHIFT)
 *
 * And for normal mappings this is false.
 *
 * This restricts such mappings to be a linear translation from virtual address
 * to pfn. To get around this restriction, we allow arbitrary mappings so long
 * as the vma is not a COW mapping; in that case, we know that all ptes are
 * special (because none can have been COWed).
 *
 *
 * In order to support COW of arbitrary special mappings, we have VM_MIXEDMAP.
 *
 * VM_MIXEDMAP mappings can likewise contain memory with or without "struct
 * page" backing, however the difference is that _all_ pages with a struct
 * page (that is, those where pfn_valid is true) are refcounted and considered
 * normal pages by the VM. The disadvantage is that pages are refcounted
 * (which can be slower and simply not an option for some PFNMAP users). The
 * advantage is that we don't have to follow the strict linearity rule of
 * PFNMAP mappings in order to support COWable mappings.
 *
 */
struct page *vm_normal_page(struct vm_area_struct *vma, unsigned long addr,
				pte_t pte)
{
	unsigned long pfn = pte_pfn(pte);

	if (unlikely(vma->vm_flags & (VM_PFNMAP|VM_MIXEDMAP))) {
		if (vma->vm_flags & VM_MIXEDMAP) {
			if (!pfn_valid(pfn))
				return NULL;
			goto out;
		} else {
			unsigned long off;
			off = (addr - vma->vm_start) >> PAGE_SHIFT;
			if (pfn == vma->vm_pgoff + off) {
				printk(KERN_ALERT "pfn==vma->vm_pgoff+off\n");
				return NULL;
			}
			if (!is_cow_mapping(vma->vm_flags)) {
				/*** return NULL; ***/
			}
		}
	}

	VM_BUG_ON(!pfn_valid(pfn));

	/*
	 * NOTE! We still have PageReserved() pages in the page tables.
	 *
	 * eg. VDSO mappings can cause them to exist.
	 */
out:
	return pfn_to_page(pfn);
}

/*
 * Do a quick page-table lookup for a single page.
 */
struct page *__follow_page(struct vm_area_struct *vma, 
			   unsigned long address,
			   unsigned int flags)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep, pte;
	spinlock_t *ptl;
	struct page *page;
	struct mm_struct *mm = vma->vm_mm;

	page = NULL;
	pgd = pgd_offset(mm, address);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
		goto no_page_table;

	pud = pud_offset(pgd, address);
	if (pud_none(*pud) || unlikely(pud_bad(*pud)))
		goto no_page_table;
	
	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd))
		goto no_page_table;

	if (pmd_huge(*pmd)) {
		BUG_ON(flags & FOLL_GET);
		page = follow_huge_pmd(mm, address, pmd, flags & FOLL_WRITE);
		goto out;
	}

	if (unlikely(pmd_bad(*pmd)))
		goto no_page_table;

	ptep = pte_offset_map_lock(mm, pmd, address, &ptl);

	pte = *ptep;
	if (!pte_present(pte))
		goto no_page;
	if ((flags & FOLL_WRITE) && !pte_write(pte))
		goto unlock;
	page = vm_normal_page(vma, address, pte);
	if (unlikely(!page))
		goto bad_page;

	if (flags & FOLL_GET)
		get_page(page);
	if (flags & FOLL_TOUCH) {
		if ((flags & FOLL_WRITE) &&
		    !pte_dirty(pte) && !PageDirty(page))
			set_page_dirty(page);
		mark_page_accessed(page);
	}
unlock:
	pte_unmap_unlock(ptep, ptl);
out:
	return page;

bad_page:
	printk(KERN_ALERT "bad_page\n");
	pte_unmap_unlock(ptep, ptl);
	return ERR_PTR(-EFAULT);

no_page:
	printk(KERN_ALERT "no_page for addr %lx\n",address);
	pte_unmap_unlock(ptep, ptl);
	if (!pte_none(pte))
		return page;
	/* Fall through to ZERO_PAGE handling */
no_page_table:
	/*
	 * When core dumping an enormous anonymous area that nobody
	 * has touched so far, we don't want to allocate page tables.
	 */
	printk(KERN_ALERT "no_page_table\n");
	if (flags & FOLL_ANON) {
		page = ZERO_PAGE(0);
		if (flags & FOLL_GET)
			get_page(page);
		BUG_ON(flags & FOLL_WRITE);
	}
	return page;
}

static struct vm_area_struct *
__find_extend_vma(struct mm_struct * mm, unsigned long addr)
{
	struct vm_area_struct * vma;

	addr &= PAGE_MASK;
	vma = find_vma(mm,addr);
	if (!vma) {
		printk(KERN_ALERT "find_extend_vma find_vma failed\n");
		return NULL;
	}
	if (vma->vm_start <= addr) {
		return vma;
	}
	return NULL;
}

/**
 **  An ill-advised disregard for the rules of vm_flags
 **/
static int __get_user_pages(struct task_struct *tsk, 
			    struct mm_struct *mm,
			    unsigned long start,
			    int len,
			    struct page** pages)
{
  struct vm_area_struct *vma;
  struct page* page;
  unsigned long foll_flags = FOLL_GET;
  int i = 0;

  vma = __find_extend_vma(mm, start);
  if (!vma) return 0;

  while( (i<len) && (page=__follow_page(vma, start, foll_flags)) ) {
        pages[i++] = page;
	start     += PAGE_SIZE;
  }

  return i;
}


/* Copied from fs/pipe.c because it is not exported to modules */
void pipe_wait(struct pipe_inode_info *pipe)
{
	DEFINE_WAIT(wait);

	/*
	 * Pipes are system-local resources, so sleeping on them
	 * is considered a noninteractive wait:
	 */
	prepare_to_wait(&pipe->wait, &wait, TASK_INTERRUPTIBLE);
	if (pipe->inode)
		mutex_unlock(&pipe->inode->i_mutex);
	schedule();
	finish_wait(&pipe->wait, &wait);
	if (pipe->inode)
		mutex_lock(&pipe->inode->i_mutex);
}

/* Copied from fs/splice.c because it is not exported to modules */
static ssize_t ssplice_to_pipe(struct pipe_inode_info *pipe,
			       struct splice_pipe_desc *spd)
{
  	unsigned int spd_pages = spd->nr_pages; 
	int ret, do_wakeup, page_nr;

	ret = 0;
	do_wakeup = 0;
	page_nr = 0;

	if (pipe->inode)
		mutex_lock(&pipe->inode->i_mutex);

	for (;;) {
		if (!pipe->readers) {
			send_sig(SIGPIPE, current, 0);
			if (!ret)
				ret = -EPIPE;
			break;
		}

		if (pipe->nrbufs < PIPE_BUFFERS) {
			int newbuf = (pipe->curbuf + pipe->nrbufs) & (PIPE_BUFFERS - 1);
			struct pipe_buffer *buf = pipe->bufs + newbuf;

			buf->page = spd->pages[page_nr];
			buf->offset = spd->partial[page_nr].offset;
			buf->len = spd->partial[page_nr].len;
			buf->private = spd->partial[page_nr].private;
			buf->ops = spd->ops;
			if (spd->flags & SPLICE_F_GIFT)
				buf->flags |= PIPE_BUF_FLAG_GIFT;

			pipe->nrbufs++;
			page_nr++;
			ret += buf->len;

			if (pipe->inode)
				do_wakeup = 1;

			if (!--spd->nr_pages)
				break;
			if (pipe->nrbufs < PIPE_BUFFERS)
				continue;

			break;
		}

		if (spd->flags & SPLICE_F_NONBLOCK) {
			if (!ret)
				ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			if (!ret)
				ret = -ERESTARTSYS;
			break;
		}

		if (do_wakeup) {
			smp_mb();
			if (waitqueue_active(&pipe->wait))
				wake_up_interruptible_sync(&pipe->wait);
			kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
			do_wakeup = 0;
		}

		pipe->waiting_writers++;
		pipe_wait(pipe);
		pipe->waiting_writers--;
	}

	if (pipe->inode) {
		mutex_unlock(&pipe->inode->i_mutex);

		if (do_wakeup) {
			smp_mb();
			if (waitqueue_active(&pipe->wait))
				wake_up_interruptible(&pipe->wait);
			kill_fasync(&pipe->fasync_readers, SIGIO, POLL_IN);
		}
	}

	while (page_nr < spd_pages)
		spd->spd_release(spd, page_nr++);

	return ret;
}

