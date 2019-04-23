#pragma once
#include <linux/hashtable.h>
#include <linux/bitmap.h>
#define FLS_SIZE 4096


//Singola fiber
struct Fiber{
	struct hlist_node node;
	pid_t id;
	pid_t runner;
	int running;
	struct pt_regs regs;
	struct fpu fpu;
	spinlock_t lock_fiber;
    unsigned long flags;
    
	long long fls[FLS_SIZE];
	DECLARE_BITMAP(bitmap_fls, FLS_SIZE);
    
	unsigned long exec_time;
	unsigned long last_activation_time;

	unsigned long correct_counter;
	atomic_long_t failed_counter;
	void* entry_point;
	pid_t creator;
};


struct Fiber_Stuff{
	struct pid_entry* fiber_base_stuff;
	int len_fiber_stuff;
};

//Lista di gestori
struct Fiber_Processi{
	struct hlist_node node;
    DECLARE_HASHTABLE(listafiber, 10);
	pid_t last_fib_id;

	struct Fiber_Stuff fiber_stuff;
	pid_t id;
	unsigned int counter_proc_look;
	unsigned int counter_proc_read;
	
};

//
struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        unsigned long stack_size;
        void *start_function_address;
        void *start_function_parameters;
        pid_t fiber_id;
        long fls_index;
        long long fls_value;
		unsigned long buffer;
        unsigned long alloc_size;
};

//Strutture kernel che non sono visibili


union proc_op {
	int (*proc_get_link)(struct dentry *, struct path *);
	int (*proc_show)(struct seq_file *m,
		struct pid_namespace *ns, struct pid *pid,
		struct task_struct *task);
};

struct pid_entry {
	const char *name;
	unsigned int len;
	umode_t mode;
	const struct inode_operations *iop;
	const struct file_operations *fop;
	union proc_op op;
};
