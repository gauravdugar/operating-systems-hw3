#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "sys_ds.h"

#define __NR_xjob	349	/* our private syscall number */

extern int get_msg();

// parseOptions and option args to be set, for checking correctness of type and number of arguments.

int main(int argc, char *argv[])
{
	int rc; /* return value from syscall */
	struct jobs job;

	job.work_type = SUBMIT;
	void *dummy = (void *)&job;
	rc = syscall(__NR_xjob, dummy, sizeof(job));

	if (errno != 0) {
		printf("ERROR = %s,\n", strerror(errno));
		printf("ERROR_NUMBER = %d\n", errno);
	}
	get_msg();
	return 0;
}
