#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define DEV_NAME "fib_device"

#define FIB_FLS_ALLOC	1
#define FIB_FLS_GET	2
#define FIB_FLS_SET	3
#define FIB_FLS_DEALLOC	4

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7

int fd;
pid_t stmp1;
pid_t stmp2;
pid_t stmp3;



typedef void(*user_function_t)(void* param);
struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        unsigned long stack_size;
        void *start_function_address;
        void *start_function_parameters;
        unsigned long fiber_id;
        unsigned long fls_index;
        long long fls_value;
		unsigned long buffer;
        unsigned long alloc_size;
};


void stampa1(void* parameters){
	int i;
	long ret = 2;
	struct fiber_arguments fa;
	
	for(i=0;i<100;++i){
		printf("STAMPA1-%d\n",i);
		fa.fls_index=(long)i;
		fa.fls_value=(long long)i;
		ioctl(fd, FIB_FLS_SET, &fa);
		do{
			ret = ioctl(fd, FIB_SWITCH_TO, stmp2);
			printf("SWITCH RETURNED %ld\n",ret);
			if(ret!=0){
				sleep(10);

			}		}while(ret!=0);
		ioctl(fd, FIB_FLS_GET, &fa);
		printf("STAMPA1_FLS-%llu\n",fa.fls_value);
		
	}
	while(1);

	exit(0);
}

void stampa2(void* parameters){
	int i;
	long ret;
	ioctl(fd, FIB_FLS_ALLOC,NULL);
	struct fiber_arguments fa;
	
	for(i=0;i<100;++i){
		printf("STAMPA2-%d\n",i);
		fa.fls_index=(long)i;
		fa.fls_value=(long long)i;
		ioctl(fd, FIB_FLS_SET, &fa);
		do{	
			
			ret = ioctl(fd, FIB_SWITCH_TO, stmp3);
			printf("SWITCH RETURNED %ld\n",ret);
			if(ret!=0){
				sleep(10);

			}

		}while(ret!=0);
		ioctl(fd, FIB_FLS_GET, &fa);
		printf("STAMPA2_FLS-%llu\n",fa.fls_value);
	}
	while(1);
	exit(0);
}


void stampa3(void* parameters){
	int i;
	long ret;
	ioctl(fd, FIB_FLS_ALLOC,NULL);
	struct fiber_arguments fa;
	
	for(i=0;i<100;++i){
		printf("STAMPA3-%d\n",i);
		fa.fls_index=(long)i;
		fa.fls_value=(long long)i;
		ioctl(fd, FIB_FLS_SET, &fa);
		do{
			ret = ioctl(fd, FIB_SWITCH_TO, stmp1);
			printf("SWITCH RETURNED %ld\n",ret);
			if(ret!=0){
				sleep(10);

			}
		}while(ret!=0);
		ioctl(fd, FIB_FLS_GET, &fa);
		printf("STAMPA3_FLS-%llu\n",fa.fls_value);
	}
	while(1);
	exit(0);
}

void* thread_function(void* param){
	printf("Ingresso thread %d\n", (int) param);

	printf("Test thread fib_convert\n");
	ioctl(fd, FIB_CONVERT, (int32_t*) 0);
	printf("Test thread fib_switch_to\n");
	ioctl(fd, FIB_SWITCH_TO, stmp1);
	printf("Uscita thread\n");
}
 
int main()
{
	printf("Test Open\n");
	//fd = open("/dev/" DEV_NAME, O_RDWR);
	if(fd < 0) {
		printf("Cannot open device file...\n");
		return 0;
	}
	printf("PID: %d\n",getpid());

	printf("Test fib_convert\n");
	ioctl(fd, FIB_CONVERT, (int32_t*) 0);
	printf("Test main %p\n",main);

	printf("Test fib_create\n");
	
	//PASSA PARAM CON STRUCT
 	struct fiber_arguments fa1;
    fa1.start_function_address = stampa1;
	fa1.stack_pointer= malloc(4096*sizeof(char));
	fa1.stack_size=4096*sizeof(char);
	fa1.start_function_parameters=stampa1;

	struct fiber_arguments fa2;
    fa2.start_function_address = stampa2;
    fa2.stack_pointer= malloc(4096*sizeof(char));
	fa2.stack_size=4096*sizeof(char);
	fa2.start_function_parameters=stampa2;

	struct fiber_arguments fa3;
    fa3.start_function_address = stampa3;
    fa3.stack_pointer= malloc(4096*sizeof(char));
	fa3.stack_size=4096*sizeof(char);
	fa3.start_function_parameters=stampa3;
 	
 	printf("Test fib_create 1\n");
	ioctl(fd, FIB_CREATE, &fa1);
	
	printf("Test fib_create1 id %d\n", fa1.fiber_id);
	
	printf("Test fib_create 2\n");
	ioctl(fd, FIB_CREATE, &fa2);
	printf("Test fib_create2 id %d\n", fa2.fiber_id);

	printf("Test fib_create 3\n");
	ioctl(fd, FIB_CREATE, &fa3);
	printf("Test fib_create3 id %d\n", fa3.fiber_id);
	
	stmp1 = fa1.fiber_id;	
	stmp2 = fa2.fiber_id;
	stmp3 = fa3.fiber_id;
	
	int i;
	printf("Test Thread\n");
	pthread_t threads[11];
	for (i = 0; i < 2; i++) {
		pthread_create(&threads[i], NULL, thread_function, i);
    }
    
    printf("Test Join\n");
	for (i = 0; i < 2; i++) {
		pthread_join(threads[i], NULL);
    }
	
	printf("Test Close\n");
	close(fd);
}
