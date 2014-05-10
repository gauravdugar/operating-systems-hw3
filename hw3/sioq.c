#include "sys_xjob.h"

#define CONSUMERS 1
#define QUEUE_SIZE 5

static struct workqueue_struct *superio_workqueue;

struct mutex lock;
struct sioq_args **work_array = NULL;
static int count = -1;
extern atomic_t counter;

int init_sioq(void)
{
	int err;
	int i = 0;
	superio_workqueue = create_workqueue("my_workqueue");
	if (!IS_ERR(superio_workqueue)) {
		workqueue_set_max_active(superio_workqueue, CONSUMERS);
		work_array = kmalloc(sizeof(struct sioq_args) * QUEUE_SIZE, GFP_KERNEL);
		if(work_array == NULL) {
			err = -ENOMEM;
			stop_sioq();
			return err;
		}
		while (i < QUEUE_SIZE)
			work_array[i++] = 0;
		mutex_init(&lock);
		return 0;
	}
	err = PTR_ERR(superio_workqueue);
	printk(KERN_ERR "create_workqueue failed %d\n", err);
	superio_workqueue = NULL;
	return err;
}

static int get_wid(void)
{
	int id = count;
	int i = 0;
	while(work_array[id] != 0) {
		id = (id + 1) % QUEUE_SIZE;
		i++;
		if (i == QUEUE_SIZE) {
			i = 0;
			printk("\nget_wid in waiting now..!!\n");
			schedule();
		}
	}
	return id;
}

void stop_sioq(void)
{
	if (superio_workqueue) {
		destroy_workqueue(superio_workqueue);
		superio_workqueue = NULL;
	}
	if(work_array) {
		kfree(work_array);
		work_array = NULL;
	}
}

void run_sioq(work_func_t func, struct sioq_args *args)
{
	int id;
	mutex_lock(&lock);
	if (atomic_read(&counter) == QUEUE_SIZE){
		schedule();
	}
	id = get_wid();
	args->id = id;
	args->pid = current->pid;

	INIT_WORK(&args->work, func);

	init_completion(&args->comp);
	while (!queue_work(superio_workqueue, &args->work)) {
		schedule();
	}
	work_array[id] = args;
	count = id;
	atomic_inc(&counter);
	mutex_unlock(&lock);
	printk("\nMutex unlocking..!!\n");
}

int cancel_work(int id)
{
	struct sioq_args *x = work_array[id];
	if(x == 0) {
		printk("\nNo Work ID %d, No such work exists..!!\n", id);
		return -4;
 	}
	if(work_pending(&x->work)) {
		cancel_work_sync(&x->work);
		work_array[x->id] = 0;
		return 0;
	}
	return -5;
}

void testPrint(struct work_struct *work)
{
	struct sioq_args *args = container_of(work, struct sioq_args, work);
	int id = args->id;
	int i = 0;
	printk("\nID before sleep = %d, %d,", id, work_pending(&args->work));
	msleep(5000);
	printk("\nID After sleep = %d\n,", id);
	atomic_dec(&counter);
	work_array[args->id] = 0;
	send_msg(args->pid, "Work finished..!!");
	for (i = 0; i < QUEUE_SIZE; i++)
		printk ("%p, ", work_array[i]);
	kfree(args);
}
