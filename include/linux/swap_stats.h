/**
 * swap_stats.h - collect swap stats for profiling
 */

#ifndef _LINUX_SWAP_STATS_H
#define _LINUX_SWAP_STATS_H

#include <linux/swap.h>

#define RSWAP_KERNEL_SUPPORT

// profile swap-in cnts
enum adc_counter_type {
	ADC_ONDEMAND_SWAPIN,
	ADC_PREFETCH_SWAPIN,
	ADC_HIT_ON_PREFETCH,
	ADC_SWAPOUT,
	ADC_TOTAL_RECLAIM,
	ADC_FS_RECLAIM,
	NUM_ADC_COUNTER_TYPE
};

extern atomic_t adc_profile_counters[NUM_ADC_COUNTER_TYPE];

static inline void reset_adc_profile_counter(enum adc_counter_type type)
{
	atomic_set(&adc_profile_counters[type], 0);
}

static inline void adc_profile_counter_inc(enum adc_counter_type type)
{
	atomic_inc(&adc_profile_counters[type]);
}

static inline void adc_profile_counter_add(int val, enum adc_counter_type type)
{
	atomic_add(val, &adc_profile_counters[type]);
}

static inline int get_adc_profile_counter(enum adc_counter_type type)
{
	return (int)atomic_read(&adc_profile_counters[type]);
}

void report_adc_profile_counters(void);

// profile page fault latency
enum adc_pf_bits {
	ADC_PF_SWAP_BIT = 0,
	ADC_PF_MAJOR_BIT = 1,
	ADC_PF_SWAPOUT_BIT = 2
};

static inline void set_adc_pf_bits(int *word, enum adc_pf_bits bit_pos)
{
	if (!word)
		return;
	*word |= (1 << bit_pos);
}

static inline bool get_adc_pf_bits(int word, enum adc_pf_bits bit_pos)
{
	return !!(word & (1 << bit_pos));
}

// profile accumulated time stats
struct adc_time_stat {
	atomic64_t accum_val;
	atomic_t cnt;
};

enum adc_time_stat_type {
	ADC_SWAP_MAJOR_DUR,
	ADC_SWAP_MINOR_DUR,
	ADC_SWAP_OUT_DUR,
	ADC_NON_SWAP_DUR,
	ADC_RDMA_READ_LAT,
	ADC_RDMA_WRITE_LAT,
	ADC_ALLOC_SLOT_LAT,
	ADC_ALLOC_SLOT_FST,
	ADC_IB_POLLING_LAT,
	ADC_RMAP_LAT,
	ADC_TLB_FLUSH_DIR,
	ADC_TLB_FLUSH_LAT,
	ADC_FS_RECLAIM_CNT,
	ADC_FS_RECLAIM_DUR,
	NUM_ADC_TIME_STAT_TYPE
};

extern struct adc_time_stat adc_time_stats[NUM_ADC_TIME_STAT_TYPE];
void reset_adc_time_stat(enum adc_time_stat_type type);

static inline void accum_adc_time_stat(enum adc_time_stat_type type, uint64_t val)
{
	struct adc_time_stat *ts = &adc_time_stats[type];
	atomic64_add(val, &ts->accum_val);
	atomic_inc(&ts->cnt);
}

static inline void accum_multi_adc_time_stat(enum adc_time_stat_type type,
					     uint64_t val, int cnt)
{
	struct adc_time_stat *ts = &adc_time_stats[type];
	atomic64_add(val, &ts->accum_val);
	atomic_add(cnt, &ts->cnt);
}

void report_adc_time_stat(void);
void reset_adc_swap_stats(void);

// time utils
// reference cycles.
// #1, Fix the clock cycles of CPU.
// #2, Divided by CPU frequency to calculate the wall time.
// 500 cycles/ 4.0GHz * 10^9 ns = 500/4.0 ns = xx ns.
// Use "__asm__" in header files (".h") and "asm" in source files (".c")
static inline uint64_t get_cycles_start(void)
{
	uint32_t cycles_high, cycles_low;
	__asm__ __volatile__("xorl %%eax, %%eax\n\t"
			     "CPUID\n\t"
			     "RDTSC\n\t"
			     "mov %%edx, %0\n\t"
			     "mov %%eax, %1\n\t"
			     : "=r"(cycles_high), "=r"(cycles_low)::"%rax",
			       "%rbx", "%rcx", "%rdx");
	return ((uint64_t)cycles_high << 32) + (uint64_t)cycles_low;
}

// More strict than get_cycles_start since "RDTSCP; read registers; CPUID"
// gurantee all instructions before are executed and all instructions after
// are not speculativly executed
// Refer to https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
static inline uint64_t get_cycles_end(void)
{
	uint32_t cycles_high, cycles_low;
	__asm__ __volatile__("RDTSCP\n\t"
			     "mov %%edx, %0\n\t"
			     "mov %%eax, %1\n\t"
			     "xorl %%eax, %%eax\n\t"
			     "CPUID\n\t"
			     : "=r"(cycles_high), "=r"(cycles_low)::"%rax",
			       "%rbx", "%rcx", "%rdx");
	return ((uint64_t)cycles_high << 32) + (uint64_t)cycles_low;
}

#define RMGRID_CPU_FREQ 2994 // in MHz
static inline uint64_t timer_start_in_us(void)
{
	return get_cycles_start() / RMGRID_CPU_FREQ;
}
static inline uint64_t timer_end_in_us(void)
{
	return get_cycles_end() / RMGRID_CPU_FREQ;
}

// [RMGrid] page fault breakdown profiling
enum adc_pf_breakdown_type {
	ADC_TRAP_TO_KERNEL,
	ADC_LOCK_GET_PTE,
	ADC_LOOKUP_SWAPCACHE,
	ADC_PAGE_IO,
	ADC_CGROUP_ACCOUNT,
	ADC_PAGE_RECLAIM,
	ADC_SET_PAGEMAP_UNLOCK,
	ADC_RET_TO_USER,
	ADC_TOTAL_PF,
	ADC_DEDUP_SWAPIN,
	ADC_PREFETCH,
	ADC_POLL_LOAD,
	ADC_UPD_METADATA,
	ADC_SETPTE,
	ADC_PG_CHECK_REF,
	ADC_TRY_TO_UNMAP,
	ADC_TLB_FLUSH_DIRTY,
	ADC_RLS_PG_RM_MAP,
	ADC_UNMAP_TLB_FLUSH,
	ADC_ALLOC_SWAP_SLOT,
	ADC_ADD_TO_SWAPCACHE,
	ADC_SHRNK_ACTV_LST,
	ADC_READ_PAGE,
	ADC_WRITE_PAGE,
	NUM_ADC_PF_BREAKDOWN_TYPE
};

enum adc_pf_type {
	ADC_MAJOR_SPF,
	ADC_MINOR_SPF,
	ADC_SWAPOUT_SPF,
	ADC_ALL_SPF,
	NUM_ADC_PF_TYPE
};
struct adc_pf_time_stat_list {
	atomic64_t accum_vals[NUM_ADC_PF_BREAKDOWN_TYPE];
	atomic_t cnt;
};
extern struct adc_pf_time_stat_list adc_pf_breakdowns[NUM_ADC_PF_TYPE];

#define ADC_PROFILE_PF_BREAKDOWN

#ifdef ADC_PROFILE_PF_BREAKDOWN
static inline void adc_pf_breakdown_stt(uint64_t *pf_breakdown,
					enum adc_pf_breakdown_type type,
					uint64_t ts)
{
	if (!pf_breakdown)
		return;
	pf_breakdown[type] -= ts;
}

static inline void adc_pf_breakdown_end(uint64_t *pf_breakdown,
					enum adc_pf_breakdown_type type,
					uint64_t ts)
{
	if (!pf_breakdown)
		return;
	pf_breakdown[type] += ts;
}

static inline void reset_adc_pf_breakdown(void)
{
	int i, j;
	for (i = 0; i < NUM_ADC_PF_TYPE; i++) {
		for (j = 0; j < NUM_ADC_PF_BREAKDOWN_TYPE; j++) {
			atomic64_set(&adc_pf_breakdowns[i].accum_vals[j], 0);
		}
		atomic_set(&adc_pf_breakdowns[i].cnt, 0);
	}
}

static inline void accum_adc_pf_breakdown(uint64_t pf_breakdown[], enum adc_pf_type pf_type)
{
	const int MAX_CNT = (1 << 30);
	if (!pf_breakdown)
		return;
	if (atomic_read(&adc_pf_breakdowns[pf_type].cnt) < MAX_CNT) {
		int i;
		atomic_inc(&adc_pf_breakdowns[pf_type].cnt);
		for (i = 0; i < NUM_ADC_PF_BREAKDOWN_TYPE; i++) {
			atomic64_add(pf_breakdown[i],
				     &adc_pf_breakdowns[pf_type].accum_vals[i]);
		}
	}
}
void report_adc_pf_breakdown(uint64_t *buf);
#else
static inline void adc_pf_breakdown_stt(uint64_t *pf_breakdown,
					enum adc_pf_breakdown_type type,
					uint64_t ts)
{
}

static inline void adc_pf_breakdown_end(uint64_t *pf_breakdown,
					enum adc_pf_breakdown_type type,
					uint64_t ts)
{
}

static inline void reset_adc_pf_breakdown(void)
{
}

static inline void accum_adc_pf_breakdown(int major, uint64_t pf_breakdown[])
{
}

void report_adc_pf_breakdown(uint64_t *buf);
#endif // ADC_PROFILE_PF_BREAKDOWN

// profile swap-in faults
#define HERMIT_DBG_PF_TRACE
#ifdef HERMIT_DBG_PF_TRACE
#define SPF_BUF_LEN (100L * 1000 * 1000)
#define NUM_SPF_LAT_TYPE 6
extern atomic_t spf_cnt;
extern unsigned int *spf_lat_buf[NUM_SPF_LAT_TYPE];

static inline void record_spf_lat(uint64_t pf_breakdown[])
{
	int cnt;
	cnt = atomic_inc_return(&spf_cnt) - 1;
	if (cnt >= SPF_BUF_LEN)
		return;
	spf_lat_buf[0][cnt] = pf_breakdown[ADC_PAGE_IO];
	spf_lat_buf[1][cnt] = pf_breakdown[ADC_CGROUP_ACCOUNT];
	spf_lat_buf[2][cnt] = pf_breakdown[ADC_LOCK_GET_PTE] +
			      pf_breakdown[ADC_LOOKUP_SWAPCACHE] +
			      pf_breakdown[ADC_SET_PAGEMAP_UNLOCK];
	spf_lat_buf[3][cnt] = pf_breakdown[ADC_TOTAL_PF];
}
#else
static inline void record_spf_lat(uint64_t pf_breakdown[])
{
}
#endif // HERMIT_DBG_PF_TRACE


// Fastswap utils
extern bool fs_enabled; // control flag to enable fastswap's offloading
extern bool fs_hardlimit; // control flag to restrict memory usage under the hard limit
extern unsigned fs_headroom; // for offloaded reclamation, counted in #(pages)
extern unsigned fs_nr_cores; // dedicated cores for offloaded reclamation
extern unsigned fs_cores[]; // id of dedicated cores are used

// hashing dedicated cores with current core id to make workload balanced
static inline unsigned hash_fs_core(void)
{
	return (smp_processor_id() / 2) % fs_nr_cores;
}

static inline unsigned get_fs_core(void)
{
	return fs_cores[hash_fs_core()];
}

#endif /* _LINUX_SWAP_STATS_H */
