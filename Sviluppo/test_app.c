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

 
int main()
{
        int fd;
        int32_t value, number;
 
        printf("Test Open\n");
        fd = open("/dev/" DEV_NAME, O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
 
        printf("Test switch to fiber\n");
        ioctl(fd, WR_VALUE, (int32_t*) &number); 
 
        printf("Value is %d\n", value);
 
        printf("Test Close\n");
        close(fd);
}
