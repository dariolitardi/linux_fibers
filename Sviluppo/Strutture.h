struct Fiber_Processi{
	pid_t id;
	struct Lista_Fiber gestore_fiber;
	struct Fiber_Processi next;
};

struct Fiber{
	struct pt_regs* regs;
	// add FPU
	long* fls;
};

struct Lista_Fiber{
	bool running;
	struct Fiber* fiber;
	struct Lista_Fiber next;
};
