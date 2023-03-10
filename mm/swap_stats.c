#include <linux/swap_stats.h>

atomic_t adc_profile_counters[NUM_ADC_COUNTER_TYPE];
EXPORT_SYMBOL(adc_profile_counters);

static const char *adc_profile_counter_names[NUM_ADC_COUNTER_TYPE] = {
	"Demand       ",
	"Prefetch     ",
	"HitOnCache   ",
	"Swapout      ",
	"Total Reclaim",
	"FS Reclaim   ",
};

void report_adc_profile_counters(void)
{
	int type;
	for (type = 0; type < NUM_ADC_COUNTER_TYPE; type++) {
		printk("%s: %d\n", adc_profile_counter_names[type],
		       get_adc_profile_counter(type));
	}
}


struct adc_time_stat adc_time_stats[NUM_ADC_TIME_STAT_TYPE];
EXPORT_SYMBOL(adc_time_stats);

void reset_adc_time_stat(enum adc_time_stat_type type)
{
	struct adc_time_stat *ts = &adc_time_stats[type];
	atomic64_set(&(ts->accum_val), 0);
	atomic_set(&(ts->cnt), 0);
}


static const char *adc_time_stat_names[NUM_ADC_TIME_STAT_TYPE] = {
	"major swap duration", "minor swap duration", "swap-out   duration",
	"non-swap   duration", "RDMA read  latency ", "RDMA write latency ",
	"alloc swap slot lat", "alloc swap slot fst",
	"IB polling  latency", "reverse mapping    ", "unmap flush dirty  ",
	"unmap flush        ", "fs reclaim cnt     ", "fs reclaim dur     "
};

void report_adc_time_stat(void)
{
	int type;
	for (type = 0; type < NUM_ADC_TIME_STAT_TYPE; type++) {
		struct adc_time_stat *ts = &adc_time_stats[type];
		if ((unsigned)atomic_read(&ts->cnt) == 0) {
			printk("%s: %dns, #: %d", adc_time_stat_names[type], 0, 0);
		} else {
			int64_t dur = (int64_t)atomic64_read(&ts->accum_val) /
				      (int64_t)atomic_read(&ts->cnt) * 1000 /
				      RMGRID_CPU_FREQ;
			printk("%s: %lldns, #: %lld",
			       adc_time_stat_names[type], dur,
			       (int64_t)atomic_read(&ts->cnt));
		}
	}
}

void reset_adc_swap_stats(void)
{
	int i;
	for (i = 0; i < NUM_ADC_COUNTER_TYPE; i++) {
		reset_adc_profile_counter(i);
	}

	for (i = 0; i < NUM_ADC_TIME_STAT_TYPE; i++) {
		reset_adc_time_stat(i);
	}
}

// [RMGrid] page fault breakdown profiling
struct adc_time_stat adc_time_stats[NUM_ADC_TIME_STAT_TYPE];
struct adc_pf_time_stat_list adc_pf_breakdowns[NUM_ADC_PF_TYPE];

#ifdef ADC_PROFILE_PF_BREAKDOWN
static const char *adc_pf_breakdown_names[NUM_ADC_PF_BREAKDOWN_TYPE] = {
	"TRAP_TO_KERNEL    ",
	"LOCK_GET_PTE      ",
	"LOOKUP_SWAPCACHE  ",
	"PAGE_IO           ",
	"CGROUP_ACCOUNT    ",
	"PAGE_RECLAIM      ",
	"SET_PAGEMAP_UNLOCK",
	"RET_TO_USER       ",
	"TOTAL_PF          ",
	"DEDUP_SWAPIN      ",
	"PREFETCH          ",
	"POLL_LOAD         ",
	"UPD_METADATA      ",
	"SETPTE            ",
	"PG_CHECK_REF      ",
	"TRY_TO_UNMAP      ",
	"TLB_FLUSH_DIRTY   ",
	"RLS_PG_RM_MAP     ",
	"UNMAP_TLB_FLUSH   ",
	"ALLOC_SWAP_SLOT   ",
	"ADD_TO_SWAPCACHE  ",
	"ADC_SHRNK_ACTV_LST",
	"READ_PAGE         ",
	"WRITE_PAGE        ",
};

static const char *adc_pf_type_names[NUM_ADC_PF_TYPE] = {
	"Major   SPF", "Minor   SPF", "Swapout SPF", "Avg All SPF"
};

void report_adc_pf_breakdown(uint64_t *buf)
{
	int type;
	for (type = 0; type < NUM_ADC_PF_TYPE; type++) {
		int i;
		int cnt = atomic_read(&adc_pf_breakdowns[type].cnt);
		for (i = 0; i < NUM_ADC_PF_BREAKDOWN_TYPE; i++) {
			uint64_t duration =
				cnt == 0 ?
					      0 :
					      atomic64_read(&adc_pf_breakdowns[type]
							       .accum_vals[i]) /
						cnt * 1000 / RMGRID_CPU_FREQ;
			printk("%s: %s \t%lluns", adc_pf_type_names[type],
			       adc_pf_breakdown_names[i], duration);
		}
		printk("Total #(%s): %d\n", adc_pf_type_names[type], cnt);
	}
}
#else
void report_adc_pf_breakdown(uint64_t *buf)
{
}
#endif // ADC_PROFILE_PF_BREAKDOWN


#include <linux/debugfs.h>

bool fs_enabled;
bool fs_hardlimit;
unsigned fs_headroom;
unsigned fs_nr_cores;
unsigned fs_cores[NR_CPUS];
int fastswap_init(struct dentry *root)
{
	int i;
	fs_enabled = true;
	fs_hardlimit = true;
	fs_headroom = 2048;
	fs_nr_cores = 1;
	for (i = 0; i < 15; i++) {
		fs_cores[i] = 31 - i;
	}
	debugfs_create_bool("fs_enabled", 0666, root, &fs_enabled);
	debugfs_create_bool("fs_hardlimit", 0666, root, &fs_hardlimit);
	debugfs_create_u32("fs_headroom", 0666, root, &fs_headroom);
	debugfs_create_u32("fs_nr_cores", 0666, root, &fs_nr_cores);
	return 0;
}

#ifdef HERMIT_DBG_PF_TRACE
atomic_t spf_cnt;
unsigned int *spf_lat_buf[NUM_SPF_LAT_TYPE];

int __init hermit_debugfs_init(void) {
 	struct dentry *root = debugfs_create_dir("hermit", NULL);
	static struct debugfs_blob_wrapper spf_lat_blob[NUM_SPF_LAT_TYPE];
	int i;
	char fname[15] = "spf_lats0\0";

	if (!root)
		return 0;

	debugfs_create_atomic_t("spf_cnt", 0666, root, &spf_cnt);
	atomic_set(&spf_cnt, 0);
	for (i = 0; i < NUM_SPF_LAT_TYPE; i++) {
		spf_lat_buf[i] = kvmalloc(sizeof(unsigned int) * SPF_BUF_LEN,
					  GFP_KERNEL);
		spf_lat_blob[i].data = (void *)spf_lat_buf[i];
		spf_lat_blob[i].size = sizeof(unsigned int) * SPF_BUF_LEN;
		sprintf(fname, "spf_lats%d", i);
		debugfs_create_blob(fname, 0666, root, &spf_lat_blob[i]);
	}

	// init fastswap parameters for offloading
	fastswap_init(root);
	return 0;
}
#else
int __init hermit_debugfs_init(void) {
 	struct dentry *root = debugfs_create_dir("hermit", NULL);
	int i;

	if (!root)
		return 0;

	// init fastswap parameters for offloading
	fastswap_init(root);
	return 0;
}
#endif
__initcall(hermit_debugfs_init);
