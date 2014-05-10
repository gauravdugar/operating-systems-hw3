obj-m += sys_xjob_mod.o

sys_xjob_mod-y += sys_xjob.o sioq.o netlinkkernel.o

all: xjob user_cancel_job user_submit_job netlinkuser test

test: user_submit_job.o netlinkuser.o
	gcc -o test user_submit_job.o netlinkuser.o

netlinkuser: netlinkuser.c
	gcc -Wall -Werror netlinkuser.c -c -o netlinkuser.o

user_cancel_job: user_cancel_job.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include user_cancel_job.c -g -o user_cancel_job

user_submit_job: user_submit_job.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include user_submit_job.c -c -g -o user_submit_job.o

xjob:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f user_submit_job
	rm -f user_cancel_job
	rm -f test
