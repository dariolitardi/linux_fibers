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

#include "Strutture.h"

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

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7

struct Fiber_Processi* Lista_Processi;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev fib_cdev;

static int __init fib_driver_init(void);
static void __exit fib_driver_exit(void);
static int fib_open(struct inode *inode, struct file *file);
static int fib_release(struct inode *inode, struct file *file);
static ssize_t fib_read(struct file *filp, char __user *buf, size_t len,loff_t * off);
static ssize_t fib_write(struct file *filp, const char *buf, size_t len, loff_t * off);
static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static void fib_convert(void);
static struct Lista_Fiber* fib_create(void* func);
static void fib_switch_to(unsigned long id);

static unsigned long flsAlloc(void);
static void flsFree(unsigned long id);
static long flsGetValue(unsigned long* id, unsigned long pos);
static long flsSetValue(unsigned long* id, unsigned long pos, long val);

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.read           = fib_read,
	.write          = fib_write,
	.open           = fib_open,
	.unlocked_ioctl = fib_ioctl,
	.release        = fib_release,
};

//PID stuff with kprobe
static unsigned int counter = 0;
static kprobe_opcode_t* kprobe_readdir_func;//0xffffffff83c7c960;
static kprobe_opcode_t* kprobe_lookup_func;
//static struct pid_entry* tgid_base_stuff;//0xffffffff8422be80;
static struct pid_entry* new_tgid_base_stuff;
//tgid_base stuff at ffffffff8422be80
//use cat /proc/kallsyms | grep tgid_base_stuff

void Make_new_tgid_base_stuff(struct pt_regs *regs){
	/*
	//Controlla se il processo è rilevante
	pid_t id = 0;
	void* ptr = (void*) regs->di;
	struct file* file = (struct file*) regs->di;
	if (ptr == NULL){
		printk(KERN_INFO "DEBUG FILE NULL PTR");
	} else {
		printk(KERN_INFO "DEBUG FILE PRESENT");
		ptr = (void*)&(file->f_path);
		if (ptr == NULL){
			printk(KERN_INFO "DEBUG FILEPATH NULL PTR");
		} else {
			printk(KERN_INFO "DEBUG FILEPATH PRESENT");
			ptr = (void*)&(file->f_path.dentry);
			if (ptr == NULL){
				printk(KERN_INFO "DEBUG DENTRY NULL PTR");
			} else {
				printk(KERN_INFO "DEBUG DENTRY PRESENT");
				ptr =  (void*)&(file->f_path.dentry->d_name);
				if (ptr == NULL){
					printk(KERN_INFO "DEBUG DENTRYNAME NULL PTR");
				} else {
					printk(KERN_INFO "DEBUG DENTRYNAME PRESENT");
					ptr = (void*)&(file->f_path.dentry->d_name.name);
					if (ptr == NULL){
						printk(KERN_INFO "DEBUG NAME NULL PTR");
					} else {
						printk(KERN_INFO "DEBUG NAME PRESENT");
						printk(KERN_INFO "DEBUG NAME %p",&(file->f_path.dentry->d_name.name));
						printk(KERN_INFO "DEBUG NAME %s",&(file->f_path.dentry->d_name.name));
					}
				}
			}
		}
	}
	
	//Cerca processo
    struct Fiber_Processi* tmp = Lista_Processi;
    while (tmp != NULL){
		if (tmp->id == id){
			break;
		}
		tmp = tmp->next;
	}
	*/
	
	//Crea nuova cartella
	struct pid_entry* tgid_base_stuff = (struct pid_entry*)regs->dx;
//	if (tmp){
		printk(KERN_INFO "SOSTITUZIONE ARRAY");
		
		if (new_tgid_base_stuff != NULL){
			kfree(new_tgid_base_stuff);
		}
		
		new_tgid_base_stuff = (struct pid_entry*) kmalloc((sizeof(tgid_base_stuff)/sizeof(struct pid_entry)+1)*sizeof(struct pid_entry),GFP_KERNEL);
		memcpy(new_tgid_base_stuff,tgid_base_stuff,sizeof(tgid_base_stuff));
		
		struct pid_entry* fibers_entry = (struct pid_entry*) kmalloc(sizeof(struct pid_entry),GFP_KERNEL);
		fibers_entry->name = "Fibers";
		fibers_entry->len = 6;
		fibers_entry->mode = S_IFDIR|S_IRUGO|S_IXUGO;
		
		int i;
		for (i=0;i<sizeof(tgid_base_stuff)/sizeof(struct pid_entry);i++){
			if (strcmp((tgid_base_stuff)[i].name,"attr")==0){
				fibers_entry->iop = (tgid_base_stuff)[i].iop;//Questo coso serve a copiare i fop/iop dalla cartella attr
				fibers_entry->fop = (tgid_base_stuff)[i].fop;
			}
		}
//		fibers_entry->op = (union proc_op) NULL;
		
		memcpy(new_tgid_base_stuff+(sizeof(tgid_base_stuff)),fibers_entry,sizeof(struct pid_entry));
		
		kfree(fibers_entry);
		
		regs->dx = new_tgid_base_stuff;
//	}
}

int Pre_Handler_Readdir(struct kprobe *p, struct pt_regs *regs){ 
    printk(KERN_INFO "DEBUG KPROBE READDIR PID %d\n",((struct task_struct*)regs->dx)->tgid);
	printk(KERN_INFO "DEBUG KPROBE READDIR %p , %p\n",(void*)regs->cx,(void*)kallsyms_lookup_name("tgid_base_stuff"));

	Make_new_tgid_base_stuff(regs);
	
    return 0;
} 

void Post_Handler_Readdir(struct kprobe *p, struct pt_regs *regs, unsigned long flags){ 
    if (new_tgid_base_stuff){
		kfree(new_tgid_base_stuff);
		new_tgid_base_stuff = NULL;
	}
}

int Pre_Handler_Lookup(struct kprobe *p, struct pt_regs *regs){
    printk(KERN_INFO "DEBUG KPROBE LOOKUP PID %d\n",((struct task_struct*)regs->dx)->tgid);
	printk(KERN_INFO "DEBUG KPROBE LOOKUP %p , %p\n",(void*)regs->cx,(void*)kallsyms_lookup_name("tgid_base_stuff"));

	//struct pid_entry* tgt = regs->di;
	//regs->di = new_tgid_base_stuff;

	Make_new_tgid_base_stuff(regs);
	
	return 0;
}

void Post_Handler_Lookup(struct kprobe *p, struct pt_regs *regs, unsigned long flags){
	if (new_tgid_base_stuff){
		kfree(new_tgid_base_stuff);
		new_tgid_base_stuff = NULL;
	}
}

static struct kprobe kp_readdir;
static struct kprobe kp_lookup; 
//End pidstuff


//Aggiungere i lock
static int fib_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG OPEN\n");
	//Registrare processo in struttura
	//Creare gestore fiber per il processo
	struct Fiber_Processi* processo = (struct Fiber_Processi*) kmalloc(sizeof(struct Fiber_Processi),GFP_KERNEL);
	memset(processo,0,sizeof(struct Fiber_Processi));	// pulisce la lista dei processi (la nuova entry) per sicurezza perche potrebbe essere ancora in memoria
	processo->id = current->tgid;	//Current è globale

	//Inserimento in lista
	if (Lista_Processi == NULL) {Lista_Processi = processo;}
	else {
		struct Fiber_Processi* tmp = Lista_Processi;
		Lista_Processi = processo;
		processo->next = tmp;
	}
	
	return 0;
}

static int fib_release(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG RELEASE\n");
	//Rilasciare processo da struttura
	pid_t pid = current->tgid;

	//Rimozione da lista
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
		//Porcaputtana.jpg
		} else {
		processo_precedente->next = bersaglio->next;
		}
	}
	
	//Deallocazione profonda di bersaglio
	//Deallocazione lista fiber
	if (bersaglio->lista_fiber != NULL){
		struct Lista_Fiber* TmpElem = bersaglio->lista_fiber;
		struct Lista_Fiber* Elem = TmpElem->next;
		while (TmpElem != NULL){
			//Deallocazione FLS
			if (TmpElem->fiber != NULL){
				struct Fiber* fib = TmpElem->fiber;
				if (fib->fls != NULL){
					flsFree(fib->fls);
				}
				//brucia la struttura interna
			kfree(fib);
			}

		//Elimina struttura
		kfree(TmpElem);
		TmpElem = Elem;
		Elem = Elem->next;
		}
	}
	kfree(bersaglio);
	return 0;
}

static ssize_t fib_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
	printk(KERN_INFO "DEBUG READ\n");
	return 0;
}

static ssize_t fib_write(struct file *filp, const char __user *buf, size_t len, loff_t *off){
	printk(KERN_INFO "DEBUG WRITE\n");
	return 0;
}

static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	switch(cmd) {
		//copy_from_user(&to ,&from, sizeof(from));
		//copy_to_user(&to, &from, sizeof(from));

		case FIB_FLS_ALLOC:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_ALLOC\n");
			unsigned long tmp_id = flsAlloc();
			copy_to_user(&arg,&tmp_id,sizeof(tmp_id));
			//Link alla struttura
			break;
		case FIB_FLS_GET:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET\n");
			long tmp_val = flsGetValue(arg,arg);//id, pos
			copy_to_user(&arg,&tmp_val,sizeof(tmp_id));
			break;
		case FIB_FLS_SET: //Struttura necessaria
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET\n");
			flsSetValue(arg,arg,arg);//id, pos, val
			break;
		case FIB_FLS_DEALLOC:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC\n");
			flsFree(arg);
			break;

		case FIB_CONVERT:
			printk(KERN_INFO "DEBUG IOCTL FIB_CONVERT\n");
			//Controlla se è già un fiber
			//Chiamata a FIB_CREATE
			//Chiama FIB_SWITCH_TO
			fib_convert();
			break;
		case FIB_CREATE:
			printk(KERN_INFO "DEBUG IOCTL FIB_CREATE\n");
			//Allocazione e popolamento strtuttra Fiber
			//Inserimento oggetto in gestore
			fib_create((void*)arg);
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
	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		return;	//Problemi
	}
	
	//Itera sulla lista fiber
	//Controlla se sono già un fiber
	lista_fiber_iter = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter != NULL && lista_fiber_iter->runner != tid){
		lista_fiber_iter = lista_fiber_iter->next;
	}
	
	if (lista_fiber_iter){
		return;	//Problemi
	}
	
	//Crea nuovo fiber
	lista_fiber_elem = fib_create((void*)NULL);
	
	//Manipolazione stato macchina
//	struct pt_regs* my_regs = task_pt_regs(current);
	
	lista_fiber_elem->running = 1;
	lista_fiber_elem->runner = tid;
}

static struct Lista_Fiber* fib_create(void* func){
	pid_t pid = current->tgid;
	struct Fiber_Processi* lista_processi_iter = Lista_Processi;
	struct Lista_Fiber* lista_fiber_iter;
	struct Lista_Fiber* lista_fiber_elem;
	struct Fiber* str_fiber;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		return;	//Problemi
	}
	
	//Crea Fiber str
	str_fiber = (struct Fiber*) kmalloc(sizeof(struct Fiber),GFP_KERNEL);
	memset(str_fiber,0,sizeof(struct Fiber));
	
	//Crea ptregs str
	str_fiber->regs = (struct pt_regs*) kmalloc(sizeof(struct pt_regs),GFP_KERNEL);
	memset(str_fiber->regs,0,sizeof(struct pt_regs));
	str_fiber->regs->ip = func;	//Assegna instruction pointer alla funzione passata
	
	//FPU
	
	//Crea Fiber elem in list
	lista_fiber_elem = (struct Lista_Fiber*) kmalloc(sizeof(struct Lista_Fiber),GFP_KERNEL);
	memset(lista_fiber_elem,0,sizeof(struct Lista_Fiber));
	
	//Popola entry lista
	lista_fiber_elem->id = 0;//TODO ID
	lista_fiber_elem->fiber = str_fiber;
	
	//Collega entry in lista
	lista_fiber_elem->next = lista_processi_iter->lista_fiber;
	lista_processi_iter->lista_fiber = lista_fiber_elem;
	
	return lista_fiber_iter; //Controllo
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
	while (lista_processi_iter != NULL && lista_processi_iter->id != pid){
		lista_processi_iter = lista_processi_iter->next;
	}
	
	if (lista_processi_iter == NULL){
		return;	//Problemi
	}
	
	//Itera sulla lista fiber
	//Cerca vecchio fiber
	lista_fiber_iter_old = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter_old != NULL && lista_fiber_iter_old->runner != tid){
		lista_fiber_iter_old = lista_fiber_iter_old->next;
	}
	
	if (lista_fiber_iter_old == NULL){
		return;	//Problemi
	}
	
	str_fiber_old = lista_fiber_iter_old->fiber;
	
	//Itera sulla lista fiber
	//Cerca nuovo fiber
	lista_fiber_iter_new = lista_processi_iter->lista_fiber;
	while (lista_fiber_iter_new != NULL && lista_fiber_iter_new->id != id){
		lista_fiber_iter_new = lista_fiber_iter_new->next;
	}
	
	if (lista_fiber_iter_new == NULL || lista_fiber_iter_new->running){
		return;	//Problemi
	}
	
	str_fiber_new = lista_fiber_iter_new->fiber;
	
	//Manipolazione stato macchina
	struct pt_regs* my_regs = task_pt_regs(current);
	
	//Salvataggio registri nel vecchio fiber
	memcpy(my_regs,str_fiber_old->regs,sizeof(struct pt_regs));
	//FPU
	
	//Ripristino registri dal nuovo fiber
	memcpy(str_fiber_new->regs,my_regs,sizeof(struct pt_regs));
	//FPU
	
	//Manipolazione sistema interno
	lista_fiber_iter_old->running = 0;
	lista_fiber_iter_old->runner = 0;
	lista_fiber_iter_new->running = 1;
	lista_fiber_iter_new->runner = tid;
	
	return;
}

//FLS
static unsigned long flsAlloc(){
	return (unsigned long) kmalloc(sizeof(long)*1024,GFP_KERNEL);
}

static void flsFree(unsigned long id){
	return kfree(id);
}

static long flsGetValue(unsigned long* id, unsigned long pos){
	return id[pos];
}

static long flsSetValue(unsigned long* id, unsigned long pos, long val){
	id[pos] = val;
	return val;
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
	
	//Register Proc kprobe
	kprobe_readdir_func = (kprobe_opcode_t*) kallsyms_lookup_name("proc_pident_instantiate");
	kprobe_lookup_func = (kprobe_opcode_t*) kallsyms_lookup_name("proc_pident_readdir");
	
	kp_readdir.pre_handler = Pre_Handler_Readdir; 
    kp_readdir.post_handler = Post_Handler_Readdir; 
    kp_readdir.addr = kprobe_readdir_func; 
    register_kprobe(&kp_readdir);
    
    kp_lookup.pre_handler = Pre_Handler_Lookup; 
    kp_lookup.post_handler = Post_Handler_Lookup; 
    kp_lookup.addr = kprobe_lookup_func; 
    register_kprobe(&kp_lookup);
	
	return 0;
r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
}

void fib_driver_exit(void){
	printk(KERN_INFO "DEBUG EXIT\n");
	//Unregister Proc kprobe
	unregister_kprobe(&kp_readdir);
	unregister_kprobe(&kp_lookup);
	
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
