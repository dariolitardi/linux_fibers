#pragma once

//Singola fiber
struct Fiber{
	struct pt_regs regs;
	struct fpu fpu;
	long* fls;
	unsigned long exec_time;
	unsigned long last_activation_time;

	unsigned long correct_counter;
	unsigned long failed_counter;
	void* entry_point;
	pid_t creator;
};


//Lista di fiber locali al processo
struct Lista_Fiber{
	struct Fiber* fiber;
	struct Lista_Fiber* next;
	unsigned long id;
	pid_t runner;
	bool running;

};
struct Fiber_Stuff{
	struct pid_entry* fiber_base_stuff;
	int len_fiber_stuff;
};

//Lista di gestori
struct Fiber_Processi{
	struct Lista_Fiber* lista_fiber;
	struct Fiber_Processi* next;
	struct Fiber_Stuff fiber_stuff;
	pid_t id;
	spinlock_t lock_fib_list;
	unsigned long flags;
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
        void *start_function_arguments;
        unsigned long fiber_id;
        unsigned long fls_index;
        long fls_value;
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
/*
struct timespec{
	time_t tv_sec;
	long tv_nsec;
};
*/
