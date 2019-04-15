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
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <linux/hashtable.h>


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

#define FIB_CONVERT	5
#define FIB_CREATE	6
#define FIB_SWITCH_TO	7
#define FIB_LOG_LEN 1024

#define FLS_SIZE 4096
#define HTSIZE 16

DEFINE_HASHTABLE(processi, HTSIZE);

dev_t dev = 0;
static struct class *dev_class;
static struct cdev fib_cdev;


static int __init fib_driver_init(void);
static void __exit fib_driver_exit(void);
static int fib_open(struct inode *inode, struct file *file);
static int fib_release(struct inode *inode, struct file *file);
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos);
static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


static pid_t fib_convert(void);
static struct Lista_Fiber* do_fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo);
static struct Lista_Fiber* fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size);
static long fib_switch_to(pid_t id);

static long flsAlloc(void);
static bool flsFree(long index);
static long flsGetValue(long pos);
static void flsSetValue(long pos, long val);

static struct file_operations fops = {
	.owner          = THIS_MODULE,
	.open           = fib_open,
	.unlocked_ioctl = fib_ioctl,
	.release        = fib_release,
};

static struct file_operations fops_proc = {
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
	struct Fiber_Processi* fiber_processo;
	kstrtol(file->f_path.dentry->d_parent->d_name.name,10, &parent);
	id=(pid_t)parent;

	printk(KERN_INFO "DEBUG PREREADDIR\n");
	printk(KERN_INFO "DEBUG PREREADDIR ID %d\n",id);

	if(strcmp(file->f_path.dentry->d_name.name,"fibers")==0){
	
		//Itera sulla lista processi
		//Cerca processo corrente

		hash_for_each_possible_rcu(processi, fiber_processo, node, (int)id){
		
			if(fiber_processo == NULL) {
				break;
			}
			if (fiber_processo->id == id) {
				break;
			}
		}
		
		if (fiber_processo == NULL){
			printk(KERN_INFO "DEBUG PREREADDIR PROBLEMI\n");

			return 0;	//Problemi
		}	
		
		
		
		if(fiber_processo->fiber_stuff.len_fiber_stuff==0){
			regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;

			return 0;
		}
		
		printk(KERN_INFO "DEBUG PREREADIR 0\n");

		regs->dx=(unsigned long) fiber_processo->fiber_stuff.fiber_base_stuff;
		regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;
	
	
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
	struct Fiber_Processi* fiber_processo;
	
	kstrtol(dentry->d_parent->d_name.name,10, &parent);
	id=(pid_t)parent;
	
	printk(KERN_INFO "DEBUG PRELOOKUP\n");
	printk(KERN_INFO "DEBUG PRELOOKUP ID %d\n",id);


	if(strcmp(dentry->d_name.name,"fibers")==0){
	
		//Itera sulla lista processi
		//Cerca processo corrente
		hash_for_each_possible_rcu(processi, fiber_processo, node, id){
		
			if(fiber_processo == NULL) {
				break;
			}
			if (fiber_processo->id == id) {
				break;
			}
		}
		
		if (fiber_processo == NULL){
			printk(KERN_INFO "DEBUG LOOKUP PROBLEMI\n");

			return 0;	//Problemi
		}	
			
		if(fiber_processo->fiber_stuff.len_fiber_stuff==0){
			regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;

			return 0;
		}

		
		printk(KERN_INFO "DEBUG PRELOOKUP 0\n");
	

		
		regs->dx=(unsigned long) fiber_processo->fiber_stuff.fiber_base_stuff;
		regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;

		
	
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
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter_old;
	struct Fiber* str_fiber_old;
	printk(KERN_INFO "DEBUG EXIT \n");

	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int	flag_fib=0;

	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		
		if(fiber_processo == NULL) {
			
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	if(flag==0){
		printk(KERN_INFO "DEBUG EXIT NO LISTA\n");
		return 0;
	}
		
	//Itera sulla lista fiber
	//Cerca vecchio fiber
	
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter_old, node, (int)tid){
		if (lista_fiber_iter_old == NULL){

			flag_fib=0;
			break;	//Problemi
		}
		
        if (lista_fiber_iter_old->runner == tid) {
			flag_fib=1;

            break;
        }
	}
	
	if(flag_fib==0){
		printk(KERN_INFO "DEBUG EXIT NO LISTA\n");
		return 0;
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
	int n_fib=10;
	int size=sizeof(struct pid_entry)*(n_fib);
	
	processo->fiber_stuff.fiber_base_stuff=kzalloc(size,GFP_KERNEL);
	processo->fiber_stuff.len_fiber_stuff=0;
	
	hash_init(processo->lista_fiber);   

	//Inserimento in lista
    hash_add_rcu(processi, &(processo->node), processo->id);  

	
	
	return 0;
}

static int fib_release(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG RELEASE\n");
	//Rilasciare processo da struttura
	pid_t pid = current->tgid;

	//Rimozione da lista

	struct Fiber_Processi* bersaglio;
	int flag=0;
	hash_for_each_possible_rcu(processi, bersaglio, node, (int)pid){
		
		if(bersaglio == NULL) {
			flag=0;

			break;
        }
        if (bersaglio->id == pid) {
			flag=1;
            break;
        }
	}
	if(flag == 0) {
			printk(KERN_INFO "DEBUG RELEASE PROBLEMI 1\n");
			return -1;	//Problemi
        }
	
	//Deallocazione profonda di bersaglio
	//Deallocazione lista fiber
	struct Lista_Fiber* TmpElem = bersaglio->lista_fiber;
	
	hash_for_each_possible_rcu(fiber_processo->listafiber, TmpElem, node,(int) tid){
		if (TmpElem == NULL){
			break;
		}
		if (TmpElem != NULL){
		
			struct Fiber* fib = TmpElem->fiber;
			if (fib->fls != NULL){
					kfree(fib->fls);

			}
			if (fib->bitmap_fls!= NULL){
				kfree(fib->bitmap_fls);
					
			}
			
			//brucia la struttura interna
			kfree(fib);
			kfree(TmpElem);
		}

	}
	
		

	kfree(bersaglio->fiber_stuff.fiber_base_stuff);
	kfree(bersaglio);
	

	return 0;
}

static ssize_t myread(struct file *filp, char __user *buf, size_t len, loff_t *off){
	
	pid_t pid;
	long tmp_pid;
	unsigned long fib_id;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	size_t i;
	char fiber_stat[FIB_LOG_LEN] = "";
	
	printk(KERN_INFO "DEBUG READFILE\n");
	
	//Ottiene gli id (fiber id e pid)
	
	kstrtoul(filp->f_path.dentry->d_name.name+1,10, &fib_id);
	kstrtol(filp->f_path.dentry->d_parent->d_parent->d_name.name,10, &tmp_pid);
	pid=(pid_t) tmp_pid;
	printk(KERN_INFO "DEBUG READFILE parent %s\n",filp->f_path.dentry->d_name.name);

	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		
		if(fiber_processo == NULL) {
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	if(flag == 0) {
			printk(KERN_INFO "DEBUG READFILE PROBLEMI 1\n");
			return -1;	//Problemi
        }
	
	//Itera sulla lista fiber
	

	
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node,(int) fib_id){
		if (lista_fiber_iter == NULL){
			printk(KERN_INFO "DEBUG READFILE PROBLEMI 2\n");
			flag_fib=0;
			break;	//Problemi
		}
		
        if (lista_fiber_iter->id == fib_id) {
			flag_fib=1;
            break;
        }
	}
	
	if (flag_fib == 0){
			printk(KERN_INFO "DEBUG READFILE PROBLEMI 2\n");
			return -1;	//Problemi
		}

	
	if (lista_fiber_iter == NULL){
		printk(KERN_INFO "DEBUG READFILE PROBLEMI 3, id: %ul\n",fib_id);
	
		return -1;	//Problemi
	}
	
	str_fiber = lista_fiber_iter->fiber;
	
	char* running_str;
	if (lista_fiber_iter->running){
		running_str = "";
	} else {
		running_str = "not ";
	}

	sprintf(fiber_stat,"##FIBER STATISTICS##\n\n - This fiber is currently %srunning.\n - The initial entry point is: %p\n - This fiber was created by: %lu (pid)\n - Succesful activations: %lu\n - Unsuccesful activations: %lu\n - Total running time: %lu msec\n\0",running_str,str_fiber->entry_point,str_fiber->creator,str_fiber->correct_counter,str_fiber->failed_counter,str_fiber->exec_time/1000);
	
	
	if (*off >= strnlen(fiber_stat, FIB_LOG_LEN)){
		return 0;
	}
	i = min_t(size_t, len, strnlen(fiber_stat, FIB_LOG_LEN));
	if (copy_to_user(buf, fiber_stat, i)) {
		return -EFAULT;
	}
	*off += i;
	
	return i;
}

static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	struct fiber_arguments fa;
	long ret;
	long ret_val;
	printk(KERN_INFO "DEBUG IOCTL CMD %d\n",cmd);
	switch(cmd) {
		//copy_from_user(&to ,&from, sizeof(from));
		//copy_to_user(&to, &from, sizeof(from));

		case FIB_FLS_ALLOC:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_ALLOC\n");
			ret = (long)flsAlloc();
			return ret;
			break;
		case FIB_FLS_GET:
		case 404:
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET\n");
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			fa.fls_value = (long) flsGetValue(fa.fls_index);//pos
			copy_to_user(arg,&fa,sizeof(struct fiber_arguments));
			break;
		case FIB_FLS_SET: //Struttura necessaria
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET\n");
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			flsSetValue(fa.fls_index,fa.fls_value);//pos, val
			break;
		case FIB_FLS_DEALLOC:
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC\n");
			ret=(long)flsFree(fa.fls_index);
			return ret;
			break;

		case FIB_CONVERT:
			printk(KERN_INFO "DEBUG IOCTL FIB_CONVERT\n");
			//Controlla se è già un fiber
			//Chiamata a FIB_CREATE
			//Chiama FIB_SWITCH_TO
			return fib_convert();
			break;
		case FIB_CREATE:
			//PASSA PARAMETRO CON STRUCT
			printk(KERN_INFO "DEBUG IOCTL FIB_CREATE\n");
			//Allocazione e popolamento struttra Fiber
			//Inserimento oggetto in gestore
			copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments));
			printk(KERN_INFO "DEBUG IOCTL FIB_CREATE ARG %p\n", fa.start_function_address);

			fa.fiber_id=fib_create(fa.start_function_address,fa.start_function_parameters,fa.stack_pointer,fa.stack_size)->id;
			copy_to_user(arg,&fa,sizeof(struct fiber_arguments));
			break;
		case FIB_SWITCH_TO:
			printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO\n");
			//Controllo se il fiber è running
			//Cambiamento di stato macchina

			return fib_switch_to(arg);
			break;
	}
	return 0;
}

//Funzioni ausiliarie
static pid_t fib_convert(){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Lista_Fiber* lista_fiber_elem;
	struct timespec* time_str;
	unsigned long ret=0;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	hash_for_each_possible_rcu(processi, fiber_processo, node, (int)pid){
		if(fiber_processo == NULL) {
			flag=0;
			break;
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	if(flag == 0) {
			printk(KERN_INFO "DEBUG CONVERT PROBLEMI\n");

			return 0;	//Problemi
        }

	//Itera sulla lista fiber
	//Controlla se sono già un fiber
	
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node, (int)tid){
		if(lista_fiber_iter == NULL) {
			break;
        }
        if (lista_fiber_iter->runner == tid) {
            break;
        }
	}
	
	
	if (lista_fiber_iter){

		return 0;	//Problemi
	}
	
	//Crea nuovo fiber
	int stack_size=1;
	struct pt_regs* my_regs = task_pt_regs(current);

	lista_fiber_elem = do_fib_create((void*)my_regs->ip,0,1,0,fiber_processo);

	//Manipolazione stato macchina
	printk(KERN_INFO "DEBUG CONVERT IP %p\n",my_regs->ip);

	lista_fiber_elem->running = 1;
	lista_fiber_elem->runner = tid;
			
	time_str= kmalloc(sizeof(struct timespec),GFP_KERNEL);
	getnstimeofday(time_str);
	
	
	lista_fiber_elem->fiber->last_activation_time=time_str->tv_nsec;
	lista_fiber_elem->fiber->correct_counter=1;
	kfree(time_str);
	
	ret=lista_fiber_elem->id;
	
	return ret;
}


static struct Lista_Fiber* do_fib_create(void* func,void* parameters, void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo){

	
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	pid_t fiber_id=1;
	struct Lista_Fiber* lista_fiber_elem;
	struct Fiber* str_fiber;

	
	//Crea Fiber str
	//spin_lock_irqsave(&(str_processo->lock_fib_list), str_processo->flags);

	str_fiber =kmalloc(sizeof(struct Fiber),GFP_KERNEL);
	memset(str_fiber,0,sizeof(struct Fiber));
	memset(&str_fiber->regs,0,sizeof(struct pt_regs));

	memcpy(&str_fiber->regs, task_pt_regs(current), sizeof(struct pt_regs)); 
	//Crea ptregs str
	str_fiber->regs.ip = (long)func;	//Assegna instruction pointer alla funzione passata
	str_fiber->regs.sp=(void*)(((char*)stack_pointer)+stack_size-1);
	str_fiber->regs.bp=str_fiber->regs.sp;
	str_fiber->regs.di=(long)parameters;
	//
	str_fiber->entry_point = func;
	str_fiber->creator = tid;
	str_fiber->correct_counter = 0;
	str_fiber->failed_counter = 0;
	str_fiber->exec_time = 0;
	str_fiber->last_activation_time = 0;

	if (str_fiber->fls == NULL){
		str_fiber->fls = (long*) kmalloc(sizeof(long)*FLS_SIZE,GFP_KERNEL);
		memset(str_fiber->fls,0,sizeof(long)*FLS_SIZE);	
	
	}
	
	if (str_fiber->bitmap_fls == NULL){
		str_fiber->bitmap_fls = (unsigned long*) kmalloc(sizeof(unsigned long)*FLS_SIZE,GFP_KERNEL);
		memset(str_fiber->bitmap_fls,0,sizeof(unsigned long)*FLS_SIZE);	

	}

	
	printk(KERN_INFO "DEBUG CREATE FUNC %p\n", func);

	//FPU
	
	memset(&str_fiber->fpu,0,sizeof(struct fpu));
	copy_fxregs_to_kernel(&(str_fiber->fpu));                             
	
	//Crea Fiber elem in list
	lista_fiber_elem = (struct Lista_Fiber*) kmalloc(sizeof(struct Lista_Fiber),GFP_KERNEL);
	memset(lista_fiber_elem,0,sizeof(struct Lista_Fiber));
	int c=0;
	//Assegna fiber id
	hash_for_each_possible_rcu(str_processo->listafiber, lista_fiber_iter, node, (int)tid){
		if (lista_fiber_iter == NULL ){
			break;
		}
		
		if (lista_fiber_iter != NULL ){
			c+=1;
		}
		

	}
	
	
	fiber_id=c;
	

	
	//Popola entry lista
	lista_fiber_elem->id = fiber_id;
	lista_fiber_elem->fiber = str_fiber;

	//Collega entry in lista

	hash_add_rcu(str_processo->listafiber,&(lista_fiber_elem->node), lista_fiber_elem->id);
	

	return lista_fiber_elem; //Controllo
}


static struct Lista_Fiber* fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size){
	pid_t pid = current->tgid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_elem;


	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		if(fiber_processo == NULL) {
			flag=0;

			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG CREATE PROBLEMI\n");

			return;	//Problemi
        }
	
	//Crea Fiber str
	int i=fiber_processo->fiber_stuff.len_fiber_stuff;

	lista_fiber_elem=do_fib_create( func,parameters, stack_pointer, stack_size,fiber_processo);
	struct pid_entry fibers_entry_log;
	fibers_entry_log.name=kmalloc(sizeof(char)*16,GFP_KERNEL);
	sprintf (fibers_entry_log.name, "f%lu", lista_fiber_elem->id);
	fibers_entry_log.len=strlen(fibers_entry_log.name);
	fibers_entry_log.mode=(S_IFREG|(S_IRUGO));
	fibers_entry_log.iop = NULL;
	fibers_entry_log.fop = &fops_proc;
	fiber_processo->fiber_stuff.fiber_base_stuff[i]=fibers_entry_log;
	fiber_processo->fiber_stuff.len_fiber_stuff=i+1;
	

	return lista_fiber_elem; //Controllo
}



static long fib_switch_to(pid_t id){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter_old;
	struct Lista_Fiber* lista_fiber_iter_new;
	struct Fiber* str_fiber_old;
	struct Fiber* str_fiber_new;
	struct timespec* time_str;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_old=0;
	int flag_new=0;

	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		
		if(fiber_processo == NULL) {
			printk(KERN_INFO "DEBUG SWITCH PROBLEMI 1\n");
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG SWITCH PROBLEMI 1\n");

			return -1;	//Problemi
    }
	//Itera sulla lista fiber
	//Cerca vecchio fiber
	
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter_old, node, (int)tid){
		if (lista_fiber_iter_old == NULL){
			flag_old=0;

			break;	//Problemi
		}
		
        if (lista_fiber_iter_old->runner == tid) {
			flag_old=1;
            break;
        }
	}
	
	if (flag_old == 0){
			printk(KERN_INFO "DEBUG SWITCH PROBLEMI 2\n");

			return -1;	//Problemi
	}
		
	
	str_fiber_old = lista_fiber_iter_old->fiber;
	
	//Itera sulla lista fiber
	//Cerca nuovo fiber
	
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter_new, node,(int) tid){
		if (lista_fiber_iter_new == NULL){
			printk(KERN_INFO "DEBUG SWITCH PROBLEMI 3\n");
			flag_new=0;

			break;	//Problemi
		}
		
        if (lista_fiber_iter_new->runner == tid) {
			flag_new=1;

            break;
        }
	}
	if (flag_new == 0){
			printk(KERN_INFO "DEBUG SWITCH PROBLEMI 3\n");

			return -1;	//Problemi
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
	
		
	time_str= kmalloc(sizeof(struct timespec),GFP_KERNEL);
	getnstimeofday(time_str);
	
	str_fiber_new->correct_counter += 1;
	str_fiber_new->last_activation_time =time_str->tv_nsec;
	str_fiber_old->exec_time += ((str_fiber_new->last_activation_time)-(str_fiber_old->last_activation_time));
	
	kfree(time_str);
	printk(KERN_INFO "DEBUG SWITCH OK\n");
	
	
	return 0;
}

//FLS
static long flsAlloc(){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	long index;
	printk(KERN_INFO "DEBUG FLSALLOC\n");

	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	
	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		
		if(fiber_processo == NULL) {
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 1\n");

			return -1;	//Problemi
        }
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node, (int)tid){
		if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
			flag_fib=0;

			break;//Problemi
		}
		
        if (lista_fiber_iter->running == tid) {
			flag_fib=1;
            break;
        }
	}
	if (flag_fib==0){
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 2\n");

			return -1;	//Problemi
		}
		
	
	
	
	
	str_fiber = lista_fiber_iter->fiber;
	if (str_fiber->bitmap_fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 3\n");
		
		return -1;	//Problemi
	}
	

    int c;
    for (c=0; c<FLS_SIZE; c++){
		if(str_fiber->bitmap_fls[c]==0){
			index=c;
			break;
		}
	}
	
	
    printk(KERN_INFO "DEBUG FLSALLOC %d\n",index);
	
	if (index == FLS_SIZE){
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 4\n");
		
		return -1;	//Problemi
    }
	
	str_fiber->bitmap_fls[index]=1;	
   

    return index;
}

static bool flsFree(long index){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	printk(KERN_INFO "DEBUG FLSFREE \n");

	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	
	hash_for_each_possible_rcu(processi, fiber_processo, node, (int) pid){
		
		if(fiber_processo == NULL) {
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 1\n");

			return false;	//Problemi
        }
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node, (int) tid){
		if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
			flag_fib=0;

			break;//Problemi
		}
		
        if (lista_fiber_iter->running == tid) {
			flag_fib=1;
            break;
        }
	}
	if (flag_fib==0){
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 2\n");

			return false;	//Problemi
		}
		
	str_fiber = lista_fiber_iter->fiber;
	if (str_fiber->bitmap_fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 3\n");
		
		return false;	//Problemi
	}
	
	 if (index >= FLS_SIZE || index < 0){
		printk(KERN_INFO "DEBUG FLSFREE PROBLEMI 3\n");

		return false;	//Problemi
	 }

     if(str_fiber->bitmap_fls[index]== 0){
		printk(KERN_INFO "DEBUG FLSFREE PROBLEMI 4\n");

		return false;	//Problemi
	 }
     
     str_fiber->bitmap_fls[index]=0;

	
     return true;
}

static long flsGetValue(long pos){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	long ret;
	printk(KERN_INFO "DEBUG FLSGET \n");

	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	
	hash_for_each_possible_rcu(processi, fiber_processo, node, (int) pid){
		
		if(fiber_processo == NULL) {
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 1\n");

			return -1;	//Problemi
        }
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node, (int) tid){
		if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
			flag_fib=0;

			break;//Problemi
		}
		
        if (lista_fiber_iter->running == tid) {
			flag_fib=1;
            break;
        }
	}
	if (flag_fib==0){
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 2\n");

			return -1;	//Problemi
		}
		

	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 3\n");

		return -1;	//Problemi
	}
	if (str_fiber->bitmap_fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 4\n");
	
		return -1;	//Problemi
	}
	
	if (pos>=FLS_SIZE || pos<0) {
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 5\n");
		return -1;
	}
	
	if(str_fiber->bitmap_fls[pos] == 0){
	
		printk(KERN_INFO "DEBUG FLSGET PROBLEMI 6\n");
	
		return -1;	//Problemi
	
	}
	
	ret=str_fiber->fls[pos];
	
	
	printk(KERN_INFO "DEBUG FLSGET FINE, %ld\n",ret);
	return 	ret;
}

static void flsSetValue(long pos, long val){
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	struct Fiber_Processi* fiber_processo;
	struct Lista_Fiber* lista_fiber_iter;
	struct Fiber* str_fiber;
	printk(KERN_INFO "DEBUG FLSSET\n");

	
	//Itera sulla lista processi
	//Cerca processo corrente
	int flag=0;
	int flag_fib=0;
	
	hash_for_each_possible_rcu(processi, fiber_processo, node,(int) pid){
		
		if(fiber_processo == NULL) {
			flag=0;
			break;	//Problemi
        }
        if (fiber_processo->id == pid) {
			flag=1;
            break;
        }
	}
	
	if(flag == 0) {
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 1\n");

			return;	//Problemi
        }
	
	
	//Itera sulla lista fiber
	//Cerca fiber
	hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node,(int) tid){
		if (lista_fiber_iter == NULL || lista_fiber_iter->running == 0){
			flag_fib=0;

			break;//Problemi
		}
		
        if (lista_fiber_iter->running == tid) {
			flag_fib=1;
            break;
        }
	}
	if (flag_fib==0){
			printk(KERN_INFO "DEBUG FLSALLOC PROBLEMI 2\n");

			return;	//Problemi
		}
		
	
	str_fiber = lista_fiber_iter->fiber;
	
	if (str_fiber->fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 3\n");
	
		return;	//Problemi
	}
	if (str_fiber->bitmap_fls == NULL){
		//FLS NON ALLOCATO
		printk(KERN_INFO "DEBUG FLSSET PROBLEMI 3\n");
	
		return;	//Problemi
	}
	
	if (pos>=FLS_SIZE || pos<0) {
		 printk(KERN_INFO "DEBUG FLSSET PROBLEMI 5\n");
		return;
	}
	
	
	 if(str_fiber->bitmap_fls[pos]== 0){
		 printk(KERN_INFO "DEBUG FLSSET PROBLEMI 4\n");


		return;	//Problemi
	 }

	
	str_fiber->fls[pos] = val;
	printk(KERN_INFO "DEBUG FLSSET %d,%d\n",pos,str_fiber->fls[pos]);

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
