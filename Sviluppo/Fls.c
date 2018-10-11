#include "Fls.h"

unsigned long FlsAlloc(){
	return (unsigned long) kmalloc(sizeof(long)*1024);
}

void FlsFree(unsigned long id){
	return kfree(id);
}

long FlsGetValue(unsigned long id, unsigned long pos){
	return id[pos];
}

long FlsSetValue(unsigned long id, unsigned long pos, long val){
	id[pos] = val;
	return val;
}
