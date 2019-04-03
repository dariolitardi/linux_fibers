#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#pragma once

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>

#define DEV_NAME "fib_device"

#define FIB_FLS_ALLOC	1
#define FIB_FLS_GET	2
#define FIB_FLS_SET	3
#define FIB_FLS_DEALLOC	4

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7

int IFACE_FIBER_DEV;

struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        unsigned long stack_size;
        void *start_function_address;
        void *start_function_parameters;
        unsigned long fiber_id;
        unsigned long fls_index;
        long fls_value;
		unsigned long buffer;
        unsigned long alloc_size;
};

int fib_fls_alloc();
void fib_fls_dealloc();
void fib_fls_set(unsigned long pos, long val);
long fib_fls_get(unsigned long pos);

unsigned long fib_convert();
unsigned long fib_create(void* func, void *parameters, unsigned long stack_size);
int fib_switch_to(unsigned long id);
