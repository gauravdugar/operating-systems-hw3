#ifndef FILE_H

#define FILE_H

#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/slab.h>

int myOpen(const char *filename, int oflags, mode_t mode, struct file **fp);
void myClose(struct file **filp);
int myRead(struct file *filp, void *buf, int len, int offset);
int myWrite(struct file *filp, void *buf, int len, int offset);
int mySync(struct file *file);

#endif
