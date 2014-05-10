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
#include "sys_ds.h"

extern int init_netlink(void);
extern int netlink_exit(void);
extern void send_msg(int pid, char *msg);
