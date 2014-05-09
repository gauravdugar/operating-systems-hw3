#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define __NR_xconcat	349	/* our private syscall number */

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
	struct argStruct argToSys; /* struct to store values for syscall */
	int i = 0; /* general index parameter */
	int flags = 0; /* options given by user */
	int x = 0; /* mode ARG in octal format */
	char *ptr; /* pointer to check successful string to octal conversion */
	int pnMode = 0; /* flag to specify either P or N */
	char *p1 = "N and P cannot be passed together\n";

	argToSys.oflags = 0;
	argToSys.flags = 0x00;
	argToSys.mode = 0;

	/*
	* This while loop traverses through all options given by user,
	* and sets value in structure accordingly.
	* It also checks validity of argument and exits accordingly.
	*/
	while ((flags = getopt(argc, argv, "acteANPhm:")) != -1) {
		switch (flags) {
		case 'a':
			argToSys.oflags |= O_APPEND;
			break;
		case 'c':
			argToSys.oflags |= O_CREAT;
			break;
		case 't':
			argToSys.oflags |= O_TRUNC;
			break;
		case 'e':
			argToSys.oflags |= O_EXCL;
			break;
		case 'A':
			argToSys.flags |= 0x04;
			break;
		case 'N':
			if (pnMode == 0) {
				argToSys.flags |= 0x01;
				pnMode = 1;
			} else {
				fprintf(stderr, p1);
				exit(1);
			}
			break;
		case 'P':
			if (pnMode == 0) {
				argToSys.flags |= 0x02;
				pnMode = 1;
			} else {
				fprintf(stderr, p1);
				exit(1);
			}
			break;
		case 'h':
			printf("ABOUT:\n\t");
			printf("It reads and writes the input files in ");
			printf("outfile with certain options as provided by ");
			printf("user.\n\nSYNOPSIS:\n\t./xhw1 [OPTION] ");
			printf("[OUTFILE] [INFILE_1] [INFILE_2]...\n\n");
			printf("DESCRIPTION:\n\tMaximum input files can be ");
			printf("10.\n\tAtleast 1 infile and exactly 1 out");
			printf("file must be given.\n\nOPTION:\n\t");
			printf("-a: append mode\n\t-c: create outfile\n\t");
			printf("-t: truncate mode\n\t-e: excl mode\n\t");
			printf("-A: Atomic concat mode\n\t");
			printf("-N: return num-files instead of num-bytes ");
			printf("written\n\t");
			printf("-P: return percentage of data written out\n\t");
			printf("-m ARG: set default mode to ARG (e.g., octal ");
			printf("755)\n\t-h: print short usage string\n\n");
			exit(0);
			break;
		case 'm':
			x = strtol(optarg, &ptr, 8);
			if (!*ptr)
				argToSys.mode = x;
			else {
				fprintf(stderr,
				"Wrong arguments for mode : %s\n", ptr);
				exit(1);
			}
			break;
		case '?':
			if (optopt == 'm')
				fprintf(stderr,
				"Option -%c requires an argument.\n",
				optopt);
			else
				fprintf(stderr,
				"Unknown option `-%c'.\n", optopt);
			exit(1);
		default:
			printf("Invalid Argument..!!,\n");
			exit(1);
		}
	}

	/* Validation checks for outfile and infile count */
	if (argc - optind == 0) {
		printf("No Outfile provided..!!\n\n");
		exit(0);
	}
	if (argc - optind - 1 == 0) {
		printf("No Infiles provided..!!\n\n");
		exit(0);
	}
	if (argc - optind - 1 > 10) {
		printf("More than 10 input files provided, Exiting..!\n\n");
		exit(0);
	}
	const char *inputArray[argc - optind - 1];

	for (i = optind + 1; i < argc; i++)
		inputArray[i - optind - 1] = argv[i];

	argToSys.outfile = argv[optind];
	argToSys.infiles = (const char **)&inputArray;
	argToSys.infile_count = argc - optind - 1;

	int rc; /* return value from syscall */

	/* wrapping of struct in void pointer */
	void *dummy = (void *)&argToSys;

	/* Actual system call */
	rc = syscall(__NR_xconcat, dummy, sizeof(argToSys));

	if (errno != 0) {
		printf("ERROR = %s,\n", strerror(errno));
		printf("ERROR_NUMBER = %d\n", errno);
	}

	/* On successful execution, prints information according to flags */
	if (errno == 0) {
		if ((argToSys.flags & 0x04) != 0)
			printf("Atomic Mode\n");
		if ((argToSys.flags & 0x01) != 0)
			printf("Number of files written = %d\n", rc);
		else {
			if ((argToSys.flags & 0x02) != 0)
				printf("Percentage of data written = %d\n",
				rc);
			else
				printf("Number of bytes written = %d\n", rc);
		}
	}
	exit(rc);
}
