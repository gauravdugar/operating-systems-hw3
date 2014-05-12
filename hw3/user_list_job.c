#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys_ds.h"

#define __NR_xjob	349	/*our private syscall number*/

#define SIZE 5

int main(int argc, char *argv[])
{
	struct jobs job;
	struct list_args arg;
	struct list_buf buf[SIZE];
	void *dummy;

	int rc, flag = 0;

	job.work_type = LIST;
	job.type = SIZE;
	job.config = &arg;
	arg.buf = buf;
	arg.additional = 0;
	arg.ret = 0;

	printf("Work ID\t\tProcess ID\t\tWork Type\n");
	do {
		int i;
		arg.start_id = flag;
		dummy = ((void *) &job);
		rc = syscall(__NR_xjob, dummy, sizeof(job));
		if (rc < 0) {
			printf("syscall returned %d (errno=%d)\n", rc, errno);
			perror("Message from Syscall ");
			exit(rc);
		}
		for (i = 0; i < arg.ret; i++) {
			printf("%d\t\t%d\t\t\t", buf[i].wid, buf[i].pid);
			if (buf[i].type == ENCRYPT)
				printf("ENCRYPTION");
			if (buf[i].type == DECRYPT)
				printf("DECRYPTION");
			if (buf[i].type == CHECKSUM)
				printf("CHECKSUM");
			printf("\n");
			flag = buf[i].wid + 1;
		}
	} while (arg.additional);
	return 0;
}
