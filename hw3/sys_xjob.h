#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "sioq.h"

struct jobs {
	const char **infiles;
	int op;
	void *config;
	int flags; /*flags to change behavior of syscall*/
};
