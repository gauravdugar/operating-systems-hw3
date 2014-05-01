#include "sioq.h"

static struct workqueue_struct *superio_workqueue;

int __init init_sioq(void)
{
	int err;

	superio_workqueue = create_workqueue("unionfs_siod");
	if (!IS_ERR(superio_workqueue))
		return 0;

	err = PTR_ERR(superio_workqueue);
	printk(KERN_ERR "unionfs: create_workqueue failed %d\n", err);
	superio_workqueue = NULL;
	return err;
}

void stop_sioq(void)
{
	if (superio_workqueue)
		destroy_workqueue(superio_workqueue);
}

void run_sioq(work_func_t func, struct sioq_args *args)
{
	INIT_WORK(&args->work, func);

	init_completion(&args->comp);
	while (!queue_work(superio_workqueue, &args->work)) {
		schedule();
	}
	wait_for_completion(&args->comp);
}

void testPrint(struct work_struct *work)
{
	struct sioq_args *args = container_of(work, struct sioq_args, work);
	int id = args->id;
	printk("\n\nID = %d,\n\n", id);
	complete(&args->comp);
}
