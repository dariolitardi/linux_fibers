#pragma once

//Singola fiber
struct Fiber{
	struct pt_regs* regs;
	// add FPU
	long* fls;
};

//Lista di fiber locali al processo
struct Lista_Fiber{
	bool running;
	unsigned long id;
	pid_t runner;
	struct Fiber* fiber;
	struct Lista_Fiber* next;
};

//Lista di gestori
struct Fiber_Processi{
	pid_t id;
	struct Lista_Fiber* lista_fiber;
	struct Fiber_Processi* next;
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
