#pragma once
#include <linux/hashtable.h>
#include <linux/bitmap.h>

#define FLS_SIZE 128


//Singola fiber
struct Fiber{
	struct hlist_node node;
	struct pt_regs* regs;
	struct fpu* fpu;
	void* entry_point;
	struct Thread* selected_thread;
    spinlock_t lock_fiber;
	long long* fls;
	unsigned long *bitmap_fls;    
	unsigned long exec_time;
	unsigned long last_activation_time;
	unsigned long correct_counter;
	pid_t creator;
	pid_t id;
	int failed_counter;
};


struct Fiber_Stuff{
	struct pid_entry* fiber_base_stuff;
	int len_fiber_stuff;
};

struct Thread {
        struct hlist_node node;
        struct Fiber *runner;
        pid_t id; 

};

//Process
struct Process{
	struct hlist_node node;
	DECLARE_HASHTABLE(listathread, 10);
    DECLARE_HASHTABLE(listafiber, 10);
	struct Fiber_Stuff fiber_stuff;
	pid_t last_fib_id;
	pid_t id;
};

//
struct fiber_arguments {
        //this struct is used in order to pass arguments to the IOCTL call
        //packing all we need in a void*
        void *stack_pointer;
        void *start_function_address;
        void *start_function_parameters;
        long long fls_value;
        long fls_index;
		unsigned long stack_size;
		unsigned long buffer;
        unsigned long alloc_size;
        pid_t fiber_id;
		
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
