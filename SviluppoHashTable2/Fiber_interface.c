#include "Fiber_interface.h"


__attribute__((constructor)) void fiber_init(){

	IFACE_FIBER_DEV = open("/dev/" DEV_NAME, O_RDONLY);

	return;
}

__attribute__((destructor)) void close_fiberlib(){
	close(IFACE_FIBER_DEV);
    return;
}

long fib_fls_alloc(){
	return (long)ioctl(IFACE_FIBER_DEV, FIB_FLS_ALLOC, NULL);
}

bool fib_fls_dealloc(long index){
	struct fiber_arguments param;
	param.fls_index = index;
	
	return ioctl(IFACE_FIBER_DEV, FIB_FLS_DEALLOC, &param);
}

void fib_fls_set(long pos, long long val){

	struct fiber_arguments param;
	memset(&param,0,sizeof(struct fiber_arguments));
	
	param.fls_index = pos;
	param.fls_value = val;
	ioctl(IFACE_FIBER_DEV, FIB_FLS_SET, &param);
	fprintf(stderr, "SET %ld %ld \n",param.fls_index ,param.fls_value);
	return;
}

long long fib_fls_get(long pos){
	
	struct fiber_arguments param;
	memset(&param,0,sizeof(struct fiber_arguments));
	long long ret=0;
	
	param.fls_index = pos;
	param.fls_value = (unsigned long)&ret;
	ioctl(IFACE_FIBER_DEV, 404, &param);
	fprintf(stderr, "GET %ld \n",ret);

	return ret;
}

pid_t fib_convert(){
		pid_t id=(pid_t)ioctl(IFACE_FIBER_DEV, FIB_CONVERT, NULL);
		fprintf(stderr, "CONVERT %d \n",id);

		return id;
}

pid_t fib_create(void* func, void *parameters, unsigned long stack_size){

	void* stack_pointer=(void*)malloc(stack_size);
	pid_t id;
	struct fiber_arguments param;
	param.start_function_address = func;
	param.stack_pointer = stack_pointer;
	param.stack_size = stack_size;
	param.fiber_id=0;
	param.start_function_parameters=parameters;
	ioctl(IFACE_FIBER_DEV, FIB_CREATE, &param);
	id=param.fiber_id;
	fprintf(stderr,"CREATE %d \n",id);

	return id;
}

long  fib_switch_to(pid_t id){
	fprintf(stderr,"SWITCH %d\n",id);

	return (long) ioctl(IFACE_FIBER_DEV, FIB_SWITCH_TO, id);
}
