#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/slab.h>

#include "sioq.h"

asmlinkage extern int(*sysptr)(void *args, int argslen);

asmlinkage int xjob(void *args, int argslen)
{
	struct sioq_args *x;
	if (args == NULL)
		return -EINVAL;
	if (argslen != 1)
		return -EINVAL;
	printk("\n\nargslen = %d\n\n", argslen);
	x = kmalloc(sizeof(struct sioq_args), GFP_KERNEL);
	x->id = argslen;
	run_sioq(testPrint, x);
	stop_sioq();
	printk("\n\njob completed..!!\n\n");
	return 0;
}

static int __init init_sys_xjob(void)
{
	int err = 0;
	printk(KERN_INFO "installed new sys_xjob module\n");
	if (sysptr == NULL)
		sysptr = xjob;
	err = init_sioq();
	return 0;
}

static void  __exit exit_sys_xjob(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk(KERN_INFO "removed sys_xjob module\n");
}

module_init(init_sys_xjob);
module_exit(exit_sys_xjob);
MODULE_LICENSE("GPL");
