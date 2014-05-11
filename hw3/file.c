#include "file.h"

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
 * @filp:	file descriptor of file
 */
void myClose(struct file **filp)
{
	if (*filp != NULL)
		filp_close(*filp, NULL);
	*filp = NULL;
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
int mySync(struct file *file)
{
	vfs_fsync(file, 0);
	return 0;
}
