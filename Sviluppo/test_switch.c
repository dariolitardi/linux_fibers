#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
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
        user_function_t start_function_address;
        void *start_function_arguments;
        pid_t fiber_id;
        long index;
        unsigned long buffer;
        unsigned long alloc_size;
};

void stampa1(void* parameters){
	printf("STAMPA1");
	ioctl(fd, FIB_SWITCH_TO, stmp2);
}

void stampa2(void* parameters){
	printf("STAMPA2");
	ioctl(fd, FIB_SWITCH_TO, stmp1);
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
	
	printf("Test fib_create\n");
	//PASSA PARAM CON STRUCT
 	struct fiber_arguments fa1;

    	fa1.start_function_address = stampa1;
 	
	struct fiber_arguments fa2;

    	fa2.start_function_address = stampa2;
 	
	ioctl(fd, FIB_CREATE, &fa1);
	
	printf("Test fib_create\n");
	ioctl(fd, FIB_CREATE, &fa2);
	
	printf("Test fib_switch_to\n");
	ioctl(fd, FIB_SWITCH_TO, stmp1);
	
	printf("Test Close\n");
	close(fd);
}
