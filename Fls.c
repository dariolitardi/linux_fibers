#include "Fls.h"

int* FlsAlloc(){
	return kmalloc(sizeof(long)*1024);
}

int FlsFree(long id){
	return kfree(id);
}

long FlsGetValue(long id, long pos){
	return id[pos];
}

long FlsSetValue(long id, long pos, long val){
	id[pos] = val;
	return val;
}
