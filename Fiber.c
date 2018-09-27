#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_INFO
#include <linux/init.h>		// Macro __init,__exit
//#include <sys/ioctl.h>
#include <linux/fs.h>
#include "Strutture.h"
#include "Fls.h"

const struct file_operations fops ={
	.open		= NULL, //try_module_get(THIS)
	.release	= NULL, //module_put(THIS)
	.read		= NULL,
	.write		= NULL,
};

static int __init ingresso(void){
	int num = register_chrdev(666,"LinuxFibers",fops);
	printk(KERN_INFO "CARICATO FIBER.KO\n");
	return 0;
}

static void __exit uscita(void){
	printk(KERN_INFO "RIMOSSO FIBER.KO\n");
}

module_init(ingresso);
module_exit(uscita);

// Funzioni

static int ConvertThreadToFiber(){
	//Controlla se sto già runnando una fiber
	//Se sei il primo tira su le strutture di controllo
	//Chiama CreateFiber
	//Switcha sul nuovo fiber
	return 0;
}

static long CreateFiber(void* code){ //Parametri?
	//Crea le strutture
	//Popola le strutture
	//Crea entry su proc
	//Ritorna il fiber id
}

static void SwitchToFiber(long id){
	//Controlla se la fiber "id" esiste
	//Recupera le strutture
	//Salva lo stato, ripristina il nuovo stato
}
