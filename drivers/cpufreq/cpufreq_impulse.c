/*
 *  drivers/cpufreq/cpufreq_impulse.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *            (C)  2009 Alexander Clouter <alex@digriz.org.uk>
 *            (C)  2012 Zane Zaminsky <cyxman@yahoo.com>
 *            (C)  2014 Javier Sayago <admin@lonasdigital.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/earlysuspend.h>
#include <linux/sysfs_helpers.h>

// cpu load trigger
#define DEF_SMOOTH_UP (75)

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

//  midnight and impulse default values
#define DEF_FREQUENCY_UP_THRESHOLD		(70)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG1	(68)	//  default for hotplug up threshold for cpu1 (cpu0 stays allways on)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG2	(68)	//  default for hotplug up threshold for cpu2 (cpu0 stays allways on)
#define DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG3	(68)	//  default for hotplug up threshold for cpu3 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD		(52)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG1	(55)	//  default for hotplug down threshold for cpu1 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG2	(55)	//  default for hotplug down threshold for cpu2 (cpu0 stays allways on)
#define DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG3	(55)	//  default for hotplug down threshold for cpu3 (cpu0 stays allways on)
#define DEF_IGNORE_NICE				(0)	//  default for ignore nice load
#define DEF_FREQ_STEP				(5)	//  default for freq step at awake
#define DEF_FREQ_STEP_SLEEP			(5)	//  default for freq step at early suspend

//  LCDFreq Scaling default values
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
#define LCD_FREQ_KICK_IN_DOWN_DELAY		(20)	//  default for kick in down delay
#define LCD_FREQ_KICK_IN_UP_DELAY		(50)	//  default for kick in up delay
#define LCD_FREQ_KICK_IN_FREQ			(500000)//  default kick in frequency
#define LCD_FREQ_KICK_IN_CORES			(0)	//  number of cores which should be online before kicking in
extern int _lcdfreq_lock(int lock);			//  external lcdfreq lock function
#endif

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */

#define MIN_SAMPLING_RATE_RATIO			(2)

//  Sampling down momentum variables
static unsigned int min_sampling_rate;			//  minimal possible sampling rate
static unsigned int orig_sampling_down_factor;		//  for saving previously set sampling down factor
static unsigned int orig_sampling_down_max_mom;		//  for saving previously set smapling down max momentum

//  search limit for frequencies in scaling array, variables for scaling modes and state flags for deadlock fix/suspend detection
static unsigned int max_scaling_freq_soft = 0;		//  init value for "soft" scaling = 0 full range
static unsigned int max_scaling_freq_hard = 0;		//  init value for "hard" scaling = 0 full range
static unsigned int suspend_flag = 0;			//  init value for suspend status. 1 = in early suspend
static unsigned int skip_hotplug_flag = 1;		//  initial start without hotplugging to fix lockup issues
static int scaling_mode_up;				//  fast scaling up mode holding up value during runtime
static int scaling_mode_down;				//  fast scaling down mode holding down value during runtime

// raise sampling rate to SR*multiplier and adjust sampling rate/thresholds/hotplug/scaling/freq limit/freq step on blank screen

//  LCDFreq scaling
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
static int lcdfreq_lock_current = 0;			//  LCDFreq scaling lock switch
static int prev_lcdfreq_lock_current;			//  for saving previously set lock state
static int prev_lcdfreq_enable;				//  for saving previously set enabled state
#endif
static unsigned int sampling_rate_awake;
static unsigned int up_threshold_awake;
static unsigned int down_threshold_awake;
static unsigned int smooth_up_awake;
static unsigned int freq_limit_awake;			//  for saving freqency limit awake value
static unsigned int fast_scaling_awake;			//  for saving fast scaling awake value
static unsigned int freq_step_awake;			//  for saving frequency step awake value
static unsigned int hotplug1_awake;			//  for saving hotplug1 threshold awake value
static unsigned int hotplug2_awake;			//  for saving hotplug2 threshold awake value
static unsigned int hotplug3_awake;			//  for saving hotplug3 threshold awake value
static unsigned int sampling_rate_asleep;		//  for setting sampling rate value on early suspend
static unsigned int up_threshold_asleep;		//  for setting up threshold value on early suspend
static unsigned int down_threshold_asleep;		//  for setting down threshold value on early suspend
static unsigned int smooth_up_asleep;			//  for setting smooth scaling value on early suspend
static unsigned int freq_limit_asleep;			//  for setting frequency limit value on early suspend
static unsigned int fast_scaling_asleep;		//  for setting fast scaling value on early suspend
static unsigned int freq_step_asleep;			//  for setting freq step value on early suspend

//  midnight and impulse momentum defaults
#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define DEF_SAMPLING_DOWN_FACTOR		(1)	//  default for sampling down factor (stratosk default = 4) here disabled by default
#define MAX_SAMPLING_DOWN_FACTOR		(100000)//  changed from 10 to 100000 for sampling down momentum implementation
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

//  Sampling down momentum
#define DEF_SAMPLING_DOWN_MOMENTUM		(0)	//  sampling down momentum disabled by default
#define DEF_SAMPLING_DOWN_MAX_MOMENTUM	 	(0)	//  default for tuneable sampling_down_max_momentum stratosk default=16, here disabled by default
#define DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY  (50)	//  default for tuneable sampling_down_momentum_sensitivity
#define MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY  (1000)	//  max value for tuneable sampling_down_momentum_sensitivity

//  midnight and impulse defaults for suspend
#define DEF_SAMPLING_RATE_SLEEP_MULTIPLIER	(2)	//  default for tuneable sampling_rate_sleep_multiplier
#define MAX_SAMPLING_RATE_SLEEP_MULTIPLIER	(4)	//  maximum for tuneable sampling_rate_sleep_multiplier
#define DEF_UP_THRESHOLD_SLEEP			(90)	//  default for tuneable up_threshold_sleep
#define DEF_DOWN_THRESHOLD_SLEEP		(44)	//  default for tuneable down_threshold_sleep
#define DEF_SMOOTH_UP_SLEEP			(100)	//  default for tuneable smooth_up_sleep

/*
* Hotplug Sleep: 0 do not touch hotplug settings on early suspend, so that all cores will be online
* values 1, 2 or 3 are equivalent to cores which should be online on early suspend
*/

#define DEF_HOTPLUG_SLEEP			(0)	//  default for tuneable hotplug_sleep
#define DEF_GRAD_UP_THRESHOLD			(25)	//  default for grad up threshold

/*
* Frequency Limit: 0 do not limit frequency and use the full range up to cpufreq->max limit
* values 200000 -> 1800000 khz
*/

#define DEF_FREQ_LIMIT				(0)	//  default for tuneable freq_limit
#define DEF_FREQ_LIMIT_SLEEP			(0)	//  default for tuneable freq_limit_sleep

/*
* Fast Scaling: 0 do not activate fast scaling function
* values 1-4 to enable fast scaling with normal down scaling 5-8 to enable fast scaling with fast up and down scaling
*/

#define DEF_FAST_SCALING			(0)	//  default for tuneable fast_scaling
#define DEF_FAST_SCALING_SLEEP			(0)	//  default for tuneable fast_scaling_sleep

static void do_dbs_timer(struct work_struct *work);

struct cpu_dbs_info_s {
	u64 time_in_idle; 				//  added exit time handling
	u64 idle_exit_time;				//  added exit time handling
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	unsigned int down_skip;				//  Smapling down reactivated
	unsigned int check_cpu_skip;			//  check_cpu skip counter (to avoid potential deadlock because of double locks from hotplugging)
	unsigned int requested_freq;
	unsigned int rate_mult;				//  Sampling down momentum sampling rate multiplier
	unsigned int momentum_adder;			//  Sampling down momentum adder
	int cpu;
	unsigned int enable:1;
	unsigned int prev_load;				//  Early demand var for previous load
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, cs_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
	unsigned int sampling_rate;
	unsigned int sampling_rate_sleep_multiplier;	//  added tuneable sampling_rate_sleep_multiplier
	unsigned int sampling_down_factor;		//  Sampling down factor (reactivated)
	unsigned int sampling_down_momentum;		//  Sampling down momentum tuneable
	unsigned int sampling_down_max_mom;		//  Sampling down momentum max tuneable
	unsigned int sampling_down_mom_sens;		//  Sampling down momentum sensitivity
	unsigned int up_threshold;
	unsigned int up_threshold_hotplug1;		//  added tuneable up_threshold_hotplug1 for core1
	unsigned int up_threshold_hotplug2;		//  added tuneable up_threshold_hotplug2 for core2
	unsigned int up_threshold_hotplug3;		//  added tuneable up_threshold_hotplug3 for core3
	unsigned int up_threshold_sleep;		//  added tuneable up_threshold_sleep for early suspend
	unsigned int down_threshold;
	unsigned int down_threshold_hotplug1;		//  added tuneable down_threshold_hotplug1 for core1
	unsigned int down_threshold_hotplug2;		//  added tuneable down_threshold_hotplug2 for core2
	unsigned int down_threshold_hotplug3;		//  added tuneable down_threshold_hotplug3 for core3
	unsigned int down_threshold_sleep;		//  added tuneable down_threshold_sleep for early suspend
	unsigned int ignore_nice;
	unsigned int freq_step;
	unsigned int freq_step_sleep;			//  added tuneable freq_step_sleep for early suspend
	unsigned int smooth_up;
	unsigned int smooth_up_sleep;			//  added tuneable smooth_up_sleep for early suspend
	unsigned int hotplug_sleep;			//  added tuneable hotplug_sleep for early suspend
	unsigned int freq_limit;			//  added tuneable freq_limit
	unsigned int freq_limit_sleep;			//  added tuneable freq_limit_sleep
	unsigned int fast_scaling;			//  added tuneable fast_scaling
	unsigned int fast_scaling_sleep;		//  added tuneable fast_scaling_sleep
	unsigned int grad_up_threshold;			//  Early demand grad up threshold tuneable
	unsigned int early_demand;			//  Early demand master switch
	unsigned int disable_hotplug;			//  Hotplug switch
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	int lcdfreq_enable;				//  LCDFreq Scaling switch
	unsigned int lcdfreq_kick_in_down_delay;	//  LCDFreq Scaling kick in down delay counter
	unsigned int lcdfreq_kick_in_down_left;		//  LCDFreq Scaling kick in down left counter
	unsigned int lcdfreq_kick_in_up_delay;		//  LCDFreq Scaling kick in up delay counter
	unsigned int lcdfreq_kick_in_up_left;		//  LCDFreq Scaling kick in up left counter
	unsigned int lcdfreq_kick_in_freq;		//  LCDFreq Scaling kick in frequency
	unsigned int lcdfreq_kick_in_cores;		//  LCDFreq Scaling kick in cores
#endif

} dbs_tuners_ins = {
	.up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
	.up_threshold_hotplug1 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG1,		//  set default value for new tuneable
	.up_threshold_hotplug2 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG2,		//  set default value for new tuneable
	.up_threshold_hotplug3 = DEF_FREQUENCY_UP_THRESHOLD_HOTPLUG3,		//  set default value for new tuneable
	.up_threshold_sleep = DEF_UP_THRESHOLD_SLEEP,				//  set default value for new tuneable
	.down_threshold = DEF_FREQUENCY_DOWN_THRESHOLD,
	.down_threshold_hotplug1 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG1,	//  set default value for new tuneable
	.down_threshold_hotplug2 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG2,	//  set default value for new tuneable
	.down_threshold_hotplug3 = DEF_FREQUENCY_DOWN_THRESHOLD_HOTPLUG3,	//  set default value for new tuneable
	.down_threshold_sleep = DEF_DOWN_THRESHOLD_SLEEP,			//  set default value for new tuneable
	.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,			//  sampling down reactivated but disabled by default
	.sampling_down_momentum = DEF_SAMPLING_DOWN_MOMENTUM,			//  Sampling down momentum initial disabled
	.sampling_down_max_mom = DEF_SAMPLING_DOWN_MAX_MOMENTUM,		//  Sampling down momentum default for max momentum
	.sampling_down_mom_sens = DEF_SAMPLING_DOWN_MOMENTUM_SENSITIVITY,	//  Sampling down momentum default for sensitivity
	.sampling_rate_sleep_multiplier = DEF_SAMPLING_RATE_SLEEP_MULTIPLIER,	//  set default value for new tuneable
	.ignore_nice = DEF_IGNORE_NICE,						//  set default value for tuneable
	.freq_step = DEF_FREQ_STEP,						//  set default value for new tuneable
	.freq_step_sleep = DEF_FREQ_STEP_SLEEP,					//  set default value for new tuneable
	.smooth_up = DEF_SMOOTH_UP,
	.smooth_up_sleep = DEF_SMOOTH_UP_SLEEP,					//  set default value for new tuneable
	.hotplug_sleep = DEF_HOTPLUG_SLEEP,					//  set default value for new tuneable
	.freq_limit = DEF_FREQ_LIMIT,						//  set default value for new tuneable
	.freq_limit_sleep = DEF_FREQ_LIMIT_SLEEP,				//  set default value for new tuneable
	.fast_scaling = DEF_FAST_SCALING,					//  set default value for new tuneable
	.fast_scaling_sleep = DEF_FAST_SCALING_SLEEP,				//  set default value for new tuneable
	.grad_up_threshold = DEF_GRAD_UP_THRESHOLD,				//  Early demand default for grad up threshold
	.early_demand = 0,							//  Early demand default off
	.disable_hotplug = false,						//  Hotplug switch default off (hotplugging on)
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	.lcdfreq_enable = false,						//  LCDFreq Scaling default off
	.lcdfreq_kick_in_down_delay = LCD_FREQ_KICK_IN_DOWN_DELAY,		//  LCDFreq Scaling default for down delay
	.lcdfreq_kick_in_down_left = LCD_FREQ_KICK_IN_DOWN_DELAY,		//  LCDFreq Scaling default for down left
	.lcdfreq_kick_in_up_delay = LCD_FREQ_KICK_IN_UP_DELAY,			//  LCDFreq Scaling default for up delay
	.lcdfreq_kick_in_up_left = LCD_FREQ_KICK_IN_UP_DELAY,			//  LCDFreq Scaling default for up left
	.lcdfreq_kick_in_freq = LCD_FREQ_KICK_IN_FREQ,				//  LCDFreq Scaling default for kick in freq
	.lcdfreq_kick_in_cores = LCD_FREQ_KICK_IN_CORES,			//  LCDFreq Scaling default for kick in cores
#endif
	};

/**
 * Smooth scaling conservative governor (by Michael Weingaertner)
 *
 * This modification makes the governor use two lookup tables holding
 * current, next and previous frequency to directly get a correct
 * target frequency instead of calculating target frequencies with
 * up_threshold and step_up %. The two scaling lookup tables used
 * contain different scaling steps/frequencies to achieve faster upscaling
 * on higher CPU load.
 *
 * CPU load triggering faster upscaling can be adjusted via SYSFS,
 * VALUE between 1 and 100 (% CPU load):
 * echo VALUE > /sys/devices/system/cpu/cpufreq/impulse/smooth_up
 *
 * improved by Zane Zaminsky 2012/13
 */

#define MN_FREQ 0
#define MN_UP 1
#define MN_DOWN 2
#define freq_table_size (17)

/*
 *               - Table modified for use with Samsung I9300 by ZaneZam November 2012
 *               - table modified to reach overclocking frequencies up to 1600mhz
 *               - added fast scaling columns to frequency table
 *               - removed fast scaling colums and use line jumps instead. 4 steps and 2 modes (with/without fast downscaling) possible now
 *                table modified to reach overclocking frequencies up to 1800mhz
 *                fixed wrong frequency stepping
 *                added search limit for more efficent frequency searching and better hard/softlimit handling
 *               - combination of power and normal scaling table to only one array (idea by Yank555)
 *               - scaling logic reworked and optimized by Yank555
 */

static int mn_freqs[17][5]={
    {1800000,1800000,1700000,1800000,1700000},
    {1700000,1800000,1600000,1800000,1600000},
    {1600000,1700000,1500000,1800000,1500000},
    {1500000,1600000,1400000,1700000,1400000},
    {1400000,1500000,1300000,1600000,1300000},
    {1300000,1400000,1200000,1500000,1200000},
    {1200000,1300000,1100000,1400000,1100000},
    {1100000,1200000,1000000,1300000,1000000},
    {1000000,1100000, 900000,1200000, 900000},
    { 900000,1000000, 800000,1100000, 800000},
    { 800000, 900000, 700000,1000000, 700000},
    { 700000, 800000, 600000, 900000, 600000},
    { 600000, 700000, 400000, 800000, 500000},
    { 500000, 600000, 300000, 700000, 400000},
    { 400000, 500000, 200000, 600000, 300000},
    { 300000, 400000, 200000, 500000, 200000},
    { 200000, 300000, 200000, 400000, 200000}
};

static int mn_get_next_freq(unsigned int curfreq, unsigned int updown, unsigned int load) {

	int i=0;
	int target=0; //  Target column in freq. array
    
	if (load < dbs_tuners_ins.smooth_up) 	//  Decide what column to use as target
		target=updown+0;		//          normal scale columns
	else
		target=updown+2;		//           power scale columns

	for(i = max_scaling_freq_soft; i < freq_table_size; i++) {

		if(curfreq == mn_freqs[i][MN_FREQ]) {

			if(updown == MN_UP)	//  Scaling up
				return mn_freqs[max(              1, i - scaling_mode_up  )][target];	//  we don't want to jump out of the array if we do fs scaling
			else			//  Scaling down
				return mn_freqs[min(freq_table_size, i + scaling_mode_down)][target];	//  we don't want to jump out of the array if we do fs scaling

			return (curfreq);	//  We should never get here...
		}

	}
    
	return (curfreq); // not found

}

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

/* keep track of frequency transitions */
static int
dbs_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
		     void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpu_dbs_info_s *this_dbs_info = &per_cpu(cs_cpu_dbs_info,
							freq->cpu);

	struct cpufreq_policy *policy;

	if (!this_dbs_info->enable)
		return 0;

	policy = this_dbs_info->cur_policy;

	/*
	 * we only care if our internally tracked freq moves outside
	 * the 'valid' ranges of frequency available to us otherwise
	 * we do not change it
	*/
	if (this_dbs_info->requested_freq > policy->max
			|| this_dbs_info->requested_freq < policy->min)
		this_dbs_info->requested_freq = freq->new;

	return 0;
}

static struct notifier_block dbs_cpufreq_notifier_block = {
	.notifier_call = dbs_cpufreq_notifier
};

/************************** sysfs interface ************************/
static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_impulse Governor Tunables */
#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}
show_one(sampling_rate, sampling_rate);
show_one(sampling_rate_sleep_multiplier, sampling_rate_sleep_multiplier);	//  added sampling_rate_sleep_multiplier tuneable for early suspend
show_one(sampling_down_factor, sampling_down_factor);				//  reactivated sampling down factor
show_one(sampling_down_max_momentum, sampling_down_max_mom);			//  added Sampling down momentum tuneable
show_one(sampling_down_momentum_sensitivity, sampling_down_mom_sens);		//  added Sampling down momentum tuneable
show_one(up_threshold, up_threshold);
show_one(up_threshold_sleep, up_threshold_sleep);				//  added up_threshold_sleep tuneable for early suspend
show_one(up_threshold_hotplug1, up_threshold_hotplug1);				//  added up_threshold_hotplug1 tuneable for cpu1
show_one(up_threshold_hotplug2, up_threshold_hotplug2);				//  added up_threshold_hotplug2 tuneable for cpu2
show_one(up_threshold_hotplug3, up_threshold_hotplug3);				//  added up_threshold_hotplug3 tuneable for cpu3
show_one(down_threshold, down_threshold);
show_one(down_threshold_sleep, down_threshold_sleep);				//  added down_threshold_sleep tuneable for early suspend
show_one(down_threshold_hotplug1, down_threshold_hotplug1);			//  added down_threshold_hotplug1 tuneable for cpu1
show_one(down_threshold_hotplug2, down_threshold_hotplug2);			//  added down_threshold_hotplug2 tuneable for cpu2
show_one(down_threshold_hotplug3, down_threshold_hotplug3);			//  added down_threshold_hotplug3 tuneable for cpu3
show_one(ignore_nice_load, ignore_nice);
show_one(freq_step, freq_step);
show_one(freq_step_sleep, freq_step_sleep);					//  added freq_step_sleep tuneable for early suspend
show_one(smooth_up, smooth_up);
show_one(smooth_up_sleep, smooth_up_sleep);					//  added smooth_up_sleep tuneable for early suspend
show_one(hotplug_sleep, hotplug_sleep);						//  added hotplug_sleep tuneable for early suspend
show_one(freq_limit, freq_limit);						//  added freq_limit tuneable
show_one(freq_limit_sleep, freq_limit_sleep);					//  added freq_limit_sleep tuneable for early suspend
show_one(fast_scaling, fast_scaling);						//  added fast_scaling tuneable
show_one(fast_scaling_sleep, fast_scaling_sleep);				//  added fast_scaling_sleep tuneable for early suspend
show_one(grad_up_threshold, grad_up_threshold);					//  added Early demand tuneable grad up threshold
show_one(early_demand, early_demand);						//  added Early demand tuneable master switch
show_one(disable_hotplug, disable_hotplug);					//  added Hotplug switch
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
show_one(lcdfreq_enable, lcdfreq_enable);					//  added LCDFreq Scaling tuneable master switch
show_one(lcdfreq_kick_in_down_delay, lcdfreq_kick_in_down_delay);		//  added LCDFreq Scaling tuneable kick in down delay
show_one(lcdfreq_kick_in_up_delay, lcdfreq_kick_in_up_delay);			//  added LCDFreq Scaling tuneable kick in up delay
show_one(lcdfreq_kick_in_freq, lcdfreq_kick_in_freq);				//  added LCDFreq Scaling tuneable kick in freq
show_one(lcdfreq_kick_in_cores, lcdfreq_kick_in_cores);				//  added LCDFreq Scaling tuneable kick in cores
#endif

//  added tuneable for Sampling down momentum -> possible values: 0 (disable) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 0
static ssize_t store_sampling_down_max_momentum(struct kobject *a,
		struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR -
	dbs_tuners_ins.sampling_down_factor || input < 0)
	return -EINVAL;

	dbs_tuners_ins.sampling_down_max_mom = input;
	orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;

	/* Reset sampling down factor to default if momentum was disabled */
	if (dbs_tuners_ins.sampling_down_max_mom == 0)
	dbs_tuners_ins.sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;

	/* Reset momentum_adder and reset down sampling multiplier in case momentum was disabled */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	    if (dbs_tuners_ins.sampling_down_max_mom == 0)
	    dbs_info->rate_mult = 1;
	}

return count;
}

//  added tuneable for Sampling down momentum -> possible values: 1 to MAX_SAMPLING_DOWN_SENSITIVITY, if not set default is 50
static ssize_t store_sampling_down_momentum_sensitivity(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_MOMENTUM_SENSITIVITY || input < 1)
	return -EINVAL;
	
	dbs_tuners_ins.sampling_down_mom_sens = input;

	/* Reset momentum_adder */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->momentum_adder = 0;
	}

return count;
}

//  Sampling down factor (reactivated) added reset loop for momentum functionality -> possible values: 1 (disabled) to MAX_SAMPLING_DOWN_FACTOR, if not set default is 1
static ssize_t store_sampling_down_factor(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
	    struct cpu_dbs_info_s *dbs_info;
	    dbs_info = &per_cpu(cs_cpu_dbs_info, j);
	    dbs_info->rate_mult = 1;
	}

	return count;
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;
	
	dbs_tuners_ins.sampling_rate = max(input, min_sampling_rate);

	return count;
}

//  added tuneable -> possible values: 1 to 4, if not set default is 2
static ssize_t store_sampling_rate_sleep_multiplier(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_RATE_SLEEP_MULTIPLIER || input < 1)
		return -EINVAL;

	dbs_tuners_ins.sampling_rate_sleep_multiplier = input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 ||
			input <= dbs_tuners_ins.down_threshold)
		return -EINVAL;

	dbs_tuners_ins.up_threshold = input;
	return count;
}

//  added tuneble -> possible values: range from above down_threshold_sleep value up to 100, if not set default is 90
static ssize_t store_up_threshold_sleep(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 ||
			input <= dbs_tuners_ins.down_threshold_sleep)
		return -EINVAL;

	dbs_tuners_ins.up_threshold_sleep = input;
	return count;
}

//  added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug1(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug1 = input;
	return count;
}

//  added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug2(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug2 = input;
	return count;
}

//  added tuneable -> possible values: 0 to disable core, range from down_threshold up to 100, if not set default is 68
static ssize_t store_up_threshold_hotplug3(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || (input <= dbs_tuners_ins.down_threshold && input != 0))
		return -EINVAL;

	dbs_tuners_ins.up_threshold_hotplug3 = input;
	return count;
}

static ssize_t store_down_threshold(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold = input;
	return count;
}

//  added tuneable -> possible values: range from 11 to up_threshold_sleep but not up_threshold_sleep, if not set default is 44
static ssize_t store_down_threshold_sleep(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold_sleep)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_sleep = input;
	return count;
}

//  added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug1(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug1 = input;
	return count;
}

//  added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug2(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug2 = input;
	return count;
}

//  added tuneable -> possible values: range from 11 to up_threshold but not up_threshold, if not set default is 55
static ssize_t store_down_threshold_hotplug3(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	/* cannot be lower than 11 otherwise freq will not fall */
	if (ret != 1 || input < 11 || input > 100 ||
			input >= dbs_tuners_ins.up_threshold)
		return -EINVAL;

	dbs_tuners_ins.down_threshold_hotplug3 = input;
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(cs_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;

	}
	return count;
}

static ssize_t store_freq_step(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 100)
		input = 100;

	/* no need to test here if freq_step is zero as the user might actually
	 * want this, they would be crazy though :) */
	dbs_tuners_ins.freq_step = input;
	return count;
}

/*
 * added tuneable -> possible values: range from 0 to 100, if not set default is 5 -> value 0 will stop freq scaling and hold on actual freq
 * value 100 will directly jump up/down to limits like ondemand governor
 */
static ssize_t store_freq_step_sleep(struct kobject *a, struct attribute *b,
			       const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 100)
		input = 100;

	/* no need to test here if freq_step is zero as the user might actually
	 * want this, they would be crazy though :) */
	dbs_tuners_ins.freq_step_sleep = input;
	return count;
}

static ssize_t store_smooth_up(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1)
		return -EINVAL;

	dbs_tuners_ins.smooth_up = input;
	return count;
}

//  added tuneable -> possible values: range from 1 to 100, if not set default is 100
static ssize_t store_smooth_up_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 1)
		return -EINVAL;

	dbs_tuners_ins.smooth_up_sleep = input;
	return count;
}

/* 
 * added tuneable -> possible values: 0 do not touch the hotplug values on early suspend,
 * 1-3 equals cores to run at early suspend, if not set default is 0
 */
static ssize_t store_hotplug_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 3 || (input < 1 && input != 0))
		return -EINVAL;

	dbs_tuners_ins.hotplug_sleep = input;
	return count;
}

//  added tuneable -> possible values: 0 disable, 200000-1800000 khz -> freqency soft-limit, if not set default is 0
static ssize_t store_freq_limit(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;
	ret = sscanf(buf, "%u", &input);
	
	if (input == 0) {
	     max_scaling_freq_soft = max_scaling_freq_hard;
	     dbs_tuners_ins.freq_limit = input;
	     return count;
	}
	
        for (i=0; i < freq_table_size; i++) {
	    if (input == mn_freqs[i][MN_FREQ] && i > max_scaling_freq_hard) { 	//  check if we would go over scaling hard limit, if so drop input
		dbs_tuners_ins.freq_limit = input;			//  if we are under max hard limit accept input
		if (i > max_scaling_freq_soft) {			//  check if we have to adjust actual scaling range because we use a lower frequency now
		    max_scaling_freq_soft = i;				//  if so set it to new soft limit
		} else {
		    max_scaling_freq_soft = max_scaling_freq_hard;	//  else set it back to scaling hard limit
		}
	    return count;
	    }
	}
	return -EINVAL;
}

//  added tuneable -> possible values: 0 disable, 200000-1800000 khz -> freqency soft-limit on early suspend, if not set default is 0
static ssize_t store_freq_limit_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;
	ret = sscanf(buf, "%u", &input);

        for (i=0; i < freq_table_size; i++) {
	    if ((input == mn_freqs[i][MN_FREQ] && i > max_scaling_freq_hard) || input == 0) {
		dbs_tuners_ins.freq_limit_sleep = input;
	    return count;
	    }
	}
	return -EINVAL;
}

//  added tuneable -> possible values: 0 disable, 1-4 number of scaling jumps only for upscaling, 5-8 equivalent to 1-4 for up and down scaling, if not set default is 0
static ssize_t store_fast_scaling(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 8 || input < 0)
		return -EINVAL;

	dbs_tuners_ins.fast_scaling = input;

	if (input > 4) {
	    scaling_mode_up   = input - 4;	//  fast scaling up
	    scaling_mode_down = input - 4;	//  fast scaling down

	} else {
	    scaling_mode_up   = input;		//  fast scaling up only
	    scaling_mode_down = 0;
	}
	return count;
}

//  added tuneable -> possible values: 0 disable, 1-4 number of scaling jumps only for upscaling, 5-8 equivalent to 1-4 for up and down scaling, if not set default is 0
static ssize_t store_fast_scaling_sleep(struct kobject *a,
					  struct attribute *b,
					  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 8 || input < 0)
		return -EINVAL;

	dbs_tuners_ins.fast_scaling_sleep = input;
	return count;
}

//  LCDFreq Scaling - added tuneable (Master Switch) -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
static ssize_t store_lcdfreq_enable(struct kobject *a, struct attribute *b,
					    const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
	return -EINVAL;

	if (input > 0) {
		dbs_tuners_ins.lcdfreq_enable = true;
	}
	else {
		dbs_tuners_ins.lcdfreq_enable = false;
		// Set screen to 60Hz when stopping to switch
		lcdfreq_lock_current = 0;
		_lcdfreq_lock(lcdfreq_lock_current);
	}
	return count;
}

//  LCDFreq Scaling - added tuneable (Down Delay) -> number of samples to wait till switching to 40hz, possible range not specified, if not set default is 20
static ssize_t store_lcdfreq_kick_in_down_delay(struct kobject *a, struct attribute *b,
							const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1 && input < 0)
	return -EINVAL;

	dbs_tuners_ins.lcdfreq_kick_in_down_delay = input;
	dbs_tuners_ins.lcdfreq_kick_in_down_left =
	dbs_tuners_ins.lcdfreq_kick_in_down_delay;
	return count;
}

//  LCDFreq Scaling - added tuneable (Up Delay) -> number of samples to wait till switching to 40hz, possible range not specified, if not set default is 50
static ssize_t store_lcdfreq_kick_in_up_delay(struct kobject *a, struct attribute *b,
							const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1 && input < 0)
	return -EINVAL;

	dbs_tuners_ins.lcdfreq_kick_in_up_delay = input;
	dbs_tuners_ins.lcdfreq_kick_in_up_left =
	dbs_tuners_ins.lcdfreq_kick_in_up_delay;
	return count;
}

//  LCDFreq Scaling - added tuneable (Frequency Threshold) -> frequency from where to start switching LCDFreq, possible values are all valid frequencies up to actual scaling limit, if not set default is 500000
static ssize_t store_lcdfreq_kick_in_freq(struct kobject *a, struct attribute *b,
						    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;
	ret = sscanf(buf, "%u", &input);

        for (i=0; i < freq_table_size; i++) {
	    if ((input == mn_freqs[i][MN_FREQ] && i >= max_scaling_freq_hard) || input == 0) {
		dbs_tuners_ins.lcdfreq_kick_in_freq = input;
	    return count;
	    }
	}
	return -EINVAL;
}

//  LCDFreq Scaling - added tuneable (Online Cores Threshold) -> amount of cores which have to be online before switching LCDFreq (useable in combination with Freq Threshold), possible values 0-4, if not set default is 0
static ssize_t store_lcdfreq_kick_in_cores(struct kobject *a, struct attribute *b,
						    const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
	return -EINVAL;

	switch(input) {

		case 0  :
		case 1  :
		case 2  :
		case 3  :
		case 4  : dbs_tuners_ins.lcdfreq_kick_in_cores = input;
			  return count;
		default : return -EINVAL;
	}
}
#endif

//  Early demand - added tuneable grad up threshold -> possible values: from 11 to 100, if not set default is 50
static ssize_t store_grad_up_threshold(struct kobject *a, struct attribute *b,
						const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > 100 || input < 11)
	return -EINVAL;

	dbs_tuners_ins.grad_up_threshold = input;
	return count;
}

//  Early demand - added tuneable master switch -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_early_demand(struct kobject *a, struct attribute *b,
					    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
	return -EINVAL;
	
	dbs_tuners_ins.early_demand = !!input;
	return count;
}

//  added tuneable hotplug switch for debugging and testing purposes -> possible values: 0 to disable, any value above 0 to enable, if not set default is 0
static ssize_t store_disable_hotplug(struct kobject *a, struct attribute *b,
					    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	int i=0;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
	return -EINVAL;

	if (input > 0) {
		dbs_tuners_ins.disable_hotplug = true;
			for (i = 1; i < 4; i++) { 		//  enable all offline cores
			if (!cpu_online(i))
			cpu_up(i);
			}
	} else {
		dbs_tuners_ins.disable_hotplug = false;
	}
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(sampling_rate_sleep_multiplier);		//  added tuneable
define_one_global_rw(sampling_down_factor);			//  Sampling down factor (reactived)
define_one_global_rw(sampling_down_max_momentum);		//  Sampling down momentum tuneable
define_one_global_rw(sampling_down_momentum_sensitivity);	//  Sampling down momentum tuneable
define_one_global_rw(up_threshold);
define_one_global_rw(up_threshold_sleep); 			//  added tuneable
define_one_global_rw(up_threshold_hotplug1);			//  added tuneable
define_one_global_rw(up_threshold_hotplug2);			//  added tuneable
define_one_global_rw(up_threshold_hotplug3);			//  added tuneable
define_one_global_rw(down_threshold);
define_one_global_rw(down_threshold_sleep);			//  added tuneable
define_one_global_rw(down_threshold_hotplug1);			//  added tuneable
define_one_global_rw(down_threshold_hotplug2);			//  added tuneable
define_one_global_rw(down_threshold_hotplug3);			//  added tuneable
define_one_global_rw(ignore_nice_load);
define_one_global_rw(freq_step);
define_one_global_rw(freq_step_sleep);				//  added tuneable
define_one_global_rw(smooth_up);
define_one_global_rw(smooth_up_sleep);				//  added tuneable
define_one_global_rw(hotplug_sleep);				//  added tuneable
define_one_global_rw(freq_limit);				//  added tuneable
define_one_global_rw(freq_limit_sleep);				//  added tuneable
define_one_global_rw(fast_scaling);				//  added tuneable
define_one_global_rw(fast_scaling_sleep);			//  added tuneable
define_one_global_rw(grad_up_threshold);			//  Early demand tuneable
define_one_global_rw(early_demand);				//  Early demand tuneable
define_one_global_rw(disable_hotplug);				//  Hotplug switch
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
define_one_global_rw(lcdfreq_enable);				//  LCDFreq Scaling tuneable
define_one_global_rw(lcdfreq_kick_in_down_delay);		//  LCDFreq Scaling tuneable
define_one_global_rw(lcdfreq_kick_in_up_delay);			//  LCDFreq Scaling tuneable
define_one_global_rw(lcdfreq_kick_in_freq);			//  LCDFreq Scaling tuneable
define_one_global_rw(lcdfreq_kick_in_cores);			//  LCDFreq Scaling tuneable
#endif
static struct attribute *dbs_attributes[] = {
	&sampling_rate_min.attr,
	&sampling_rate.attr,
	&sampling_rate_sleep_multiplier.attr,			//  added tuneable
	&sampling_down_factor.attr,
	&sampling_down_max_momentum.attr,			//  Sampling down momentum tuneable
	&sampling_down_momentum_sensitivity.attr,		//  Sampling down momentum tuneable
	&up_threshold_hotplug1.attr,				//  added tuneable
	&up_threshold_hotplug2.attr,				//  added tuneable
	&up_threshold_hotplug3.attr,				//  added tuneable
	&down_threshold.attr,
	&down_threshold_sleep.attr,				//  added tuneable
	&down_threshold_hotplug1.attr,				//  added tuneable
	&down_threshold_hotplug2.attr,				//  added tuneable
	&down_threshold_hotplug3.attr,				//  added tuneable
	&ignore_nice_load.attr,
	&freq_step.attr,
	&freq_step_sleep.attr,					//  added tuneable
	&smooth_up.attr,
	&smooth_up_sleep.attr,					//  added tuneable
	&up_threshold.attr,
	&up_threshold_sleep.attr,				//  added tuneable
	&hotplug_sleep.attr,					//  added tuneable
	&freq_limit.attr,					//  added tuneable
	&freq_limit_sleep.attr,					//  added tuneable
	&fast_scaling.attr,					//  added tuneable
	&fast_scaling_sleep.attr,				//  added tuneable
	&grad_up_threshold.attr,				//  Early demand tuneable
	&early_demand.attr,					//  Early demand tuneable
	&disable_hotplug.attr,					//  Hotplug switch
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	&lcdfreq_enable.attr,					//  LCD Freq Scaling tuneable
	&lcdfreq_kick_in_down_delay.attr,			//  LCD Freq Scaling tuneable
	&lcdfreq_kick_in_up_delay.attr,				//  LCD Freq Scaling tuneable
	&lcdfreq_kick_in_freq.attr,				//  LCD Freq Scaling tuneable
	&lcdfreq_kick_in_cores.attr,				//  LCD Freq Scaling tuneable
#endif
	NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "impulse",
};

/************************** sysfs end ************************/

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int load = 0;
	unsigned int max_load = 0;
	int boost_freq = 0; 					//  Early demand boost freq switch
	struct cpufreq_policy *policy;
	unsigned int j;
	unsigned int switch_core = 0;				//  Hotplugging core switch
	int i=0;
	
	policy = this_dbs_info->cur_policy;

	/*
	 * Frequency Limit: we try here at a verly early stage to limit freqencies above limit by setting the current target_freq to freq_limit.
	 * This could be for example wakeup or touchboot freqencies which could be above the limit and are out of governors control.
	 * This can not avoid the incrementation to these frequencies but should bring it down again earlier. Not sure if that is
	 * a good way to do that or if its realy working. Just an idea - maybe a remove-candidate!
	 */
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit)
	__cpufreq_driver_target(policy, dbs_tuners_ins.freq_limit, CPUFREQ_RELATION_L);

	/*
	 * Every sampling_rate, we check, if current idle time is less than 20%
	 * (default), then we try to increase frequency. Every sampling_rate *
	 * sampling_down_factor, we check, if current idle time is more than 80%
	 * (default), then we try to decrease frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of maximum frequency
	 */

	/* Get Absolute Load */
	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time;
		unsigned int idle_time, wall_time;

		j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);

		wall_time = (unsigned int) cputime64_sub(cur_wall_time,
				j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int) cputime64_sub(cur_idle_time,
				j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		if (dbs_tuners_ins.ignore_nice) {
			cputime64_t cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = cputime64_sub(kstat_cpu(j).cpustat.nice,
					 j_dbs_info->prev_cpu_nice);
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kstat_cpu(j).cpustat.nice;
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		if (load > max_load)
			max_load = load;
	/*
	 * Early demand by stratosk
	 * Calculate the gradient of load_freq. If it is too steep we assume
	 * that the load will go over up_threshold in next iteration(s) and
	 * we increase the frequency immediately
	 */
	if (dbs_tuners_ins.early_demand) {
               if (max_load > this_dbs_info->prev_load &&
               (max_load - this_dbs_info->prev_load >
               dbs_tuners_ins.grad_up_threshold))
                  boost_freq = 1;
           this_dbs_info->prev_load = max_load;
           }
	}

	/*
	 * reduction of possible deadlocks - we try here to avoid deadlocks due to double locking from hotplugging and timer mutex
	 * during start/stop/limit events. to be "sure" we skip here 25 times till the locks hopefully are unlocked again. yeah that's dirty
	 * but no better way found yet! ;)
	 */
	if (this_dbs_info->check_cpu_skip != 0) {
		++this_dbs_info->check_cpu_skip;
		if (this_dbs_info->check_cpu_skip >= 25)
		    this_dbs_info->check_cpu_skip = 0;
		return;
	}
	
	/*
	 * break out if we 'cannot' reduce the speed as the user might
	 * want freq_step to be zero
	 */
	if (dbs_tuners_ins.freq_step == 0)
		return;
	
	/*
	 *              - Modification by ZaneZam November 2012
	 *                Check for frequency increase is greater than hotplug up threshold value and wake up cores accordingly
	 *                Following will bring up 3 cores in a row (cpu0 stays always on!)
	 *
	 *              - changed hotplug logic to be able to tune up threshold per core and to be able to set
	 *                cores offline manually via sysfs
	 *
	 *              - fixed non switching cores at 0+2 and 0+3 situations
	 *              - optimized hotplug logic by removing locks and skipping hotplugging if not needed
	 *              - try to avoid deadlocks at critical events by using a flag if we are in the middle of hotplug decision
	 *
	 *              - optimised hotplug logic by reducing code and concentrating only on essential parts
	 *              - preperation for automatic core detection
	 */

	if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != num_possible_cpus()) {
	switch_core = 0;
	for (i = 1; i < num_possible_cpus(); i++) {
		    if (skip_hotplug_flag == 0) {
			if (i == 1 && dbs_tuners_ins.up_threshold_hotplug1 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug1) {
			    switch_core = 1;
			} else if (i == 2 && dbs_tuners_ins.up_threshold_hotplug2 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug2) {
			    switch_core = 2;
			} else if (i == 3 && dbs_tuners_ins.up_threshold_hotplug3 != 0 && max_load > dbs_tuners_ins.up_threshold_hotplug3) {
			    switch_core = 3;
			}
		    if (!cpu_online(switch_core) && switch_core != 0 && skip_hotplug_flag == 0)
		    cpu_up(switch_core);
		    }
	    }
	}
	
	/* Check for frequency increase */
	if (max_load > dbs_tuners_ins.up_threshold || boost_freq) { //  Early demand - added boost switch
	
	    /* Sampling down momentum - if momentum is inactive switch to "down_skip" method */
	    if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1)
		this_dbs_info->down_skip = 0;

	    /* if we are already at full speed then break out early */
	    if (policy->cur == policy->max) //  changed check from reqested_freq to current freq (DerTeufel1980)
		return;

	    /* Sampling down momentum - if momentum is active and we are switching to max speed, apply sampling_down_factor */
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && policy->cur < policy->max)
		this_dbs_info->rate_mult = dbs_tuners_ins.sampling_down_factor;

	    /* Frequency Limit: if we are at freq_limit break out early */
	    if (dbs_tuners_ins.freq_limit != 0 && policy->cur == dbs_tuners_ins.freq_limit)
		return;

	    /* Frequency Limit: try to strictly hold down freqency at freq_limit by spoofing requested freq */
	    if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit) {
		this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

	    /* check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
	    if (this_dbs_info->requested_freq > policy->max)
		 this_dbs_info->requested_freq = policy->max;
	
	    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);

		    /* Sampling down momentum - calculate momentum and update sampling down factor */
		    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
			this_dbs_info->momentum_adder++;
			dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
			dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
		    }
		    return;

	    /* Frequency Limit: but let it scale up as normal if the freqencies are lower freq_limit */
	    } else if (dbs_tuners_ins.freq_limit != 0 && policy->cur < dbs_tuners_ins.freq_limit) {
			this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_UP, max_load);
	
		    /* check again if we are above limit because of fast scaling */
		    if (dbs_tuners_ins.freq_limit != 0 && this_dbs_info->requested_freq > dbs_tuners_ins.freq_limit)
			this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;

		    /* check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
		    if (this_dbs_info->requested_freq > policy->max)
		          this_dbs_info->requested_freq = policy->max;

		    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
				CPUFREQ_RELATION_H);

		    /* Sampling down momentum - calculate momentum and update sampling down factor */
		    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
			this_dbs_info->momentum_adder++; 
			dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
			dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
		    }
		return;
	    }
	
	    this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_UP, max_load);

	    /* check if requested freq is higher than max freq if so bring it down to max freq (DerTeufel1980) */
	    if (this_dbs_info->requested_freq > policy->max)
		 this_dbs_info->requested_freq = policy->max;

	    __cpufreq_driver_target(policy, this_dbs_info->requested_freq,
			CPUFREQ_RELATION_H);

	    /* Sampling down momentum - calculate momentum and update sampling down factor */
	    if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder < dbs_tuners_ins.sampling_down_mom_sens) {
		this_dbs_info->momentum_adder++; 
		dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
		dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
	    }
	return;
	}

#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	if(dbs_tuners_ins.lcdfreq_enable) {

		//  LCDFreq Scaling delays
		if( (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // No core threshold, only check freq. threshold
		     dbs_tuners_ins.lcdfreq_kick_in_cores == 0                                ) ||

		    (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // Core threshold reached, check freq. threshold
		     dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&
		     dbs_tuners_ins.lcdfreq_kick_in_cores == num_online_cpus()                ) ||

		    (dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&      // Core threshold passed, no need to check freq. threshold
		     dbs_tuners_ins.lcdfreq_kick_in_cores <  num_online_cpus()                )
		                                                                                ) {

			// We are above threshold, reset down delay, decrement up delay
			if(dbs_tuners_ins.lcdfreq_kick_in_down_left != dbs_tuners_ins.lcdfreq_kick_in_down_delay)
				dbs_tuners_ins.lcdfreq_kick_in_down_left = dbs_tuners_ins.lcdfreq_kick_in_down_delay;
			dbs_tuners_ins.lcdfreq_kick_in_up_left--;

		} else {

			// We are below threshold, reset up delay, decrement down delay
			if(dbs_tuners_ins.lcdfreq_kick_in_up_left != dbs_tuners_ins.lcdfreq_kick_in_up_delay)
				dbs_tuners_ins.lcdfreq_kick_in_up_left = dbs_tuners_ins.lcdfreq_kick_in_up_delay;
			dbs_tuners_ins.lcdfreq_kick_in_down_left--;

		}

		//  LCDFreq Scaling set frequency if needed
		if(dbs_tuners_ins.lcdfreq_kick_in_up_left <= 0 && lcdfreq_lock_current != 0) {

			// We reached up delay, set frequency to 60Hz
			lcdfreq_lock_current = 0;
			_lcdfreq_lock(lcdfreq_lock_current);

		} else if(dbs_tuners_ins.lcdfreq_kick_in_down_left <= 0 && lcdfreq_lock_current != 1) {

			// We reached down delay, set frequency to 40Hz
			lcdfreq_lock_current = 1;
			_lcdfreq_lock(lcdfreq_lock_current);

		}
	}
#endif
	/*
	 *              - Modification by ZaneZam November 2012
	 *                Check for frequency decrease is lower than hotplug value and put cores to sleep accordingly
	 *                Following will disable 3 cores in a row (cpu0 is always on!)
	 *
	 *              - changed logic to be able to tune down threshold per core via sysfs
	 *
	 *              - fixed non switching cores at 0+2 and 0+3 situations
	 *              - optimized hotplug logic by removing locks and skipping hotplugging if not needed
	 *              - try to avoid deadlocks at critical events by using a flag if we are in the middle of hotplug decision
	 *
	 *              - optimized hotplug logic by reducing code and concentrating only on essential parts
	 *              - preperation for automatic core detection
	 */

	if (!dbs_tuners_ins.disable_hotplug && skip_hotplug_flag == 0 && num_online_cpus() != 1) {
	switch_core = 0;
	for (i = 1; i < num_possible_cpus(); i++) {
		if (skip_hotplug_flag == 0) {
		    if (i == 3 && max_load < dbs_tuners_ins.down_threshold_hotplug3) {
			switch_core = 3;
		    } else if (i == 2 && max_load < dbs_tuners_ins.down_threshold_hotplug2) {
			switch_core = 2;
		    } else if (i == 1 && max_load < dbs_tuners_ins.down_threshold_hotplug1) {
			switch_core = 1;
		    }
		if (cpu_online(switch_core) && switch_core != 0 && skip_hotplug_flag == 0)
		cpu_down(switch_core);
		}
	    }
	}

	/* Sampling down momentum - if momentum is inactive switch to down skip method and if sampling_down_factor is active break out early */
	if (dbs_tuners_ins.sampling_down_max_mom == 0 && dbs_tuners_ins.sampling_down_factor > 1) {
	    if (++this_dbs_info->down_skip < dbs_tuners_ins.sampling_down_factor)
		return;
	this_dbs_info->down_skip = 0;
	}
	
	/* Sampling down momentum - calculate momentum and update sampling down factor */
	if (dbs_tuners_ins.sampling_down_max_mom != 0 && this_dbs_info->momentum_adder > 1) {
	this_dbs_info->momentum_adder -= 2;
	dbs_tuners_ins.sampling_down_momentum = (this_dbs_info->momentum_adder * dbs_tuners_ins.sampling_down_max_mom) / dbs_tuners_ins.sampling_down_mom_sens;
	dbs_tuners_ins.sampling_down_factor = orig_sampling_down_factor + dbs_tuners_ins.sampling_down_momentum;
	}

	 /* Check for frequency decrease */
	if (max_load < dbs_tuners_ins.down_threshold) {

	    /* Sampling down momentum - No longer fully busy, reset rate_mult */
	    this_dbs_info->rate_mult = 1;
		
		/* if we cannot reduce the frequency anymore, break out early */
		if (policy->cur == policy->min)
			return;

	/* Frequency Limit: this should bring down freqency faster if we are coming from above limit (eg. touchboost/wakeup freqencies)*/ 
	if (dbs_tuners_ins.freq_limit != 0 && policy->cur > dbs_tuners_ins.freq_limit) {
		this_dbs_info->requested_freq = dbs_tuners_ins.freq_limit;
	
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
		if(dbs_tuners_ins.lcdfreq_enable) {

			//  LCDFreq Scaling delays
			if( (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // No core threshold, only check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores == 0                                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // Core threshold reached, check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&
			     dbs_tuners_ins.lcdfreq_kick_in_cores == num_online_cpus()                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&      // Core threshold passed, no need to check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores <  num_online_cpus()                )
			                                                                                   ) {

				// We are above threshold, reset down delay, decrement up delay
				if(dbs_tuners_ins.lcdfreq_kick_in_down_left != dbs_tuners_ins.lcdfreq_kick_in_down_delay)
					dbs_tuners_ins.lcdfreq_kick_in_down_left = dbs_tuners_ins.lcdfreq_kick_in_down_delay;
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left > 0)
				dbs_tuners_ins.lcdfreq_kick_in_up_left--;

			} else {

				// We are below threshold, reset up delay, decrement down delay
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left != dbs_tuners_ins.lcdfreq_kick_in_up_delay)
					dbs_tuners_ins.lcdfreq_kick_in_up_left = dbs_tuners_ins.lcdfreq_kick_in_up_delay;
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left > 0)
				dbs_tuners_ins.lcdfreq_kick_in_down_left--;

			}

			//  LCDFreq Scaling set frequency if needed
			if(dbs_tuners_ins.lcdfreq_kick_in_up_left <= 0 && lcdfreq_lock_current != 0) {

				// We reached up delay, set frequency to 60Hz
				lcdfreq_lock_current = 0;
				_lcdfreq_lock(lcdfreq_lock_current);

			} else if(dbs_tuners_ins.lcdfreq_kick_in_down_left <= 0 && lcdfreq_lock_current != 1) {

				// We reached down delay, set frequency to 40Hz
				lcdfreq_lock_current = 1;
				_lcdfreq_lock(lcdfreq_lock_current);

			}
		}
#endif
		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_L);
		return;

	/* Frequency Limit: else we scale down as usual */
	} else if (dbs_tuners_ins.freq_limit != 0 && policy->cur <= dbs_tuners_ins.freq_limit) {
		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_DOWN, max_load);

#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
		if(dbs_tuners_ins.lcdfreq_enable) {

			//  LCDFreq Scaling delays
			if( (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // No core threshold, only check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores == 0                                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // Core threshold reached, check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&
			     dbs_tuners_ins.lcdfreq_kick_in_cores == num_online_cpus()                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&      // Core threshold passed, no need to check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores <  num_online_cpus()                )
			                                                                                   ) {

				// We are above threshold, reset down delay, decrement up delay
				if(dbs_tuners_ins.lcdfreq_kick_in_down_left != dbs_tuners_ins.lcdfreq_kick_in_down_delay)
					dbs_tuners_ins.lcdfreq_kick_in_down_left = dbs_tuners_ins.lcdfreq_kick_in_down_delay;
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left > 0)
				dbs_tuners_ins.lcdfreq_kick_in_up_left--;

			} else {

				// We are below threshold, reset up delay, decrement down delay
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left != dbs_tuners_ins.lcdfreq_kick_in_up_delay)
					dbs_tuners_ins.lcdfreq_kick_in_up_left = dbs_tuners_ins.lcdfreq_kick_in_up_delay;
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left > 0)
				dbs_tuners_ins.lcdfreq_kick_in_down_left--;

			}

			//  LCDFreq Scaling set frequency if needed
			if(dbs_tuners_ins.lcdfreq_kick_in_up_left <= 0 && lcdfreq_lock_current != 0) {

				// We reached up delay, set frequency to 60Hz
				lcdfreq_lock_current = 0;
				_lcdfreq_lock(lcdfreq_lock_current);

			} else if(dbs_tuners_ins.lcdfreq_kick_in_down_left <= 0 && lcdfreq_lock_current != 1) {

				// We reached down delay, set frequency to 40Hz
				lcdfreq_lock_current = 1;
				_lcdfreq_lock(lcdfreq_lock_current);

			}
		}
#endif
		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_L); //  changed to relation low 
		return;
	}

    		this_dbs_info->requested_freq = mn_get_next_freq(policy->cur, MN_DOWN, max_load);

#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
		if(dbs_tuners_ins.lcdfreq_enable) {

			//  LCDFreq Scaling delays
			if( (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // No core threshold, only check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores == 0                                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_freq  <= this_dbs_info->requested_freq &&      // Core threshold reached, check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&
			     dbs_tuners_ins.lcdfreq_kick_in_cores == num_online_cpus()                ) ||

			    (dbs_tuners_ins.lcdfreq_kick_in_cores != 0                             &&      // Core threshold passed, no need to check freq. threshold
			     dbs_tuners_ins.lcdfreq_kick_in_cores <  num_online_cpus()                )
			                                                                                   ) {

				// We are above threshold, reset down delay, decrement up delay
				if(dbs_tuners_ins.lcdfreq_kick_in_down_left != dbs_tuners_ins.lcdfreq_kick_in_down_delay)
					dbs_tuners_ins.lcdfreq_kick_in_down_left = dbs_tuners_ins.lcdfreq_kick_in_down_delay;
				dbs_tuners_ins.lcdfreq_kick_in_up_left--;

			} else {

				// We are below threshold, reset up delay, decrement down delay
				if(dbs_tuners_ins.lcdfreq_kick_in_up_left != dbs_tuners_ins.lcdfreq_kick_in_up_delay)
					dbs_tuners_ins.lcdfreq_kick_in_up_left = dbs_tuners_ins.lcdfreq_kick_in_up_delay;
				dbs_tuners_ins.lcdfreq_kick_in_down_left--;

			}

			//  LCDFreq Scaling set frequency if needed
			if(dbs_tuners_ins.lcdfreq_kick_in_up_left <= 0 && lcdfreq_lock_current != 0) {

				// We reached up delay, set frequency to 60Hz
				lcdfreq_lock_current = 0;
				_lcdfreq_lock(lcdfreq_lock_current);

			} else if(dbs_tuners_ins.lcdfreq_kick_in_down_left <= 0 && lcdfreq_lock_current != 1) {

				// We reached down delay, set frequency to 40Hz
				lcdfreq_lock_current = 1;
				_lcdfreq_lock(lcdfreq_lock_current);

			}
		}
#endif
		__cpufreq_driver_target(policy, this_dbs_info->requested_freq,
					CPUFREQ_RELATION_L); //  changed to relation low
		return;
	}
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;

	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate * dbs_info->rate_mult); //  Sampling down momentum - added multiplier

	delay -= jiffies % delay;

	mutex_lock(&dbs_info->timer_mutex);

	dbs_check_cpu(dbs_info);

	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate); 
	delay -= jiffies % delay;

	dbs_info->enable = 1;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	dbs_info->enable = 0;
	cancel_delayed_work(&dbs_info->work); //  changed to asyncron cancel to fix deadlock on govenor stop
}

static void powersave_early_suspend(struct early_suspend *handler)
{
  int i=0;
  skip_hotplug_flag = 1; 						//  try to avoid deadlock by disabling hotplugging if we are in the middle of hotplugging logic
  suspend_flag = 1; 							//  we want to know if we are at suspend because of things that shouldn't be executed at suspend
  for (i = 0; i < 1000; i++);						//  wait a few samples to be sure hotplugging is off (never be sure so this is dirty)

  mutex_lock(&dbs_mutex);
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	prev_lcdfreq_enable = dbs_tuners_ins.lcdfreq_enable; 		//  LCDFreq Scaling - store state
	prev_lcdfreq_lock_current = lcdfreq_lock_current;		//  LCDFreq Scaling - store lock current
	if(dbs_tuners_ins.lcdfreq_enable) { 				//  LCDFreq Scaling - reset display freq. to 60Hz only if it was enabled
		dbs_tuners_ins.lcdfreq_enable = false;
		lcdfreq_lock_current = 0;
		_lcdfreq_lock(lcdfreq_lock_current);
	}
#endif
  sampling_rate_awake = dbs_tuners_ins.sampling_rate;
  up_threshold_awake = dbs_tuners_ins.up_threshold;
  down_threshold_awake = dbs_tuners_ins.down_threshold;
  dbs_tuners_ins.sampling_down_max_mom = 0;				//  Sampling down momentum - disabled at suspend
  smooth_up_awake = dbs_tuners_ins.smooth_up;
  freq_step_awake = dbs_tuners_ins.freq_step;				//  save freq step
  freq_limit_awake = dbs_tuners_ins.freq_limit;				//  save freq limit
  fast_scaling_awake = dbs_tuners_ins.fast_scaling;			//  save scaling setting

  if (dbs_tuners_ins.hotplug_sleep != 0) {				//  if set to 0 do not touch hotplugging values
	hotplug1_awake = dbs_tuners_ins.up_threshold_hotplug1;		//  save hotplug1 value for restore on awake
	hotplug2_awake = dbs_tuners_ins.up_threshold_hotplug2;		//  save hotplug2 value for restore on awake
	hotplug3_awake = dbs_tuners_ins.up_threshold_hotplug3;		//  save hotplug3 value for restore on awake
  }

  sampling_rate_asleep = dbs_tuners_ins.sampling_rate_sleep_multiplier; //  save sleep multiplier
  up_threshold_asleep = dbs_tuners_ins.up_threshold_sleep;		//  save up threshold
  down_threshold_asleep = dbs_tuners_ins.down_threshold_sleep;		//  save down threshold
  smooth_up_asleep = dbs_tuners_ins.smooth_up_sleep;			//  save smooth up
  freq_step_asleep = dbs_tuners_ins.freq_step_sleep;			//  save frequency step
  freq_limit_asleep = dbs_tuners_ins.freq_limit_sleep;			//  save frequency limit
  fast_scaling_asleep = dbs_tuners_ins.fast_scaling_sleep;		//  save fast scaling
  dbs_tuners_ins.sampling_rate *= sampling_rate_asleep;			//  set sampling rate
  dbs_tuners_ins.up_threshold = up_threshold_asleep;			//  set up threshold
  dbs_tuners_ins.down_threshold = down_threshold_asleep;		//  set down threshold
  dbs_tuners_ins.smooth_up = smooth_up_asleep;				//  set smooth up
  dbs_tuners_ins.freq_step = freq_step_asleep;				//  set freqency step
  dbs_tuners_ins.freq_limit = freq_limit_asleep;			//  set freqency limit
  dbs_tuners_ins.fast_scaling = fast_scaling_asleep;			//  set fast scaling

	if (dbs_tuners_ins.fast_scaling > 4) {				//  set scaling mode
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling - 4;	//  fast scaling up
	    scaling_mode_down = dbs_tuners_ins.fast_scaling - 4;	//  fast scaling down
	} else {
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling;		//  fast scaling up only
	    scaling_mode_down = 0;					//  fast scaling down
	}

	for (i=0; i < freq_table_size; i++) {
	if (freq_limit_asleep == mn_freqs[i][MN_FREQ] || freq_limit_asleep == 0) { 	//  check sleep frequency
	    if (max_scaling_freq_soft < max_scaling_freq_hard) { 		//  if the scaling soft value at sleep is lower (freq is higher) than sclaing hard value
		max_scaling_freq_soft = max_scaling_freq_hard; 			//  bring it down to scaling hard value as we cannot be over max hard scaling
		break;
	    } else if (max_scaling_freq_soft > max_scaling_freq_hard && dbs_tuners_ins.freq_limit == 0) { //  if the value is higher (freq is lower) than scaling hard value and no soft limit is active
		max_scaling_freq_soft = max_scaling_freq_hard; 			//  we also have to bring it to maximal hard scaling value
		break;
	    } else {
		max_scaling_freq_soft = i; 					//  else we can set it to actual limit number
		break;
	    }
	}
  }

  if (dbs_tuners_ins.hotplug_sleep != 0) {				//  if set to 0 do not touch hotplugging values
	if (dbs_tuners_ins.hotplug_sleep == 1) {			
	    dbs_tuners_ins.up_threshold_hotplug1 = 0;			//  set to one core
    	    dbs_tuners_ins.up_threshold_hotplug2 = 0;			//  set to one core
    	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			//  set to one core
	}
	if (dbs_tuners_ins.hotplug_sleep == 2) {			
	    dbs_tuners_ins.up_threshold_hotplug2 = 0;			//  set to two cores
	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			//  set to two cores
	} 
	if (dbs_tuners_ins.hotplug_sleep == 3) {			
	    dbs_tuners_ins.up_threshold_hotplug3 = 0;			//  set to three cores
	}
  }

  mutex_unlock(&dbs_mutex);
  for (i = 0; i < 1000; i++);						//  wait a few samples to be sure hotplugging is off (never be sure so this is dirty)
  skip_hotplug_flag = 0; 						//  enable hotplugging again

}

static void powersave_late_resume(struct early_suspend *handler)
{
  int i=0;
  skip_hotplug_flag = 1; 						//  same as above skip hotplugging to avoid deadlocks
  suspend_flag = 0; 							//  we are resuming so reset supend flag

      for (i = 1; i < 4; i++) { 					//  enable offline cores to avoid stuttering after resume if hotplugging limit was active
      if (!cpu_online(i))
      cpu_up(i);
      }

  for (i = 0; i < 1000; i++);  						//  wait a few samples to be sure hotplugging is off (never be sure so this is dirty)

 mutex_lock(&dbs_mutex);
#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
	dbs_tuners_ins.lcdfreq_enable = prev_lcdfreq_enable;		//  LCDFreq Scaling - enable it again if it was enabled
	if(dbs_tuners_ins.lcdfreq_enable) { 				//  LCDFreq Scaling - restore display freq. only if it was enabled before suspend
		lcdfreq_lock_current = prev_lcdfreq_lock_current;
		_lcdfreq_lock(lcdfreq_lock_current);
	}
#endif

   if (dbs_tuners_ins.hotplug_sleep != 0) {
	dbs_tuners_ins.up_threshold_hotplug1 = hotplug1_awake;		//  restore previous settings
	dbs_tuners_ins.up_threshold_hotplug2 = hotplug2_awake;		//  restore previous settings
	dbs_tuners_ins.up_threshold_hotplug3 = hotplug3_awake;		//  restore previous settings
  }

  dbs_tuners_ins.sampling_down_max_mom = orig_sampling_down_max_mom;	//  Sampling down momentum - restore max value
  dbs_tuners_ins.sampling_rate = sampling_rate_awake;			//  restore previous settings
  dbs_tuners_ins.up_threshold = up_threshold_awake;			//  restore previous settings
  dbs_tuners_ins.down_threshold = down_threshold_awake;			//  restore previous settings
  dbs_tuners_ins.smooth_up = smooth_up_awake;				//  restore previous settings
  dbs_tuners_ins.freq_step = freq_step_awake;				//  restore previous settings
  dbs_tuners_ins.freq_limit = freq_limit_awake;				//  restore previous settings
  dbs_tuners_ins.fast_scaling = fast_scaling_awake;			//  restore previous settings

	if (dbs_tuners_ins.fast_scaling > 4) {				//  set scaling mode
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling - 4;	//  fast scaling up
	    scaling_mode_down = dbs_tuners_ins.fast_scaling - 4;	//  fast scaling down
	} else {
	    scaling_mode_up   = dbs_tuners_ins.fast_scaling;		//  fast scaling up only
	    scaling_mode_down = 0;					//  fast scaling down
	}
	
	for (i=0; i < freq_table_size; i++) { 
	if (freq_limit_awake == mn_freqs[i][MN_FREQ] || freq_limit_awake == 0) { 
	    if (max_scaling_freq_soft < max_scaling_freq_hard) {	//  the same as at suspend we have to check if limit is active and over hard limit
		max_scaling_freq_soft = max_scaling_freq_hard;		//  and if not we have to change back the scaling value to max hard value
		break;
	    } else if (max_scaling_freq_soft > max_scaling_freq_hard && dbs_tuners_ins.freq_limit == 0) {
		max_scaling_freq_soft = max_scaling_freq_hard;
		break;
	    } else {
		max_scaling_freq_soft = i;
		break;
	    }
	}
  }
  mutex_unlock(&dbs_mutex);
  for (i = 0; i < 1000; i++);						//  wait a few samples to be sure hotplugging is off (never be sure so this is dirty)
  skip_hotplug_flag = 0; 						//  enable hotplugging again
}

static struct early_suspend _powersave_early_suspend = {
  .suspend = powersave_early_suspend,
  .resume = powersave_late_resume,
  .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,
};

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;
	int i=0;
	
	this_dbs_info = &per_cpu(cs_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(cs_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice) {
				j_dbs_info->prev_cpu_nice =
						kstat_cpu(j).cpustat.nice;
			}
			j_dbs_info->time_in_idle = get_cpu_idle_time_us(cpu, &j_dbs_info->idle_exit_time); //  added idle exit time handling
		}
		this_dbs_info->cpu = cpu; 		//  Initialise the cpu field during conservative governor start (https://github.com/ktoonsez/KT747-JB/commit/298dd04a610a6a655d7b77f320198d9f6c7565b6)
		this_dbs_info->rate_mult = 1; 		//  Sampling down momentum - reset multiplier
		this_dbs_info->momentum_adder = 0; 	//  Sampling down momentum - reset momentum adder
		this_dbs_info->down_skip = 0;		//  Sampling down - reset down_skip
		this_dbs_info->check_cpu_skip = 1;	//  we do not want to crash because of hotplugging so we start without it by skipping check_cpu
		this_dbs_info->requested_freq = policy->cur;
		max_scaling_freq_hard = 0;		//  set freq scaling start point to 0 (all frequencies up to table max)
		max_scaling_freq_soft = 0;		//  set freq scaling start point to 0 (all frequencies up to table max)
		
		//  initialisation of freq search in scaling table
		for (i=0; i < freq_table_size; i++) {
		if (policy->max == mn_freqs[i][MN_FREQ]) {
		    max_scaling_freq_hard = i; //  init hard value
		    max_scaling_freq_soft = i; //  init soft value
		    break;
		    }
		}

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_enable++;
		
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;
			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/*
			 * conservative does not implement micro like ondemand
			 * governor, thus we are bound to jiffes/HZ
			 */
			min_sampling_rate =
				MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(3);
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			orig_sampling_down_factor = dbs_tuners_ins.sampling_down_factor;	//  Sampling down momentum - set down factor
			orig_sampling_down_max_mom = dbs_tuners_ins.sampling_down_max_mom;	//  Sampling down momentum - set max momentum
			sampling_rate_awake = dbs_tuners_ins.sampling_rate;
			up_threshold_awake = dbs_tuners_ins.up_threshold;
			down_threshold_awake = dbs_tuners_ins.down_threshold;
			smooth_up_awake = dbs_tuners_ins.smooth_up;
			cpufreq_register_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);
		}
		mutex_unlock(&dbs_mutex);
		dbs_timer_init(this_dbs_info);
        register_early_suspend(&_powersave_early_suspend);
		break;

	case CPUFREQ_GOV_STOP:
		skip_hotplug_flag = 1; 			//  disable hotplugging during stop to avoid deadlocks if we are in the hotplugging logic
		this_dbs_info->check_cpu_skip = 1;	//  and we disable cpu_check also on next 25 samples
		mutex_lock(&dbs_mutex);			//  added for deadlock fix on governor stop
		dbs_timer_exit(this_dbs_info);
		mutex_unlock(&dbs_mutex);		//  added for deadlock fix on governor stop
		this_dbs_info->idle_exit_time = 0;	//  added idle exit time handling
		
		mutex_lock(&dbs_mutex);
		dbs_enable--;
		mutex_destroy(&this_dbs_info->timer_mutex);
		/*
		 * Stop the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 0)
			cpufreq_unregister_notifier(
					&dbs_cpufreq_notifier_block,
					CPUFREQ_TRANSITION_NOTIFIER);

		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

        unregister_early_suspend(&_powersave_early_suspend);

#ifdef CONFIG_CPU_FREQ_LCD_FREQ_DFS
if (dbs_tuners_ins.lcdfreq_enable == true) {
		lcdfreq_lock_current = 0; 		//  LCDFreq Scaling disable at stop
		_lcdfreq_lock(lcdfreq_lock_current);	//  LCDFreq Scaling disable at stop
}
#endif
		break;

	case CPUFREQ_GOV_LIMITS:
		skip_hotplug_flag = 1;			//  disable hotplugging during limit change
		this_dbs_info->check_cpu_skip = 1;	//  to avoid deadlocks skip check_cpu next 25 samples
		for (i = 0; i < 1000; i++);		//  wait a few samples to be sure hotplugging is off (never be sure so this is dirty)
		/*
		 * we really want to do this limit update but here are deadlocks possible if hotplugging locks are active, so if we are about
		 * to crash skip the whole freq limit change attempt by using mutex_trylock instead of mutex_lock.
		 * so now this is a real fix but on the other hand it could also avoid limit changes so we keep all the other workarounds
		 * to reduce the chance of such situations!
		 */
		if (mutex_trylock(&this_dbs_info->timer_mutex)) {
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(
					this_dbs_info->cur_policy,
					policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		} else {
		return 0;
		}
		/*
		* obviously this "limit case" will be executed multible times at suspend (not sure why!?)
		* but we have already a early suspend code to handle scaling search limits so we have to use a flag to avoid double execution at suspend!
		*/
		
		if (suspend_flag == 0) {
		    for (i=0; i < freq_table_size; i++) { 		//  trim search in scaling table
		    if (policy->max == mn_freqs[i][MN_FREQ]) {
			max_scaling_freq_hard = i; 	//  set new freq scaling number
			break;
		    }
		}
		
		if (max_scaling_freq_soft < max_scaling_freq_hard) { 		//  if we would go under soft limits reset them
			    max_scaling_freq_soft = max_scaling_freq_hard; 	//  if soft value is lower than hard (freq higher than hard max limit) then set it to hard max limit value
			    if (policy->max <= dbs_tuners_ins.freq_limit) 	//  check limit
			    dbs_tuners_ins.freq_limit = 0;			//  and delete active limit if it is under hard limit
			    if (policy->max <= dbs_tuners_ins.freq_limit_sleep) //  check sleep limit
				dbs_tuners_ins.freq_limit_sleep = 0;		//  if we would go also under this soft limit delete it also
		} else if (max_scaling_freq_soft > max_scaling_freq_hard && dbs_tuners_ins.freq_limit == 0) {
			    max_scaling_freq_soft = max_scaling_freq_hard; 	//  if no limit is set and new limit has a higher number than soft (freq lower than limit) then set back to hard max limit value
			} 							//  if nothing applies then leave search range as it is (in case of soft limit most likely)
		}

		skip_hotplug_flag = 0; 						//  enable hotplugging again
		this_dbs_info->time_in_idle = get_cpu_idle_time_us(cpu, &this_dbs_info->idle_exit_time); //  added idle exit time handling
		break;
	}
	return 0;
}

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_IMPULSE
static
#endif
struct cpufreq_governor cpufreq_gov_impulse = {
	.name			= "impulse",
	.governor		= cpufreq_governor_dbs,
	.max_transition_latency	= TRANSITION_LATENCY_LIMIT,
	.owner			= THIS_MODULE,
};

static int __init cpufreq_gov_dbs_init(void) //  added idle exit time handling
{
    unsigned int i;
    struct cpu_dbs_info_s *this_dbs_info;
    /* Initalize per-cpu data: */
    for_each_possible_cpu(i) {
	this_dbs_info = &per_cpu(cs_cpu_dbs_info, i);
	this_dbs_info->time_in_idle = 0;
	this_dbs_info->idle_exit_time = 0;
    }
	return cpufreq_register_governor(&cpufreq_gov_impulse);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_impulse);
}

/*
 * Impulse governor is based on "zzmoove 0.4 developed by Zane Zaminsky <cyxman@yahoo.com>
 * Source Code zzmoove 0.5.1b: https://github.com/zanezam/cpufreq-governor-zzmoove/commit/4d4023b876b1c2caa6cfd70581fb2e349fd879fa "
 *
 * and zzmoove governor is based on the modified conservative (original author
 * Alexander Clouter <alex@digriz.org.uk>) smoove governor from Michael
 * Weingaertner <mialwe@googlemail.com> (source: https://github.com/mialwe/mngb/)
 * Modified by Zane Zaminsky November 2012 to be hotplug-able and optimzed for use
 * with Samsung I9300. CPU Hotplug modifications partially taken from ktoonservative
 * governor from ktoonsez KT747-JB kernel (https://github.com/ktoonsez/KT747-JB)
 */

MODULE_AUTHOR("Javier Sayago <admin@lonasdigital.com>");
MODULE_DESCRIPTION("'cpufreq_impulse' - A dynamic cpufreq/cpuhotplug impulse governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_IMPULSE
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
