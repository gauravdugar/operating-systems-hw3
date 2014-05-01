#ifndef _SIOQ_H
#define _SIOQ_H

#include <linux/sched.h>
#include <linux/workqueue.h>

struct sioq_args {
	struct completion comp;
	struct work_struct work;
	int err;
	int id;
	void *ret;
};

extern int __init init_sioq(void);
extern void stop_sioq(void);
extern void run_sioq(work_func_t func, struct sioq_args *args);

extern void testPrint(struct work_struct *work);

#endif /* not _SIOQ_H */
