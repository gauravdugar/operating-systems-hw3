#include "sys_xjob.h"

#define CONSUMERS 1
#define QUEUE_SIZE 5

static struct workqueue_struct *superio_workqueue;

struct mutex lock;
struct sioq_args **work_array = NULL;
static int count = -1;
extern atomic_t counter;

static void testPrint(struct work_struct *work);

int init_sioq(void)
{
	int err;
	superio_workqueue = create_workqueue("my_workqueue");
	if (!IS_ERR(superio_workqueue)) {
		workqueue_set_max_active(superio_workqueue, CONSUMERS);
		work_array = kzalloc(sizeof(struct sioq_args) * QUEUE_SIZE, GFP_KERNEL);
		if(work_array == NULL) {
			err = -ENOMEM;
			stop_sioq();
			return err;
		}
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

/*
 * Function taken from fs_struct.c and modified
 */
static void __set_fs_pwd(struct fs_struct *fs, struct path *path)
{
	struct path old_pwd;

	spin_lock(&fs->lock);
	write_seqcount_begin(&fs->seq);
	old_pwd = fs->pwd;
	fs->pwd = *path;
	path_get(path);
	write_seqcount_end(&fs->seq);
	spin_unlock(&fs->lock);

	if (old_pwd.dentry)
		path_put(&old_pwd);
}

void stop_sioq(void)
{
	if (superio_workqueue) {
		destroy_workqueue(superio_workqueue);
		superio_workqueue = NULL;
	}
	if(work_array) {
		int i;
		for ( i = 0; i < QUEUE_SIZE; i++) {
			if (work_array[i] != 0) {
				kfree(work_array[i]->checksum_arg);
				kfree(work_array[i]);
			}
		}
		kfree(work_array);
		work_array = NULL;
	}
}

int run_sioq(struct sioq_args *args)
{
	int id;
	mutex_lock(&lock);
	if (atomic_read(&counter) == QUEUE_SIZE){
		schedule();
	}
	id = get_wid();
	args->id = id;
	args->pid = current->pid;
	get_fs_pwd(current->fs, &args->pwd);

	INIT_WORK(&args->work, testPrint);

	init_completion(&args->comp);

	work_array[id] = args;
	count = id;
	atomic_inc(&counter);

	args->complete = 0;

	while (unlikely(!queue_work(superio_workqueue, &args->work))) {
		schedule();
	}
	mutex_unlock(&lock);
	return id;
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
		send_msg(x->pid, "Job has been cancelled..!!");
		kfree(x->checksum_arg);
		kfree(x);
		return 0;
	}
	return -5;
}

static void __call_send_msg(struct sioq_args *args, char *msg)
{
	while (!args->complete)
		schedule();
	schedule();
	send_msg(args->pid, msg);
}

static void __calc_hash(struct sioq_args *args)
{
	struct file *file_handle;
	char err_code[4];
	char *hash = NULL;
	int err = 0;

	struct checksum_args *check_arg = args->checksum_arg;
	err_code[0] = '~'; // WHY..???

	err = myOpen(check_arg->file, O_RDONLY, 0, &file_handle);
	if (err) {
		printk(KERN_ERR "Error opening file: %d", err);
		sprintf(err_code+1, "%d", err);
		__call_send_msg(args, err_code);
		goto out;
	}
	err = calc_hash(check_arg->algo, file_handle, &hash);
	myClose(&file_handle);
	if(err) {
		sprintf(err_code, "%d", err);
		__call_send_msg(args, err_code);
		goto out;
	} else {
		printk("digest: %s\n", hash);
		__call_send_msg(args, hash);
	}
out:
	if (hash)
		kfree(hash);
}

static void __crypt_file(struct sioq_args *args, int enc)
{
	struct file *infile = NULL, *outfile = NULL;
	char err_code[4];
	int err = 0;
	err = myOpen("/usr/src/hw3-gdugar/hw3/123", O_RDONLY, 0, &infile);
	if (err) {
		printk(KERN_ERR "Error opening file: %d", err);
		sprintf(err_code, "%d", err);
		__call_send_msg(args, err_code);
		return;
	}

	err = myOpen("/usr/src/hw3-gdugar/hw3/456", O_WRONLY | O_CREAT, 0666, &outfile);
	if (err) {
		printk(KERN_ERR "Error opening temp file: %d", err);
		sprintf(err_code, "%d", err);
		myClose(&infile);
		__call_send_msg(args, err_code);
		return;
	}

	err = encrpt_decrypt_file(infile, outfile, "cbc(aes)", "1234567890123456", 16, "dhsjadjffjgdsfgshgfhsgdfgshg", enc);
	myClose(&infile);

	if(err) {
		vfs_unlink(outfile->f_dentry->d_parent->d_inode, outfile->f_dentry);
		sprintf(err_code, "%d", err);
		__call_send_msg(args, err_code);
		return;
	} else
		__call_send_msg(args, "0");
	myClose(&outfile);
}

void testPrint(struct work_struct *work)
{
	struct sioq_args *args = container_of(work, struct sioq_args, work);
	int id = 0;

	id = args->id;
	__set_fs_pwd(current->fs, &args->pwd);

	if (args->type == ENCRYPT)
		__crypt_file(args, 1);
	if (args->type == DECRYPT)
		__crypt_file(args, 0);
	if (args->type == CHECKSUM)
		__calc_hash(args);

	printk("\nID DONE = %d\n,", id);
	atomic_dec(&counter);
	work_array[args->id] = 0;

	//send_msg(args->pid, "Work finished..!!");
	//for (i = 0; i < QUEUE_SIZE; i++)
	//	printk ("%p, ", work_array[i]);
	kfree(args->checksum_arg);
	kfree(args);
}
