#include <linux/mm_types.h>
// #include <linux/hermit_types.h>
#include <linux/hermit_utils.h>
// #include <linux/hermit.h>
#include <linux/swap.h>
#include <linux/swap_stats.h>
#include <linux/mm.h>

#include "internal.h"

void hermit_faultin_page(struct vm_area_struct *vma,
		unsigned long addr, struct page * page, pte_t *pte, pte_t orig_pte){
	struct mm_struct *mm = vma->vm_mm;
	vm_fault_t ret;
	pmd_t * pmd;
	struct vm_fault vmf;

	VM_BUG_ON(addr & PAGE_MASK);
	pmd = mm_find_pmd(mm, addr);
	if(!pmd)
		return;

	mmap_read_lock(mm);
	vmf.vma = vma;
	vmf.gfp_mask = GFP_HIGHUSER_MOVABLE;
	vmf.address = addr;
	vmf.pgoff = linear_page_index(vma, addr);
	vmf.pte = pte;
	vmf.orig_pte = orig_pte;
	vmf.page = page;
	vmf.pmd = pmd;
	vmf.flags = FAULT_FLAG_WRITE | FAULT_FLAG_REMOTE | FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_RETRY_NOWAIT;
	ret = do_swap_page_map_pte_profiling(&vmf, NULL, NULL);
	mmap_read_unlock(mm);
	// if(ret == 0)
	// 	adc_counter_add(1, ADC_EARLY_MAP_PTE);
}
EXPORT_SYMBOL(hermit_faultin_page);