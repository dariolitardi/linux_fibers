#pragma once
#include <linux/hashtable.h>
#include <linux/bitmap.h>

#define FLS_SIZE 16


//Singola fiber
struct Fiber{
	struct hlist_node node;
	pid_t id;
	struct pt_regs* regs;
	struct fpu* fpu;
    unsigned long flags;
    spinlock_t lock_fiber;

	long long* fls;
	unsigned long *bitmap_fls;
    
	unsigned long exec_time;
	unsigned long last_activation_time;

	unsigned long correct_counter;
	int failed_counter;
	void* entry_point;
	pid_t creator;
	struct Thread* selected_thread;
};


struct Fiber_Stuff{
	struct pid_entry* fiber_base_stuff;
	int len_fiber_stuff;
};

struct Thread {
        pid_t id; 
        struct hlist_node node;
        struct Fiber *runner;
        struct Fiber_Processi *parent_process;

};


//Lista di gestori
struct Fiber_Processi{
	struct hlist_node node;
	DECLARE_HASHTABLE(listathread, 3);
    DECLARE_HASHTABLE(listafiber, 9);

	pid_t last_fib_id;

	spinlock_t lock_fiber;

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
