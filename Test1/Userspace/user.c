#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

int main(){
	int fd = open("/dev/MyTest",0);
	ioctl(fd,1,NULL);
	ioctl(fd,2,NULL);
	ioctl(fd,1,NULL);
	return 0;
}
