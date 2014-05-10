#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "sys_ds.h"


#define __NR_xjob	349	/*our private syscall number*/

int main(int argc, char *argv[])
{
	int rc;

	struct jobs job;
	job.work_type = CANCEL;
	job.type = atoi(argv[1]);
	void *p = ((void *) &job);
	rc = syscall(__NR_xjob, p, sizeof(job));
	if (rc < 0) {
		printf("syscall returned %d (errno=%d)\n", rc, errno);
		perror("Message from Syscall");
	} else
			printf("Work Cancelled: %d\n", rc);
	exit(rc);
}
