struct jobs {
	int work_type;
	const char **infiles;
	int type;
	void *config;
	int flags; /*flags to change behavior of syscall*/
};

static const int SUBMIT = 1;
static const int CANCEL = 2;
static const int LIST = 3;
