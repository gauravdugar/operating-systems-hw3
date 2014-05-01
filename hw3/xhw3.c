#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define __NR_xjob	349	/* our private syscall number */

struct argStruct {
	const char *outfile; /* name of output file */
	const char **infiles; /* array with names of input files */
	unsigned int infile_count; /* number of input files in infiles array */
	int oflags; /* Open flags to change behavior of syscall */
	mode_t mode; /* default permission mode for newly created outfile */
	unsigned int flags; /* special flags to change behavior of syscall */
};

int main(int argc, char *argv[])
{
	int i = 0; /* general index parameter */
	int rc; /* return value from syscall */
	void *dummy = (void *)&i;

	rc = syscall(__NR_xjob, dummy, 1);

	if (errno != 0) {
		printf("ERROR = %s,\n", strerror(errno));
		printf("ERROR_NUMBER = %d\n", errno);
	}
	exit(rc);
}
