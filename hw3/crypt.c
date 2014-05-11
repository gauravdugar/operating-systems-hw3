#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "sys_ds.h"

#define __NR_xjob	349	/*our private syscall number*/

extern char *get_msg();
extern int init_msg();

int main(int argc, char *argv[])
{
	struct jobs job;
	struct crypt_args arg;
	int rc;
	if (argc < 5) {
		printf("Insufficient Arguments");
		return 1;
	}
	job.work_type = SUBMIT;
	if (!strcmp(argv[argc-1], "1"))
		job.type = ENCRYPT;

	if (!strcmp(argv[argc-1], "0"))
		job.type = DECRYPT;

	job.config = &arg;
	arg.algo = argv[1];
	arg.key = argv[3];
	arg.keysize = strlen(arg.key);
	arg.file = argv[2];
	void *p = ((void *) &job);
	init_msg();
	rc = syscall(__NR_xjob, p, sizeof(job));
	if (rc < 0) {
		printf("syscall returned %d (errno=%d)\n", rc, errno);
		perror("Message from Syscall");
		exit(rc);
	} else {
		printf("Job Successfully Submitted, Job ID: %d\n", rc);
		rc = atoi(get_msg());
		if(!rc)
			printf("Job Successfully Completed\n");
		else
			printf("Error while checksum operation, error code=%d, error msg=%s\n", rc, strerror(-rc));
	}
	return 0;
}
