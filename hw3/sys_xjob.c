#include "sys_xjob.h"

asmlinkage extern int(*sysptr)(void *args, int argslen);

int counter = 0;

static int checkArgument(void *arg, int argslen)
{
	int err = 0;
	if (argslen != 1)
		return -EINVAL;
	return err;
}

asmlinkage int xjob(void *args, int argslen)
{
	struct sioq_args *x;
	int err = 0;
	if (args == NULL)
		return -EINVAL;
	//err = checkArgument(args, argslen);
	if (err < 0)
		return err;
	printk("\nargslen = %d,", argslen);
	x = kmalloc(sizeof(struct sioq_args), GFP_KERNEL);
	x->id = argslen;
	run_sioq(testPrint, x);
	printk("\nJob completed..!!");
	return err;
}

static int __init init_sys_xjob(void)
{
	printk(KERN_INFO "installed new sys_xjob module\n");
	if (sysptr == NULL)
		sysptr = xjob;
	init_sioq();
	counter = 0;
	return 0;
}

static void  __exit exit_sys_xjob(void)
{
	//stop_sioq();
	if (sysptr != NULL)
		sysptr = NULL;
	printk(KERN_INFO "removed sys_xjob module\n");
}

module_init(init_sys_xjob);
module_exit(exit_sys_xjob);
MODULE_LICENSE("GPL");
