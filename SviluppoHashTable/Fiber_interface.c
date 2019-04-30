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
	

	long index=ioctl(IFACE_FIBER_DEV, FIB_FLS_ALLOC, NULL);
	fprintf(stderr, "ALLOC 1 %ld \n",index);
	return index;
}

bool fib_fls_dealloc(long index){
	struct fiber_arguments param;
	param.fls_index = index;
	
	return ioctl(IFACE_FIBER_DEV, FIB_FLS_DEALLOC, &param);
}

void fib_fls_set(long pos, long long val){

	struct fiber_arguments param={
		.fls_index = pos,
        .fls_value = val,
	};
	fprintf(stderr, "SET 0 %ld %lld \n",param.fls_index ,param.fls_value);

	ioctl(IFACE_FIBER_DEV, FIB_FLS_SET, &param);
	fprintf(stderr, "SET 1 %ld %lld \n",param.fls_index ,param.fls_value);
	return;
}

long long fib_fls_get(long pos){
	long long ret;
	struct fiber_arguments param={
		.fls_index = pos,
	};
	
	fprintf(stderr, "GET 0 %ld %lld \n",param.fls_index ,param.fls_value);
	
	ioctl(IFACE_FIBER_DEV, 404, &param);
	
	ret = param.fls_value;
	fprintf(stderr, "GET 1 %ld %lld \n",param.fls_index ,param.fls_value);

	return ret;
}

pid_t fib_convert(){
		pid_t id=(pid_t)ioctl(IFACE_FIBER_DEV, FIB_CONVERT, 0);
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
	param.start_function_parameters=parameters;
	ioctl(IFACE_FIBER_DEV, FIB_CREATE, &param);
	id=param.fiber_id;
	fprintf(stderr,"CREATE %d \n",id);

	return (pid_t) id;
}

long  fib_switch_to(pid_t id){
	fprintf(stderr,"SWITCH %d\n",id);
	  struct fiber_arguments param = {
                .fiber_id = id,
        };

	return (long) ioctl(IFACE_FIBER_DEV, FIB_SWITCH_TO, &param);
}
