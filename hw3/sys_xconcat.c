#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/slab.h>

asmlinkage extern long(*sysptr)(void *arg, int structlen);

/* Structure to retrieve arguments..!! */
struct argStruct {
	__user const char *outfile; /* name of output file */
	__user const char **infiles; /* array with names of input files */
	unsigned int infile_count; /* number of input files in infiles array */
	int oflags; /* Open flags to change behavior of syscall */
	mode_t mode; /* default permission mode for newly created outfile */
	unsigned int flags; /* special flags to change behavior of syscall */
};

/**
 * myOpen() - It opens the file & set file descriptor.
 * @filename:	filename of file to be opened.
 * @oflags:	oflags are the mode in which file will be opened.
 * @mode:	mode specifies the permission of file if created new.
 * @fp:		file descriptor.
 *
 * It returns 0 if all goes well, else error is returned.
 */
int myOpen(const char *filename, int oflags, mode_t mode, struct file **fp)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	filp = filp_open(filename, oflags, mode);
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	if (!filp || IS_ERR(filp)) {
		printk(KERN_INFO
		"Error in reading file %s, error = %d,\n",
		filename, (int) PTR_ERR(filp));
		*fp = NULL;
		return PTR_ERR(filp);
	}
	*fp = filp;
	return 0;
}

/**
 * myClose() - It closes the files and flushes their memory.
 * @filp_in:	file descriptor for input file
 * @filp_out:	file descriptor for output file
 */
void myClose(struct file *filp_in, struct file *filp_out)
{
	if (filp_in != NULL)
		filp_close(filp_in, NULL);
	if (filp_out != NULL)
		filp_close(filp_out, NULL);
}

/**
 * myRead() - It reads file from fd in buffer.
 * @filp:	file descriptor of file to be read.
 * @buf:	buffer to be filled with content read.
 * @len:	length of bytes to be read.
 * @offset:	offset from which reading should start.
 *
 * returns negative numbers if error else number of bytes read.
 */
int myRead(struct file *filp, void *buf, int len, int offset)
{
	mm_segment_t oldfs;
	int bytes;
	filp->f_pos = offset;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytes = filp->f_op->read(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);
	return bytes;
}

/**
 * myWrite() - It writes in file from buffer content.
 * @filp:	file descriptor of file to be written.
 * @buf:	buffer for content read.
 * @len:	length of bytes to be written.
 * @offset:	offset from which writing should start.
 *
 * returns negative numbers if error else number of bytes written.
 */
int myWrite(struct file *filp, void *buf, int len, int offset)
{
	mm_segment_t oldfs;
	int bytes;
	filp->f_pos = offset;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytes = filp->f_op->write(filp, buf, len, &filp->f_pos);
	set_fs(oldfs);
	return bytes;
}

/**
 * checkStruct() - It checks different validations & copies structure.
 * @ptr:	structure pointer to be filled with user data securely.
 * @arg:	strcuture pointer in void send by user.
 * @structlen:	sizeof structure in user side, sent by user.
 *
 * This function allocates kernel memory to structure and copies
 * all data from user provided structure pointer after all validations.
 * It also makes sure to free any allocated memory before exiting.
 *
 * Returns 0 if all goes well, else returns error.
 */
int checkStruct(struct argStruct **ptr, void *arg, int structlen)
{
	struct argStruct *newPtr;
	const char **inputFiles = NULL;
	int check = 0;
	int i = 0;
	if (sizeof(struct argStruct) != structlen)
		return -EINVAL;
	check = access_ok(VERIFY_READ, arg, structlen);
	if (check < 0)
		return -EFAULT;
	*ptr = kmalloc(sizeof(struct argStruct), GFP_KERNEL);
	newPtr = *ptr;
	check = copy_from_user(newPtr, arg, structlen);
	if (check > 0) {
		check = -EINVAL;
		goto free_mem;
	}
	if (newPtr->infile_count < 1 || newPtr->infile_count > 10) {
		check = -EINVAL;
		goto free_mem;
	}

	if (!newPtr->infiles || !newPtr->outfile) {
		check = -EINVAL;
		goto free_mem;
	}

	inputFiles = kmalloc(sizeof(char *) * (newPtr->infile_count),
	GFP_KERNEL);
	for (i = 0; i < newPtr->infile_count; i++) {
		if (!newPtr->infiles[i]) {
			check = -EINVAL;
			goto free_mem;
		}

		inputFiles[i] = getname(newPtr->infiles[i]);
		if (*inputFiles[i] < 0) {
			check = (int) *inputFiles[i];
			goto free_mem;
		}
	}
	newPtr->outfile = getname(newPtr->outfile);
	if (*newPtr->outfile < 0) {
		check = (int) *newPtr->outfile;
		goto free_mem;
	}
	newPtr->infiles = inputFiles;
	if (newPtr->mode > 0777) {
		check = -EINVAL;
		goto free_mem;
	}
	if (newPtr->flags > 0x06 || newPtr->flags == 0x03) {
		check = -EINVAL;
		goto free_mem;
	}
	if ((newPtr->oflags & (O_APPEND | O_CREAT | O_TRUNC | O_EXCL))
	!= newPtr->oflags) {
		check = -EINVAL;
		goto free_mem;
	}
	return 0;

free_mem:
	kfree(*ptr);
	i--;
	while (i >= 0) {
		putname(inputFiles[i]);
		i--;
	}
	return check;
}

/**
 * xconcat() - It copies bytes from infiles to outfile.
 * @arg:	structure pointer wrapped in void pointer by user.
 * @structlen:	size of structure in user code, sent by user.
 *
 * It opens outfile and infiles and writes bytes from infiles
 * to outfile with validations. It also handles code cleanup and
 * releasing any memory allocated to avoid memory leaks.
 *
 * Returns either error (negative) or required output as per flags.
 */
asmlinkage long xconcat(void *arg, int structlen)
{
	if (arg == NULL)
		return -EINVAL;
	else {
		struct argStruct *argPtr = NULL;
		int page_size = PAGE_SIZE;
		struct file *filp_in = NULL;
		struct file *filp_out = NULL;
		int i = 0;
		int bytesRead = 0;
		int bytesWrite = 0;
		int offsetOut = 0;
		int totalBytesRead = 0;
		int totalBytesWrite = 0;
		int check = 0;
		int filesRead = 0;
		int temp = 0;
		char *buf = NULL;

		check = checkStruct(&argPtr, arg, structlen);

		if (check != 0)
			return check;

		check = myOpen(argPtr->outfile, O_APPEND | O_WRONLY,
		argPtr->mode, &filp_out);
		if ((check == -ENOENT && ((argPtr->oflags & O_CREAT) == 0))
			|| (check < 0 && check != -ENOENT)) {
			printk(KERN_INFO "Error in outfile %s, err = %d\n",
			argPtr->outfile, check);
			goto cleanup;
		}

		for (i = 0; i < argPtr->infile_count; i++) {
			check = myOpen(argPtr->infiles[i],
			O_RDONLY, 0, &filp_in);
			if (check != 0)
				goto cleanup;
			if (filp_out && filp_out->f_dentry->d_inode
			== filp_in->f_dentry->d_inode) {
				check = -EINVAL;
				goto cleanup;
			}
			myClose(filp_in, NULL);
			filp_in = NULL;
		}
		myClose(NULL, filp_out);
		filp_out = NULL;

		check = myOpen(argPtr->outfile, argPtr->oflags | O_WRONLY,
		argPtr->mode, &filp_out);
		if (check != 0)
			goto cleanup;

		for (i = 0; i < argPtr->infile_count; i++) {
			int offsetIn = 0;
			bytesRead = page_size;
			check = myOpen(argPtr->infiles[i],
			O_RDONLY, 0, &filp_in);
			if (check != 0)
				goto cleanup;
			while (bytesRead == page_size) {
				buf = kmalloc(page_size, GFP_KERNEL);
				bytesRead = myRead(filp_in, buf, page_size,
				offsetIn);
				if (bytesRead < 0) {
					temp = 1;
					break;
				}
				totalBytesRead += bytesRead;
				bytesWrite =
				myWrite(filp_out, buf, bytesRead, offsetOut);
				if (bytesWrite < bytesRead) {
					temp = 1;
					break;
				}
				totalBytesWrite += bytesWrite;
				offsetIn += bytesRead;
				offsetOut += bytesWrite;
				kfree(buf);
				buf = NULL;
			}
			if (temp == 1)
				break;
			filesRead++;
			myClose(filp_in, NULL);
			filp_in = NULL;
		}
		myClose(NULL, filp_out);
		filp_out = NULL;

		if ((argPtr->flags & 0x02) == 0x02) {
			int percent = (int)((offsetOut*100)/totalBytesRead);
			check = percent;
			goto cleanup;
		}
		if ((argPtr->flags & 0x01) == 0x01) {
			check = filesRead;
			goto cleanup;
		}
		check = totalBytesWrite;
		goto cleanup;

cleanup:
		temp = argPtr->infile_count - 1;
		while (temp >= 0) {
			putname(argPtr->infiles[temp]);
			temp--;
		}
		kfree(argPtr->infiles);
		putname(argPtr->outfile);
		kfree(argPtr);
		if (buf != NULL)
			kfree(buf);
		if (filp_in == NULL)
			myClose(NULL, filp_out);
		if (filp_out == NULL)
			myClose(filp_in, NULL);
		return check;
	}
}

static int __init init_sys_xconcat(void)
{
	printk(KERN_INFO "installed new sys_xconcat module\n");
	if (sysptr == NULL)
		sysptr = xconcat;
	return 0;
}

static void  __exit exit_sys_xconcat(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk(KERN_INFO "removed sys_xconcat module\n");
}

module_init(init_sys_xconcat);
module_exit(exit_sys_xconcat);
MODULE_LICENSE("GPL");
