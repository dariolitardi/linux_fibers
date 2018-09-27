struct Fiber{
	struct pt_regs* regs;
	// add FPU
	long* fls;
};

struct Nodo_Fiber{
	bool running;
	struct Fiber* fiber;
	struct Nodo_Fiber next;
};
