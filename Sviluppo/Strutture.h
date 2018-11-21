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
struct pid_entry {
	const char *name;
	unsigned int len;
	umode_t mode;
	const struct inode_operations *iop;
	const struct file_operations *fop;
	union proc_op op;
};
