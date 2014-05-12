#include "sys_xjob.h"

#ifdef CONSUMERS
	#if CONSUMERS <= 0
		#define CONSUMERS 2
	#endif
	#else
		#define CONSUMERS 2
#endif
#ifdef QUEUE_SIZE
	#if QUEUE_SIZE <= 0
		#define QUEUE_SIZE 1024
	#endif
	#else
		#define QUEUE_SIZE 1024
#endif

static struct workqueue_struct *superio_workqueue;

static struct mutex lock;
struct sioq_args **work_array;
static int count = -1;

static void testPrint(struct work_struct *work);

int init_sioq(void)
{
	int err;
	superio_workqueue = create_workqueue("my_workqueue");
	if (!IS_ERR(superio_workqueue)) {
		workqueue_set_max_active(superio_workqueue, CONSUMERS);
		work_array = kzalloc(sizeof(struct sioq_args) *
				QUEUE_SIZE, GFP_KERNEL);
		if (work_array == NULL) {
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
	while (work_array[id] != 0) {
		id = (id + 1) % QUEUE_SIZE;
		i++;
		if (i == QUEUE_SIZE) {
			i = 0;
			printk(KERN_ERR "\nget_wid in waiting now..!!\n");
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

void freeMem(struct sioq_args *args)
{
	if (args->type == ENCRYPT || args->type == DECRYPT) {
		putname(args->crypt_arg->algo);
		putname(args->crypt_arg->key);
		putname(args->crypt_arg->file);
		kfree(args->crypt_arg);
	}

	if (args->type == CHECKSUM) {
		putname(args->checksum_arg->algo);
		putname(args->checksum_arg->file);
		kfree(args->checksum_arg);
	}
	kfree(args);
}

void stop_sioq(void)
{
	mutex_lock(&lock);
	if (superio_workqueue) {
		destroy_workqueue(superio_workqueue);
		superio_workqueue = NULL;
	}
	if (work_array) {
		int i;
		for (i = 0; i < QUEUE_SIZE; i++)
			if (work_array[i] != 0)
				freeMem(work_array[i]);
		kfree(work_array);
		work_array = NULL;
	}
	mutex_unlock(&lock);
}

static char *tmpName(const char *name, int len)
{
	char *buf;
	buf = kmalloc(len + 6, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	strcpy(buf, ".tmp.");
	strlcat(buf, name, len + 6);
	return buf;
}

int run_sioq(struct sioq_args *args)
{
	int id;
	mutex_lock(&lock);
	if (atomic_read(&counter) == QUEUE_SIZE)
		schedule();

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

	while (!queue_work(superio_workqueue, &args->work))
		schedule();

	mutex_unlock(&lock);
	return id;
}

int list_work(struct jobs *job)
{
	struct list_buf buf;
	struct list_args *arg = job->config;
	struct sioq_args *args = NULL;
	int i = 0;
	int err = 0;
	int temp = 0;
	arg->ret = 0;
	arg->additional = 0;
	temp = (arg->start_id) % QUEUE_SIZE;

	for (i = temp; i < QUEUE_SIZE; i++) {
		if (work_array[i]) {
			if (arg->ret ==  job->type) {
				arg->additional = 1;
				break;
			}
			args = work_array[i];

			if (!work_pending(&args->work))
				continue;
			buf.wid = args->id;
			buf.pid = args->pid;
			buf.type = args->type;

			err = copy_to_user(&arg->buf[arg->ret],
				&buf, sizeof(buf));
			arg->ret++;

			if (err > 0) {
				printk(KERN_ERR "copy_to_user error: %d\n",
					err);
				return -EFAULT;
			}
		}
	}
	return 0;
}

int cancel_work(int id)
{
	struct sioq_args *x = work_array[id];
	if (x == 0) {
		printk(KERN_ERR "\nNo Work ID %d, No such work exists.\n", id);
		return -4;
	}
	if (work_pending(&x->work)) {
		cancel_work_sync(&x->work);
		work_array[x->id] = 0;
		send_msg(x->pid, "Job has been cancelled..!!");
		freeMem(x);
		return 0;
	}
	return -5;
}

static void __call_send_msg(struct sioq_args *args, char *msg)
{
	while (!args->complete)
		schedule();
	schedule();
	msleep(100);
	send_msg(args->pid, msg);
}

static void __calc_hash(struct sioq_args *args)
{
	struct file *file_handle;
	char error[5];
	char *hash = NULL;
	int err = 0;

	struct checksum_args *check_arg = args->checksum_arg;
	error[4] = '!';

	err = myOpen(check_arg->file, O_RDONLY, 0, &file_handle);
	if (err) {
		printk(KERN_ERR "Error opening file: %d", err);
		sprintf(error + 1, "%d", err);
		__call_send_msg(args, error);
		goto out;
	}
	err = calc_hash(check_arg->algo, file_handle, &hash);
	myClose(&file_handle);
	if (err) {
		sprintf(error + 1, "%d", err);
		__call_send_msg(args, error);
		goto out;
	} else {
		printk("\nDigest = %s\n", hash);
		__call_send_msg(args, hash);
	}
out:
	kfree(hash);
}

static void __crypt_file(struct sioq_args *args, int flag)
{
	struct file *infile = NULL, *outfile = NULL;
	char error[4];
	int err = 0;
	char *tmpFile;

	struct crypt_args *arg = args->crypt_arg;

	err = myOpen(arg->file, O_RDONLY, 0, &infile);
	if (err) {
		printk(KERN_ERR "Error opening file: %d", err);
		sprintf(error, "%d", err);
		__call_send_msg(args, error);
		return;
	}

	tmpFile = tmpName(infile->f_dentry->d_name.name,
			infile->f_dentry->d_name.len);

	err = myOpen(tmpFile, O_WRONLY | O_CREAT | O_TRUNC, 0666, &outfile);
	if (err) {
		printk(KERN_ERR "Error opening temp file: %d", err);
		sprintf(error, "%d", err);
		myClose(&infile);
		__call_send_msg(args, error);
		return;
	}

	err = encrypt_decrypt_file(infile, outfile, arg->algo,
			arg->key, arg->keysize, "", flag);

	if (err) {
		myClose(&infile);
		vfs_unlink(outfile->f_dentry->d_parent->d_inode,
				outfile->f_dentry);
		sprintf(error, "%d", err);
		__call_send_msg(args, error);
		return;
	} else {
		struct inode *dir = infile->f_dentry->d_parent->d_inode;
		struct dentry *olddentry = outfile->f_dentry;
		struct dentry *newdentry = infile->f_dentry;
		err = vfs_rename(dir, olddentry, dir, newdentry);
		if (err) {
			myClose(&infile);
			vfs_unlink(outfile->f_dentry->d_parent->d_inode,
					outfile->f_dentry);
			__call_send_msg(args, error);
		} else {
			__call_send_msg(args, "0");
		}
	}
}

void testPrint(struct work_struct *work)
{
	struct sioq_args *args = container_of(work, struct sioq_args, work);
	int id = 0;

	id = args->id;
	__set_fs_pwd(current->fs, &args->pwd);

	/*
	 * Added intentionally so that output can be observed for cancel jobs.
	 */
	msleep(5000);

	if (args->type == ENCRYPT)
		__crypt_file(args, 1);
	if (args->type == DECRYPT)
		__crypt_file(args, 0);
	if (args->type == CHECKSUM)
		__calc_hash(args);
	printk("Work Done.. :)\n");

	atomic_dec(&counter);
	work_array[args->id] = 0;
	freeMem(args);
}
