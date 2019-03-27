#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/uaccess.h>              //copy_to/from_user()
#include <linux/ioctl.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/dirent.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/kprobes.h>
#include <asm/fpu/types.h>
#include <asm/fpu/internal.h>
#include <linux/slab.h>
#include "Strutture.h"
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>   
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/kprobes.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/percpu-defs.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/kallsyms.h>
#include <linux/spinlock.h>
#define BUFSIZE  100

//NOTA
//TGID == PARENT TGID == PROCESS ID
//PID == THERAD ID

#define DEV_NAME "fib_device"
//#define WR_VALUE _IOW('a','a',int32_t*)
//#define RD_VALUE _IOR('a','b',int32_t*)

#define FIB_FLS_ALLOC	1
#define FIB_FLS_GET	2
#define FIB_FLS_SET	3
#define FIB_FLS_DEALLOC	4
#define FLS_SIZE 1024

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7

struct Fiber_Processi* Lista_Processi;
spinlock_t lock_lista_processi;
unsigned long flags;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev fib_cdev;


static int __init fib_driver_init(void);
static void __exit fib_driver_exit(void);
static int fib_open(struct inode *inode, struct file *file);
static int fib_release(struct inode *inode, struct file *file);
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos);
static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


static void fib_convert(void);
static struct Lista_Fiber* do_fib_create(void* func, void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo);
static struct Lista_Fiber* fib_create(void* func, void *stack_pointer, unsigned long stack_size);
static void fib_switch_to(unsigned long id);

static void flsAlloc(void);
static void flsFree(void);
static long flsGetValue(unsigned long pos);
static void flsSetValue(unsigned long pos, long val);

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.open           = fib_open,
	.unlocked_ioctl = fib_ioctl,
	.release        = fib_release,
};

static struct file_operations fops_proc = {
	.owner 			= THIS_MODULE,
	.read 			= myread,
};


//PID stuff with kprobe
static unsigned int counter = 0;
static struct pid_entry* new_tgid_base_stuff;


int Pre_Handler_Readdir(struct kprobe *p, struct pt_regs *regs){ 
	int i; 
	int size;
	struct pid_entry *tgid_base_stuff;
	int n;
	struct file *file=(void *) regs->di;
	struct inode *inode;
	long parent;
	pid_t id;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	kstrtol(file->f_path.dentry->d_parent->d_name.name,10, &parent);
	id=(pid_t)parent;

	printk(KERN_INFO "DEBUG PREREADDIR\n");
	printk(KERN_INFO "DEBUG PREREADDIR ID %d\n",id);
/*
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != id){
			lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG READDIR PROBLEMI\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
*/
	if(strcmp(file->f_path.dentry->d_name.name,"fibers")==0){
	
		//Itera sulla lista processi
		//Cerca processo corrente
		spin_lock_irqsave(&(lock_lista_processi), flags);

		while (lista_processi_iter != NULL && lista_processi_iter->id != id){
				lista_processi_iter = lista_processi_iter->next;
		}
		
		if (lista_processi_iter == NULL){
			printk(KERN_INFO "DEBUG READDIR PROBLEMI\n");
			spin_unlock_irqrestore(&(lock_lista_processi), flags);

			return 0;	//Problemi
		}
		
		if(lista_processi_iter->fiber_stuff.len_fiber_stuff==0){
			regs->cx=(unsigned long) lista_processi_iter->fiber_stuff.len_fiber_stuff;
			spin_unlock_irqrestore(&(lock_lista_processi), flags);

			return 0;
		}
		
		printk(KERN_INFO "DEBUG PREREADIR 0\n");

		regs->dx=(unsigned long) lista_processi_iter->fiber_stuff.fiber_base_stuff;
		regs->cx=(unsigned long) lista_processi_iter->fiber_stuff.len_fiber_stuff;
	
		spin_unlock_irqrestore(&(lock_lista_processi), flags);
	
	}else /*if(lista_processi_iter->counter_proc_read==0)*/{
	
		tgid_base_stuff = regs->dx;
		
		printk(KERN_INFO "DEBUG PREREADIR 1\n");

		n = (unsigned int) regs->cx;
		struct pid_entry fibers_entry;
		fibers_entry.name="fibers";
		fibers_entry.len=6;
		fibers_entry.mode=S_IFDIR|S_IRUGO|S_IXUGO;
		size=sizeof(fibers_entry)*(n+1);
		new_tgid_base_stuff =  kzalloc(size,GFP_KERNEL);
		
		for (i=0; i<n; i++){
			if (strcmp(tgid_base_stuff[i].name,"attr")==0){
				fibers_entry.iop = tgid_base_stuff[i].iop;//Questo coso serve a copiare i fop/iop dalla cartella attr
				fibers_entry.fop = tgid_base_stuff[i].fop;

			}
			new_tgid_base_stuff[i]=tgid_base_stuff[i];
		}
		new_tgid_base_stuff[i]=fibers_entry;
		
		
		inode=file->f_inode;
		inc_nlink(inode);
		
		regs->dx=(unsigned long) new_tgid_base_stuff;
		regs->cx=(unsigned long) n+1;

		//lista_processi_iter->counter_proc_read=1;
	}

	return 0;

} 


void Post_Handler_Readdir(struct kprobe *p, struct pt_regs *regs, unsigned long flags){ 
	struct file *file;
	struct inode *inode;
	if((unsigned long) new_tgid_base_stuff == regs->dx){
		file = (void *) regs->di;
		inode=file->f_inode;
		drop_nlink(inode);
	}
		
}

int Pre_Handler_Lookup(struct kprobe *p, struct pt_regs *regs){
	int n;
	int i; 
	int size;
	struct pid_entry *tgid_base_stuff;
	struct dentry *dentry= (void *) regs->si;
	long parent;
	pid_t id;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	
	kstrtol(dentry->d_parent->d_name.name,10, &parent);
	id=(pid_t)parent;
	
	printk(KERN_INFO "DEBUG PRELOOKUP\n");
	printk(KERN_INFO "DEBUG PRELOOKUP ID %d\n",id);
/*
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != id){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG LOOKUP PROBLEMI\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
*/
	if(strcmp(dentry->d_name.name,"fibers")==0){
	
		//Itera sulla lista processi
		//Cerca processo corrente
		spin_lock_irqsave(&(lock_lista_processi), flags);

		while (lista_processi_iter != NULL && lista_processi_iter->id != id){
			lista_processi_iter = lista_processi_iter->next;
		}
		
		if (lista_processi_iter == NULL){
			printk(KERN_INFO "DEBUG LOOKUP PROBLEMI\n");
			spin_unlock_irqrestore(&(lock_lista_processi), flags);

			return 0;	//Problemi
		}	
			
		if(lista_processi_iter->fiber_stuff.len_fiber_stuff==0){
			regs->cx=(unsigned long) lista_processi_iter->fiber_stuff.len_fiber_stuff;
			spin_unlock_irqrestore(&(lock_lista_processi), flags);

			return 0;
		}

		
		printk(KERN_INFO "DEBUG PRELOOKUP 0\n");
	

		
		regs->dx=(unsigned long) lista_processi_iter->fiber_stuff.fiber_base_stuff;
		regs->cx=(unsigned long) lista_processi_iter->fiber_stuff.len_fiber_stuff;
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		
	
	}else /*if(lista_processi_iter->counter_proc_look==0)*/{
		
		tgid_base_stuff = regs->dx;
	
		printk(KERN_INFO "DEBUG PRELOOKUP 1\n");

	n= (unsigned int) regs->cx;
	struct pid_entry fibers_entry;
	fibers_entry.name="fibers";
	fibers_entry.len=6;
	fibers_entry.mode=S_IFDIR|S_IRUGO|S_IXUGO;
	size=sizeof(fibers_entry)*(n+1);
	new_tgid_base_stuff =  kzalloc(size,GFP_KERNEL);
	
	for (i=0; i<n; i++){
		if (strcmp(tgid_base_stuff[i].name,"attr")==0){
			fibers_entry.iop = tgid_base_stuff[i].iop;//Questo coso serve a copiare i fop/iop dalla cartella attr
			fibers_entry.fop = tgid_base_stuff[i].fop;

		}
		new_tgid_base_stuff[i]=tgid_base_stuff[i];
	}
	new_tgid_base_stuff[i]=fibers_entry;

	
	
	regs->dx=(unsigned long) new_tgid_base_stuff;
	regs->cx=(unsigned long) n+1;
	//lista_processi_iter->counter_proc_look=1;
	}

	return 0;

}

void Post_Handler_Lookup(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
	struct dentry *dentry;
	if((unsigned long) new_tgid_base_stuff == regs->dx){
		dentry = (void *) regs->si;

	}
}




int Pre_Handler_Exit(struct kprobe *p, struct pt_regs *regs){ 
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter_old;
	struct Fiber* str_fiber_old;
	printk(KERN_INFO "DEBUG EXIT \n");

	//Itera sulla lista processi
	//Cerca processo corrente

	//spin_lock_irqsave(&(lock_lista_processi), flags);
	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG EXIT NO LISTA\n");
		//spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//No lista
		
	}
	
	//Itera sulla lista fiber
	//Cerca vecchio fiber
	//spin_lock_irqsave(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
	//spin_unlock_irqrestore(&(lock_lista_processi), flags);

	lista_fiber_iter_old = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter_old != NULL && lista_fiber_iter_old->runner != tid){
		lista_fiber_iter_old = lista_fiber_iter_old->next;
	}
	
	if (lista_fiber_iter_old == NULL){
		printk(KERN_INFO "DEBUG EXIT PROBLEMI\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);


		return 0;	//Problemi
	}
	
	str_fiber_old = lista_fiber_iter_old->fiber;
	
	//Manipolazione stato macchina
	struct pt_regs* my_regs= task_pt_regs(current);

	//Salvataggio registri nel vecchio fiber
	memcpy(&str_fiber_old->regs,my_regs,sizeof(struct pt_regs));
	//FPU
	copy_fxregs_to_kernel(&(str_fiber_old->fpu));
	
	//Manipolazione sistema interno
	lista_fiber_iter_old->running = -1; // non è più running (-1 vuol dire killato)
	lista_fiber_iter_old->runner = 0;
	//}

	//my_regs->sp=current->stack;
	//my_regs->bp=current->stack;
	//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);


	return 0;
}

static struct kprobe kp_readdir;
static struct kprobe kp_lookup; 
static struct kprobe kp_readdir_log;
static struct kprobe kp_lookup_log;
static struct kprobe kp_exit; 
//End pidstuff


//Aggiungere i lock
static int fib_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG OPEN\n");
		
	
	
    
	//Registrare processo in struttura
	//Creare gestore fiber per il processo
	struct Fiber_Processi* processo = (struct Fiber_Processi*) kmalloc(sizeof(struct Fiber_Processi),GFP_KERNEL);
	memset(processo,0,sizeof(struct Fiber_Processi));	// pulisce la lista dei processi (la nuova entry) per sicurezza perche potrebbe essere ancora in memoria
	processo->id = current->tgid;	//Current è globale
	processo->lock_fib_list=__SPIN_LOCK_UNLOCKED(processo->lock_fib_list); //Creazione lock
	int n_fib=10;
	int size=sizeof(struct pid_entry)*(n_fib);
	
	processo->fiber_stuff.fiber_base_stuff=kzalloc(size,GFP_KERNEL);
	processo->fiber_stuff.len_fiber_stuff=0;
	
	
	//Inserimento in lista
	spin_lock_irqsave(&(lock_lista_processi), flags);

	if (Lista_Processi == NULL) {Lista_Processi = processo;}
	else {
		struct Fiber_Processi* tmp = Lista_Processi;
		Lista_Processi = processo;
		processo->next = tmp;
	}
	
	
	spin_unlock_irqrestore(&(lock_lista_processi), flags);

	return 0;
}

static int fib_release(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG RELEASE\n");
	//Rilasciare processo da struttura
	pid_t pid = current->tgid;

	//Rimozione da lista
	spin_lock_irqsave(&(lock_lista_processi), flags);

	struct Fiber_Processi* bersaglio;
	if (Lista_Processi->id == pid){//Testa
		bersaglio = Lista_Processi;
		Lista_Processi = Lista_Processi->next;
	} else {//Non testa
		struct Fiber_Processi* processo_precedente = Lista_Processi;
		bersaglio = Lista_Processi->next;
		while(bersaglio != NULL && bersaglio->id != pid){
			processo_precedente = bersaglio;
			bersaglio = bersaglio->next;
		}
		
		if (bersaglio == NULL){
			printk(KERN_INFO "DEBUG RELEASE PROBLEMI\n");
		} else {
			processo_precedente->next = bersaglio->next;
		}
	}
	
	//Deallocazione profonda di bersaglio
	//Deallocazione lista fiber
	//spin_lock_irqsave(&(bersaglio->lock_fib_list), bersaglio->flags);

	if (bersaglio->lista_fiber != NULL){
		struct Lista_Fiber* TmpElem = bersaglio->lista_fiber;
		struct Lista_Fiber* Elem = TmpElem->next;
		while (TmpElem != NULL){
			//Deallocazione FLS
			Elem = TmpElem->next;
			
			if (TmpElem->fiber != NULL){
				struct Fiber* fib = TmpElem->fiber;
				if (fib->fls != NULL){
					flsFree();
				}
			
				//brucia la struttura interna
				kfree(fib);
			}

			//Elimina struttura
			kfree(TmpElem);
			TmpElem = Elem;
		}
	}
	//spin_unlock_irqrestore(&(bersaglio->lock_fib_list), bersaglio->flags);
	
	kfree(bersaglio->fiber_stuff.fiber_base_stuff);
	kfree(bersaglio);
	

	


	spin_unlock_irqrestore(&(lock_lista_processi), flags);

	return 0;
}

static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos){
	size_t s=100;
	char buffer[s];
	int lenght=0;
	printk(KERN_INFO "DEBUG READFILE\n");

	lenght = sprintf(buffer," %s\n","Ciao siamo in una prova");
	*ppos = lenght;
	
	return s;
}

static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	struct fiber_arguments fa;

	switch(cmd) {
		//copy_from_user(&to ,&from, sizeof(from));
		//copy_to_user(&to, &from, sizeof(from));

		case FIB_FLS_ALLOC:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_ALLOC\n");
			flsAlloc();
			break;
		case FIB_FLS_GET:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET\n");
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			fa.fls_value = (long) flsGetValue(fa.fls_index);//pos
			copy_to_user(&arg,&fa,sizeof(struct fiber_arguments));
			break;
		case FIB_FLS_SET: //Struttura necessaria
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET\n");
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			flsSetValue(fa.fls_index,fa.fls_value);//pos, val
			break;
		case FIB_FLS_DEALLOC:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC\n");
			flsFree();
			break;

		case FIB_CONVERT:
			printk(KERN_INFO "DEBUG IOCTL FIB_CONVERT\n");
			//Controlla se è già un fiber
			//Chiamata a FIB_CREATE
			//Chiama FIB_SWITCH_TO
			fib_convert();
			break;
		case FIB_CREATE:
			//PASSA PARAMETRO CON STRUCT
			printk(KERN_INFO "DEBUG IOCTL FIB_CREATE\n");
			//Allocazione e popolamento strtuttra Fiber
			//Inserimento oggetto in gestore
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			printk(KERN_INFO "DEBUG IOCTL FIB_CREATE ARG %p\n", fa.start_function_address);

			fa.fiber_id=fib_create(fa.start_function_address,fa.stack_pointer,fa.stack_size)->id;
			copy_to_user(arg,&fa,sizeof(struct fiber_arguments));
			break;
		case FIB_SWITCH_TO:
			printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO\n");
			//Controllo se il fiber è running
			//Cambiamento di stato macchina

			fib_switch_to(arg);
			break;
	}
	return 0;
}

//Funzioni ausiliarie
static void fib_convert(){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Lista_Fiber* lista_fiber_elem;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	//Itera sulla lista fiber
	//Controlla se sono già un fiber
	//spin_lock_irqsave(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
	//spin_unlock_irqrestore(&(lock_lista_processi), flags);

	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter){
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	//NULLPOINTER QUI SOTTO
	//Crea nuovo fiber
	// void * stack_pointer= kmalloc(4096*2*sizeof(char));
	int stack_size=1;
	lista_fiber_elem = do_fib_create((void*)1234,1,0,lista_processi_iter);
	
	//Manipolazione stato macchina
	struct pt_regs* my_regs = task_pt_regs(current);
	printk(KERN_INFO "DEBUG CONVERT IP %p\n",my_regs->ip);

	lista_fiber_elem->running = 1;
	lista_fiber_elem->runner = tid;
	
	
	//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
	spin_unlock_irqrestore(&(lock_lista_processi), flags);

}


static struct Lista_Fiber* do_fib_create(void* func, void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo){
	
	
	
	pid_t pid = current->tgid;
	unsigned long fiber_id=1;
	struct Lista_Fiber* lista_fiber_elem;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;

	
	
	
	//Crea Fiber str
	//spin_lock_irqsave(&(str_processo->lock_fib_list), str_processo->flags);

	str_fiber =kmalloc(sizeof(struct Fiber),GFP_KERNEL);
	memset(str_fiber,0,sizeof(struct Fiber));
	memcpy(&str_fiber->regs, task_pt_regs(current), sizeof(struct pt_regs)); 
	//Crea ptregs str
	str_fiber->regs.ip = (long)func;	//Assegna instruction pointer alla funzione passata
	str_fiber->regs.sp=(void*)(((char*)stack_pointer)+stack_size-1);
	str_fiber->regs.bp=str_fiber->regs.sp;

	printk(KERN_INFO "DEBUG CREATE FUNC %p\n", func);

	//FPU
	
	memset(&str_fiber->fpu,0,sizeof(struct fpu));
	
	//Crea Fiber elem in list
	lista_fiber_elem = (struct Lista_Fiber*) kmalloc(sizeof(struct Lista_Fiber),GFP_KERNEL);
	memset(lista_fiber_elem,0,sizeof(struct Lista_Fiber));
	
	//Assegna fiber id
	if(str_processo->lista_fiber!=NULL){
		fiber_id=str_processo->lista_fiber->id+1;
	}
	
	//Popola entry lista
	lista_fiber_elem->id = fiber_id;
	lista_fiber_elem->fiber = str_fiber;
	//lista_fiber_elem->lock_fib_list = __SPIN_LOCK_UNLOCKED(lista_fiber_elem->lock_fib_list); 

	//Collega entry in lista
	lista_fiber_elem->next = str_processo->lista_fiber;
	str_processo->lista_fiber = lista_fiber_elem;
	
	
	//spin_unlock_irqrestore(&(str_processo->lock_fib_list), str_processo->flags);


	
/*	char stringid[20];
	sprintf(stringid, "%d", i);
	char* stringanuova= memcpy(stringanuova, &stringid,20);
	char stringatotale[20];
	strcpy(stringatotale, ");
	strcat(stringatotale, stringanuova);
*/
	
	return lista_fiber_elem; //Controllo
}


static struct Lista_Fiber* fib_create(void* func, void *stack_pointer, unsigned long stack_size){
	pid_t pid = current->tgid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_elem;
	

	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG CREATE PROBLEMI\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	//Crea Fiber str
	int i=lista_processi_iter->fiber_stuff.len_fiber_stuff;

	lista_fiber_elem=do_fib_create( func, stack_pointer, stack_size,lista_processi_iter);
	struct pid_entry fibers_entry_log;
	fibers_entry_log.name="prova";
	fibers_entry_log.len=5;
	fibers_entry_log.mode=(S_IFREG|(S_IRUGO));
	fibers_entry_log.iop = NULL;
	fibers_entry_log.fop = &fops_proc;
	lista_processi_iter->fiber_stuff.fiber_base_stuff[i]=fibers_entry_log;
	lista_processi_iter->fiber_stuff.len_fiber_stuff=i+1;
	

	spin_unlock_irqrestore(&(lock_lista_processi), flags);


	return lista_fiber_elem; //Controllo
}



static void fib_switch_to(unsigned long id){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter_old;
	struct Lista_Fiber* lista_fiber_iter_new;
	struct Fiber* str_fiber_old;
	struct Fiber* str_fiber_new;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG SWITCH PROBLEMI1\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	//Itera sulla lista fiber
	//Cerca vecchio fiber
	
	//spin_lock_irqsave(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
	//spin_unlock_irqrestore(&(lock_lista_processi), flags);

	lista_fiber_iter_old = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter_old != NULL && lista_fiber_iter_old->runner != tid){
		lista_fiber_iter_old = lista_fiber_iter_old->next;
	}
	
	if (lista_fiber_iter_old == NULL){
		printk(KERN_INFO "DEBUG SWITCH PROBLEMI2\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber_old = lista_fiber_iter_old->fiber;
	
	//Itera sulla lista fiber
	//Cerca nuovo fiber
	lista_fiber_iter_new = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter_new != NULL && lista_fiber_iter_new->id != id){
		lista_fiber_iter_new = lista_fiber_iter_new->next;
	}
	
	if (lista_fiber_iter_new == NULL || lista_fiber_iter_new->running!=0){
		printk(KERN_INFO "DEBUG SWITCH PROBLEMI3\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber_new = lista_fiber_iter_new->fiber;
	
	//Manipolazione stato macchina
	struct pt_regs* my_regs= task_pt_regs(current);
	


	printk(KERN_INFO "DEBUG SWITCH IP 1 %p\n",my_regs->ip);

	//Salvataggio registri nel vecchio fiber
	memcpy(&str_fiber_old->regs,my_regs,sizeof(struct pt_regs));

	//FPU
	copy_fxregs_to_kernel(&(str_fiber_old->fpu));
	
	//Ripristino registri dal nuovo fiber
	memcpy(my_regs,&str_fiber_new->regs,sizeof(struct pt_regs));

	//FPU
	copy_kernel_to_fxregs(&(str_fiber_new->fpu.state.fxsave));

	printk(KERN_INFO "DEBUG SWITCH IP 2 %p\n",my_regs->ip);
	
	//Manipolazione sistema interno
	lista_fiber_iter_old->running = 0;
	lista_fiber_iter_old->runner = 0;
	lista_fiber_iter_new->running = 1;
	lista_fiber_iter_new->runner = tid;
	
	printk(KERN_INFO "DEBUG SWITCH OK\n");
	
	//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
	spin_unlock_irqrestore(&(lock_lista_processi), flags);

	return;
}

//FLS
static void flsAlloc(){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 1\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 2\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls != NULL){
		//FLS GIA ALLOCATO
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 3\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber->fls = (unsigned long*) kmalloc(sizeof(long)*FLS_SIZE,GFP_KERNEL);
	
	spin_unlock_irqrestore(&(lock_lista_processi), flags);
	return;
}

static void flsFree(){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG FLSFREE PROBLEMI 1\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
		printk(KERN_INFO "DEBUG FLSFREE PROBLEMI 2\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSFREE PROBLEMI 3\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	kfree(str_fiber->fls);
	str_fiber->fls = NULL;
	
	spin_unlock_irqrestore(&(lock_lista_processi), flags);
	return;
}

static long flsGetValue(unsigned long pos){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	long ret;
	
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 1\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 2\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls != NULL){
		//FLS GIA ALLOCATO
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 3\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	if (pos>FLS_SIZE) {
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 4\n");
		return;
	}
	ret = str_fiber->fls[pos];
	
	spin_unlock_irqrestore(&(lock_lista_processi), flags);
	return ret;
}

static void flsSetValue(unsigned long pos, long val){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	
	
	//Itera sulla lista processi
	//Cerca processo corrente
	spin_lock_irqsave(&(lock_lista_processi), flags);

	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 1\n");
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return 0;	//Problemi
	}
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 2\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls != NULL){
		//FLS GIA ALLOCATO
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 3\n");
		//spin_unlock_irqrestore(&(lista_processi_iter->lock_fib_list), lista_processi_iter->flags);
		spin_unlock_irqrestore(&(lock_lista_processi), flags);

		return;	//Problemi
	}
	
	if (pos>FLS_SIZE) {
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 4\n");
		return;
	}
	str_fiber->fls[pos] = val;
	
	spin_unlock_irqrestore(&(lock_lista_processi), flags);
	return;
}

//INIT-EXIT
static int fib_driver_init(void){
	
	printk(KERN_INFO "DEBUG INIT\n");
	

	//Allocating Major number
	if((alloc_chrdev_region(&dev, 0, 1, "fib_Dev")) <0){
		printk(KERN_INFO "Cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	//reating cdev structure
	cdev_init(&fib_cdev,&fops);
	fib_cdev.owner = THIS_MODULE;
	fib_cdev.ops = &fops;

	//Adding character device to the system
	if((cdev_add(&fib_cdev,dev,1)) < 0){
		printk(KERN_INFO "Cannot add the device to the system\n");
		goto r_class;
	}

	//Creating struct class
	if((dev_class = class_create(THIS_MODULE,"fib_class")) == NULL){
		printk(KERN_INFO "Cannot create the struct class\n");
		goto r_class;
	}

	//Create device
	if((device_create(dev_class,NULL,dev,NULL,DEV_NAME)) == NULL){ // qui si crea il file nel filesystem dev che viene messe in memoria con le sue strutture
		printk(KERN_INFO "Cannot create the Device 1\n");
		goto r_device;
	}
	printk(KERN_INFO "Device Driver Insert...Done!!!\n");
	
	
	
	//Creazione lock
	lock_lista_processi= __SPIN_LOCK_UNLOCKED(lock_lista_processi); 
	
	//Register Proc kprobe
	kp_readdir.pre_handler = Pre_Handler_Readdir; 
    kp_readdir.post_handler = Post_Handler_Readdir; 
    kp_readdir.symbol_name = "proc_pident_readdir";
    register_kprobe(&kp_readdir);

    kp_lookup.pre_handler = Pre_Handler_Lookup; 
    kp_lookup.post_handler = Post_Handler_Lookup; 
    kp_lookup.symbol_name = "proc_pident_lookup"; 
    register_kprobe(&kp_lookup);
    
    kp_exit.pre_handler = Pre_Handler_Exit; 
    kp_exit.symbol_name = "do_exit";
    register_kprobe(&kp_exit);

	return 0;
r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
}

void fib_driver_exit(void){
	printk(KERN_INFO "DEBUG EXIT\n");
	//Unregister kprobe
	unregister_kprobe(&kp_readdir);
	unregister_kprobe(&kp_lookup);

	unregister_kprobe(&kp_exit);

	//Destroy device
	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	cdev_del(&fib_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "Device Driver Remove...Done!!!\n");
}

module_init(fib_driver_init);
module_exit(fib_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com or admin@embetronicx.com>");
MODULE_DESCRIPTION("A simple device driver");
MODULE_VERSION("1.5");
