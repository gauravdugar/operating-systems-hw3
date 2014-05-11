#ifndef SIOQ_H

#define SIOQ_H

#include <linux/path.h>
#include <linux/fs_struct.h>
#include <linux/seqlock.h>
#include <linux/delay.h>

#include "sys_ds.h"

struct sioq_args {
	struct completion comp;
	struct work_struct work;
	int err;
	int pid;
	int id;
	void *ret;
	struct path pwd;
	int complete;
	int type;
	union {
		struct checksum_args *checksum_arg;
		struct crypt_args *crypt_arg;
	};
};

extern int init_sioq(void);
extern void stop_sioq(void);
extern int run_sioq(struct sioq_args *args);
extern int cancel_work(int id);

#endif
