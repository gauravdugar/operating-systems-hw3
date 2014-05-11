#include "sys_xjob.h"

asmlinkage extern int(*sysptr)(void *args, int argslen);

int counter;

static inline int fileReadCheck(char *path)
{
	struct file *file;
	int err;
	err = myOpen(path, O_RDONLY, 0, &file);
	myClose(&file);
	return err;
}

static int check_checksum_args(struct jobs *x)
{
	struct checksum_args *args = NULL;
	int err;
	err = access_ok(VERIFY_READ, x->config, sizeof(struct checksum_args));
	if (err < 0) {
		printk(KERN_ERR "Checksum Access_OK failed - %d,\n", err);
		err =  -EINVAL;
		goto out;
	}
	args = kzalloc(sizeof(struct checksum_args), GFP_KERNEL);
	if (!args) {
		err = -ENOMEM;
		goto out;
	}
	err = copy_from_user(args, x->config, sizeof(struct checksum_args));
	if (err) {
		printk(KERN_ERR "Checksum copy_from_user failed: %d\n", err);
		err = -EINVAL;
		goto free2;
	}
	args->file = getname(args->file);
	if (args->file < 0) {
		err = (int) *args->file;
		goto free2;
	}
	err = fileReadCheck(args->file);
	if (err)
		goto free1;
	args->algo = getname(args->algo);
	if (args->algo < 0) {
		err = (int) *args->algo;
		goto free1;
	}
	x->config = args;
	goto out;

free1:
	putname(args->file);
free2:
	kfree(args);
out:
	return err;
}

static int check_crypt_args(struct jobs *x)
{
	struct crypt_args *args = NULL;
	int err;
	err = access_ok(VERIFY_READ, x->config, sizeof(struct crypt_args));
	if (err < 0) {
		printk(KERN_ERR "Crypt Access_OK failed - %d\n", err);
		err =  -EINVAL;
		goto out;
	}
	args = kzalloc(sizeof(struct crypt_args), GFP_KERNEL);
	if (!args) {
		err = -ENOMEM;
		goto out;
	}
	err = copy_from_user(args, x->config, sizeof(struct crypt_args));
	if (err) {
		printk(KERN_ERR "Crypt copy_from_user failed: %d\n", err);
		err = -EINVAL;
		goto free3;
	}
	args->file = getname(args->file);
	if (args->file < 0) {
		err = (int) *args->file;
		goto free3;
	}
	err = fileReadCheck(args->file);
	if (err)
		goto free2;
	args->algo = getname(args->algo);
	if (args->algo < 0) {
		err = (int) *args->algo;
		goto free2;
	}
	args->key = getname(args->key);
	if (args->key < 0) {
		err = (int) *args->key;
		goto free1;
	}
	x->config = args;
	goto out;

free1:
	putname(args->algo);
free2:
	putname(args->file);
free3:
	kfree(args);
out:
	return err;
}

static int checkArgument(struct jobs **job, void *arg, int len)
{
	int err = 0;
	struct jobs *x;
	len = sizeof(struct jobs);
	if (sizeof(struct jobs) != len) {
		printk(KERN_ERR "Incorrect arguments - %d\n", len);
		return -EINVAL;
	}
	err = access_ok(VERIFY_READ, arg, len);
	if (err < 0) {
		printk(KERN_ERR "User param Access_OK failed - %d\n", err);
		return -EINVAL;
	}
	*job = kzalloc(sizeof(struct jobs), GFP_KERNEL);
	if (!*job)
		return -ENOMEM ;
	x = *job;
	err = copy_from_user(x, arg, len);
	if (err > 0) {
		printk(KERN_ERR "User param copy_from_user failed - %d\n", err);
		err = -EINVAL;
		goto out;
	}

	if (!(x->work_type != 1 || x->work_type != 2 || x->work_type != 3)) {
		printk(KERN_ERR "Incorrect work_type %d\n", x->work_type);
		err = -EINVAL;
		goto out;
	}

	if (x->work_type == SUBMIT && (x->type < 1 || x->type > 3)) {
		printk(KERN_ERR "Incorrect job_type %d,", x->type);
		err = -EINVAL;
		goto out;
	}

	if (x->type == ENCRYPT || x->type == DECRYPT)
		err = check_crypt_args(x);

	if (x->type == CHECKSUM)
		err = check_checksum_args(x);

	if (err) {
		printk(KERN_ERR "Wrong Arguments..!!\n");
		goto out;
	}
	return 0;
out:
	kfree(*job);
	return err;
}

asmlinkage int xjob(void *args, int argslen)
{
	struct jobs *job;
	struct sioq_args *x;
	int err = 0;

	if (args == NULL)
		return -EINVAL;

	err = checkArgument(&job, args, argslen);
	if (err < 0)
		return err;

	if (job->work_type == CANCEL) {
		return cancel_work(job->type);
		goto out;
	}

	if (job->work_type == LIST) {
		return list_work();
		goto out;
	}

	x = kzalloc(sizeof(struct sioq_args), GFP_KERNEL);
	x->type = job->type;

	if (job->type == ENCRYPT || job->type == DECRYPT)
		x->crypt_arg = job->config;

	if (job->type == CHECKSUM)
		x->checksum_arg = job->config;

	err = run_sioq(x);
	x->complete = 1;
out:
	kfree(job);
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
