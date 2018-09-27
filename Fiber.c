#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_INFO
#include "Strutture.h"
#include "Fls.h"

int init_module(void){
	printk(KERN_INFO "CARICATO FIBER.KO\n");
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "RIMOSSO FIBER.KO\n");
}

int ConvertThreadToFiber(){
	//Controlla se sto già runnando una fiber
	return 0;
}

long CreateFiber(void* code){ //Parametri?
	//Crea le strutture
	//Popola le strutture
	//Ritorna il fiber id
}

void SwitchToFiber(long id){
	//Controlla se la fiber "id" esiste
	//Recupera le strutture
	//Salva lo stato, ripristina il nuovo stato
}
