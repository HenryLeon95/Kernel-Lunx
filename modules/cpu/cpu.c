// SPDX-License-Identifier: GPL-2.0
#include <linux/math64.h>
#include <linux/cpumask.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/stat.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/irqnr.h>
#include <linux/sched/cputime.h>
#include <linux/tick.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/hugetlb.h>
#include <linux/module.h>
#include <linux/kernel.h>	
#include <linux/init.h>		
#include <linux/sched.h>    
#include <linux/sched/signal.h>
#include <linux/unistd.h>      
#include <linux/ktime.h>
#include <linux/sysinfo.h>

#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif

#ifdef arch_idle_time

static u64 get_idle_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 idle;

	idle = kcs->cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait;

	iowait = kcs->cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}

#else

static u64 get_idle_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 idle, idle_usecs = -1ULL;

	if (cpu_online(cpu))
		idle_usecs = get_cpu_idle_time_us(cpu, NULL);

	if (idle_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcs->cpustat[CPUTIME_IDLE];
	else
		idle = idle_usecs * NSEC_PER_USEC;

	return idle;
}

static u64 get_iowait_time(struct kernel_cpustat *kcs, int cpu)
{
	u64 iowait, iowait_usecs = -1ULL;

	if (cpu_online(cpu))
		iowait_usecs = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_usecs == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcs->cpustat[CPUTIME_IOWAIT];
	else
		iowait = iowait_usecs * NSEC_PER_USEC;

	return iowait;
}

#endif

static int show_stat(struct seq_file *p, void *v)
{
	int i, j;
	u64 user, nice, system, idle, iowait, irq, softirq, steal, t_total, t_idle, t_usage;
	u64 guest, guest_nice;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};
	struct timespec64 boottime;

	user = nice = system = idle = iowait = 0;
    irq = softirq = steal = 0;
	guest = guest_nice = 0;
	getboottime64(&boottime);

	for_each_possible_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);

		user += kcs->cpustat[CPUTIME_USER];
		nice += kcs->cpustat[CPUTIME_NICE];
		system += kcs->cpustat[CPUTIME_SYSTEM];
		idle += get_idle_time(kcs, i);
		iowait += get_iowait_time(kcs, i);
		irq += kcs->cpustat[CPUTIME_IRQ];
		softirq += kcs->cpustat[CPUTIME_SOFTIRQ];
		steal += kcs->cpustat[CPUTIME_STEAL];
		guest += kcs->cpustat[CPUTIME_GUEST];
		guest_nice += kcs->cpustat[CPUTIME_GUEST_NICE];
		sum += kstat_cpu_irqs_sum(i);
		// sum += arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	seq_putc(p, '\n');

	for_each_online_cpu(i) {
		struct kernel_cpustat *kcs = &kcpustat_cpu(i);

		user = kcs->cpustat[CPUTIME_USER];
		nice = kcs->cpustat[CPUTIME_NICE];
		system = kcs->cpustat[CPUTIME_SYSTEM];
		idle = get_idle_time(kcs, i);
		iowait = get_iowait_time(kcs, i);
		irq = kcs->cpustat[CPUTIME_IRQ];
		softirq = kcs->cpustat[CPUTIME_SOFTIRQ];
		steal = kcs->cpustat[CPUTIME_STEAL];
		guest = kcs->cpustat[CPUTIME_GUEST];
		guest_nice = kcs->cpustat[CPUTIME_GUEST_NICE];

        t_total = user+nice+system+idle+iowait+irq+softirq+steal;
        t_idle = idle+iowait;
        t_usage = t_total - t_idle;
        seq_printf(p, "[{\"cpu\":{\n");
        seq_printf(p, "\"t_total\":\"%lld\", \n", t_total);
        seq_printf(p, "\"t_idle\": \"%lld\", \n", t_idle);
        seq_printf(p, "\"t_usage\": \"%lld\"\n}}]",t_usage);
	}
	seq_putc(p, '\n');

	return 0;
}

static int stat_open(struct inode *inode, struct file *file)
{
	unsigned int size = 1024 + 128 * num_online_cpus();
	size += 2 * nr_irqs;
	return single_open_size(file, show_stat, NULL, size);
}

static const struct file_operations proc_stat_operations = {
	.open		= stat_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init on_start(void){
    proc_create("cpu_usage", 0, NULL, &proc_stat_operations);
	return 0;
}

static void __exit on_exit(void){
    remove_proc_entry("cpu_usage",NULL);
}

module_init(on_start);
module_exit(on_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brandon Antony Chitay Coutiño");
MODULE_DESCRIPTION("Porcentaje de uso de del CPU.");