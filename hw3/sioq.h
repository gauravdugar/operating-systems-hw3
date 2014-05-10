struct sioq_args {
	struct completion comp;
	struct work_struct work;
	int err;
	int pid;
	int id;
	void *ret;
};

extern int init_sioq(void);
extern void stop_sioq(void);
extern void run_sioq(work_func_t func, struct sioq_args *args);
extern int cancel_work(int id);
extern void testPrint(struct work_struct *work);
