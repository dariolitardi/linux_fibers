#include "Fiber_interface.h"
#include <libexplain/ioctl.h>


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

void fib_fls_set(long pos, long val){

	struct fiber_arguments param;
	memset(&param,0,sizeof(struct fiber_arguments));
	
	param.fls_index = pos;
	param.fls_value = val;
	ioctl(IFACE_FIBER_DEV, FIB_FLS_SET, &param);
	return;
}

long fib_fls_get(long pos){
	struct fiber_arguments param;
	memset(&param,0,sizeof(struct fiber_arguments));
	long ret;
	
	param.fls_index = pos;
	
	ioctl(IFACE_FIBER_DEV, 404, &param);
	
	ret = param.fls_value;
	return ret;
}

/*
long long fib_fls_get(long pos){
	long long ret;
	struct fiber_arguments param;
	memset(&param,0,sizeof(struct fiber_arguments));
	
	fprintf(stderr,"POS GET %ld \n",pos);
	fprintf(stderr, "FLS GET DEV ID %d\n",IFACE_FIBER_DEV);
	param.fls_index = pos;
	fprintf(stderr, "GET IOCTL CALL\n");
	fprintf(stderr, "GET PARAM IDX %d\n",param.fls_index);
	//long prova = ioctl(IFACE_FIBER_DEV, FIB_FLS_GET, &param);
	long prova = ioctl(IFACE_FIBER_DEV, 2, &param);
	fprintf(stderr, "GET IOCTL RETURN %ld\n",prova);
	
	if (prova < 0){
		fprintf(stderr, "%s\n", explain_ioctl(IFACE_FIBER_DEV, 2, &param));
	}
	
	ret = param.fls_value;
	fprintf(stderr,"GET RETUNED %llu \n",ret);

	return ret;
}
*/
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
