#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define DEV_NAME "fib_device"

#define FIB_FLS_ALLOC	1
#define FIB_FLS_GET	2
#define FIB_FLS_SET	3
#define FIB_FLS_DEALLOC	4

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7

int fd;
unsigned long stmp1;
unsigned long stmp2;

typedef void(*user_function_t)(void* param);
struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        unsigned long stack_size;
        void *start_function_address;
        void *start_function_arguments;
        unsigned long fiber_id;
        unsigned long fls_index;
        long fls_value;
		unsigned long buffer;
        unsigned long alloc_size;
};


void stampa1(void* parameters){
	int i;
	ioctl(fd, FIB_FLS_ALLOC,NULL);
	struct fiber_arguments fa;
	
	for(i=0;i<1000;++i){
		printf("STAMPA1-%d\n",i);
		fa.fls_index=i;
		fa.fls_value=i;
		ioctl(fd, FIB_FLS_SET, &fa);
		ioctl(fd, FIB_SWITCH_TO, stmp2);
		ioctl(fd, FIB_FLS_GET, &fa);
		printf("STAMPA1_FLS-%l\n",fa.fls_value);
	}
	while(1);

	exit(0);
}

void stampa2(void* parameters){
	int i;
	ioctl(fd, FIB_FLS_ALLOC,NULL);
	struct fiber_arguments fa;
	
	for(i=0;i<1000;++i){
		printf("STAMPA2-%d\n",i);
		fa.fls_index=i;
		fa.fls_value=i;
		ioctl(fd, FIB_FLS_SET, &fa);
		ioctl(fd, FIB_SWITCH_TO, stmp1);
		ioctl(fd, FIB_FLS_GET, &fa);
		printf("STAMPA2_FLS-%l\n",fa.fls_value);
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
	fd = open("/dev/" DEV_NAME, O_RDWR);
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
	struct fiber_arguments fa2;
    fa2.start_function_address = stampa2;
    fa2.stack_pointer= malloc(4096*sizeof(char));
	fa2.stack_size=4096*sizeof(char);

 	printf("Test fib_create 1\n");
	ioctl(fd, FIB_CREATE, &fa1);
	printf("Test fib_create1 id %ul\n", fa1.fiber_id);
	
	printf("Test fib_create 2\n");
	ioctl(fd, FIB_CREATE, &fa2);
	printf("Test fib_create2 id %ul\n", fa2.fiber_id);

	stmp1 = fa1.fiber_id;	
	stmp2 = fa2.fiber_id;
	printf("Test stmp1 %p\n", fa1.start_function_address);
	printf("Test stmp2 %p\n", fa2.start_function_address);
	printf("Test stampa1 %p\n", stampa1);
	printf("Test stampa2 %p\n", stampa2);
	int i;
	printf("Test Thread\n");

	pthread_t threads[11];
	for (i = 0; i < 10; i++) {
       pthread_create(&threads[i], NULL, thread_function, i);
    }
    printf("Test Join\n");

	for (i = 0; i < 10; i++) {
                pthread_join(threads[i], NULL);
        }
	
	printf("Test Close\n");
	close(fd);
}
