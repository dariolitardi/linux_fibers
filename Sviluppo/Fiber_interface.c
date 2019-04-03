#include "Fiber_interface.h"

__attribute__((constructor)) void fiber_init(){
	IFACE_FIBER_DEV = open("/dev/" DEV_NAME,O_RDONLY);
	return;
}

__attribute__((destructor)) void close_fiberlib(){
    close(IFACE_FIBER_DEV);
    return;
}

int fib_fls_alloc(){
	ioctl(IFACE_FIBER_DEV, FIB_FLS_ALLOC, NULL);
	return 0;
}

void fib_fls_dealloc(){
	ioctl(IFACE_FIBER_DEV, FIB_FLS_DEALLOC, NULL);
}

void fib_fls_set(unsigned long pos, long val){
	struct fiber_arguments* param = (struct fiber_arguments*) malloc(sizeof(struct fiber_arguments));
	param->fls_index = pos;
	param->fls_value = val;
	ioctl(IFACE_FIBER_DEV, FIB_FLS_SET, param);
	free(param);
}

long fib_fls_get(unsigned long pos){
	long ret;
	struct fiber_arguments* param = (struct fiber_arguments*) malloc(sizeof(struct fiber_arguments));
	param->fls_index = pos;
	ioctl(IFACE_FIBER_DEV, FIB_FLS_GET, pos);
	ret = param->fls_value;
	free(param);
	return ret;
}

unsigned long fib_convert(){
		return ioctl(IFACE_FIBER_DEV, FIB_CONVERT, NULL);
}

unsigned long  fib_create(void* func, void *parameters, unsigned long stack_size){
	void* stack_pointer=(void*)malloc(stack_size);
	unsigned long id;
	struct fiber_arguments* param = (struct fiber_arguments*) malloc(sizeof(struct fiber_arguments));
	param->start_function_address = func;
	param->stack_pointer = stack_pointer;
	param->stack_size = stack_size;
	param->start_function_parameters=parameters;
	ioctl(IFACE_FIBER_DEV, FIB_CREATE, param);
	id=param->fiber_id;
	free(param);
	return id;
}

int fib_switch_to(unsigned long id){
	return (int) ioctl(IFACE_FIBER_DEV, FIB_SWITCH_TO, id);
}
