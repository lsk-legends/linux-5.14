/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HERMIT_UTILS_H
#define _LINUX_HERMIT_UTILS_H
/*
 * Declarations for Hermit functions in mm/hermit_utils.c
 */
#include <linux/adc_macros.h>
#include <linux/adc_timer.h>
#include <linux/swap.h>
#include <linux/hermit_types.h>

/***
 * util functions
 */

void hermit_faultin_page(struct vm_area_struct *vma,
		unsigned long addr, struct page * page, pte_t*pte, pte_t orig_pte);
#endif // _LINUX_HERMIT_UTILS_H
