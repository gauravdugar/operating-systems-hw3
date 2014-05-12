#ifndef SYS_DS_H

#define SYS_DS_H

#define SUBMIT 1
#define CANCEL 2
#define LIST 3

#define CHECKSUM 1
#define ENCRYPT 2
#define DECRYPT 3

struct jobs {
	int work_type;
	int type;
	void *config;
};

struct crypt_args {
	int keysize;
	char *file;
	char *algo;
	char *key;
};

struct checksum_args {
	char *algo;
	char *file;
};

struct list_args {
	struct list_buf *buf;
	int start_id;
	int ret;
	int additional;
};

struct list_buf {
	int pid;
	int type;
	int wid;
};

#endif
