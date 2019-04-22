#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#define DEV_NAME "fib_device"

#define FIB_FLS_ALLOC 1
#define FIB_FLS_GET	404
#define FIB_FLS_SET	3
#define FIB_FLS_DEALLOC	4

#define FIB_CONVERT	5
#define FIB_CREATE 6
#define FIB_SWITCH_TO 7

int IFACE_FIBER_DEV;

struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        unsigned long stack_size;
        void *start_function_address;
        void *start_function_parameters;
        pid_t fiber_id;
        long fls_index;
        long  long fls_value;
		unsigned long buffer;
        unsigned long alloc_size;
};


long fib_fls_alloc();
bool fib_fls_dealloc(long);
void fib_fls_set(long, long long);
long long fib_fls_get(long);
pid_t fib_convert();
pid_t fib_create(void*, void *, unsigned long);
long  fib_switch_to(pid_t);

