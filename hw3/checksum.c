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
	struct checksum_args arg;
	int rc;
	char *checksum;
	if (argc < 3) {
		printf("insufficient args");
		return 1;
	}
	job.work_type = SUBMIT;
	job.type = CHECKSUM;
	job.config = &arg;
	arg.algo = argv[2];
	arg.file = argv[1];

	void *dummy = ((void *) &job);
	init_msg();
	rc = syscall(__NR_xjob, dummy, sizeof(job));

	if (rc < 0) {
		printf("syscall returned %d (errno=%d)\n", rc, errno);
		perror("Message from Syscall");
		exit(rc);
	}

	printf("Job Successfully Submitted, Job ID: %d\n", rc);
	checksum = get_msg();
	if (*checksum != '~')
		printf("Checksum %s\n", checksum);
	else
		printf("Error while checksum, err=%d, msg=%s\n",
			atoi(checksum+1), strerror(-atoi(checksum + 1)));
	return 0;
}
