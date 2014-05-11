#include "sys_xjob.h"

asmlinkage extern int(*sysptr)(void *args, int argslen);

int counter = 0;

/*
static int checkArgument(void *arg, int argslen)
{
	int err = 0;
	if (argslen != 1)
		return -EINVAL;
	return err;
}
*/

asmlinkage int xjob(void *args, int argslen)
{
	struct jobs *job;
	struct sioq_args *x;
	int err = 0;
	if (args == NULL)
		return -EINVAL;
	//err = checkArgument(args, argslen);
	if (err < 0)
		return err;


	job = kmalloc(sizeof(struct jobs), GFP_KERNEL);
	printk("Before copy\n");
	copy_from_user(job, args, sizeof(struct jobs));
	printk("After copy\n");

	if (job->work_type == CANCEL)
		return cancel_work(job->type);

	x = kmalloc(sizeof(struct sioq_args), GFP_KERNEL);
	x->id = 1;
	msleep(5000);
	err = run_sioq(x);
	x->complete = 1;
	return err;
}

static int __init init_sys_xjob(void)
{
	printk(KERN_INFO "installed new sys_xjob module\n");
	if (sysptr == NULL)
		sysptr = xjob;
	init_sioq();
	init_netlink();
	counter = 0;
	return 0;
}

static void  __exit exit_sys_xjob(void)
{
	stop_sioq();
	netlink_exit();
	if (sysptr != NULL)
		sysptr = NULL;
	printk(KERN_INFO "removed sys_xjob module\n");
}

module_init(init_sys_xjob);
module_exit(exit_sys_xjob);
MODULE_LICENSE("GPL");
