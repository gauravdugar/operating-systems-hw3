#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "xjob_ds.h"

#define __NR_xjob	349	/*our private syscall number*/

extern int init_msg();
extern char *get_msg();

int main(int argc, char *argv[])
{
	struct jobp job_param;
	struct checksum_args conf;
	int rc;
	char *checksum;
	if (3 > argc) {
		printf("Insufficient Arguments");
		return 1;
	}
	job.work_type = SUBMIT_JOB;
	job.arg = CALC_CHECKSUM;
	job.conf = &conf;
	conf.algo = argv[2];
	conf.file = argv[1];
	void *p = ((void *) &job_param);
	init_msg();
	rc = syscall(__NR_xjob, p, sizeof(job_param));
	if (rc < 0) {
		printf("syscall returned %d (errno=%d)\n", rc, errno);
		perror("Message from Syscall");
		exit(rc);
	} else {
		printf("Job Successfully Submitted, Job ID: %d\n", rc);
		checksum = get_msg();
		if(*checksum != '~')
			printf("Checksum %s\n", checksum);
		else
			printf("Error while checksum operation, error code=%d, error msg=%s\n", atoi(checksum+1), strerror (-atoi(checksum+1)));
	}
	return 0;
}
