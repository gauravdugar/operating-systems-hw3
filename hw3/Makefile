obj-m += sys_xjob_mod.o

sys_xjob_mod-y += sys_xjob.o sioq.o netlinkkernel.o crypto.o file.o

all: xjob user_cancel_job netlinkuser crypt doCrypt checksum doCheck user_list_job

doCrypt: crypt.o netlinkuser.o
	gcc -o doCrypt crypt.o netlinkuser.o

doCheck: checksum.o netlinkuser.o
	gcc -o doCheck checksum.o netlinkuser.o

netlinkuser: netlinkuser.c
	gcc -Wall -Werror netlinkuser.c -c -o netlinkuser.o

user_cancel_job: user_cancel_job.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include user_cancel_job.c -g -o user_cancel_job

crypt: crypt.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include crypt.c -c -g -o crypt.o

xjob:
	KCPPFLAGS="${ARGS}" make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

checksum: checksum.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include checksum.c -c -g -o checksum.o

user_list_job: user_list_job.c
	gcc -Wall -Werror -I/lib/modules/$(shell uname -r)/build/arch/x86/include user_list_job.c -g -o user_list_job

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f crypt
	rm -f user_cancel_job
	rm -f doCrypt
	rm -f doCheck
	rm -f user_list_job
