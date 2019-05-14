#include <linux/ioctl.h>
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
#include "Ioctl_interface.h"
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
#include <linux/smp.h>



#define BUFSIZE  100

//NOTA
//TGID == PARENT TGID == PROCESS ID
//PID == THERAD ID

#define DEV_NAME "fib_device"



#define FIB_LOG_LEN 1024

#define FLS_SIZE 128

DEFINE_HASHTABLE(processi, 3);

dev_t dev = 0;
static struct class *dev_class;
static struct cdev fib_cdev;


static int __init fib_driver_init(void);
static void __exit fib_driver_exit(void);
static int fib_open(struct inode *inode, struct file *file);
static int fib_release(struct inode *inode, struct file *file);
static ssize_t myread(struct file *file, char __user *ubuf,size_t count, loff_t *ppos);
static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static char *fiber_devnode(struct device *dev, umode_t *mode);


static pid_t fib_convert(void);
static struct Thread* do_thread_create(struct Fiber_Processi* str_processo, pid_t id);
static struct Fiber* do_fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo, int flag);
static struct Fiber* fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size);
static long fib_switch_to(pid_t id);

static long flsAlloc(void);
static bool flsFree(long index);
static long long flsGetValue(long pos);
static void flsSetValue(long pos, long long val);
unsigned long flags;
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
		rcu_read_lock();
		hash_for_each_possible_rcu(processi, fiber_processo, node, id){
		
			if (fiber_processo->id == id) {
				if(fiber_processo->fiber_stuff.len_fiber_stuff==0){
					regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;

				}else{
		
				printk(KERN_INFO "DEBUG PREREADIR 0\n");

				regs->dx=(unsigned long) fiber_processo->fiber_stuff.fiber_base_stuff;
				regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;
				}
			}
		}
		 rcu_read_unlock();
		
		
		
		
	
	
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
		 rcu_read_lock();
		hash_for_each_possible_rcu(processi, fiber_processo, node, id){
		
			if (fiber_processo->id == id) {
				if(fiber_processo->fiber_stuff.len_fiber_stuff==0){
					regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;

				}else{

		
					printk(KERN_INFO "DEBUG PRELOOKUP 0\n");
		
					regs->dx=(unsigned long) fiber_processo->fiber_stuff.fiber_base_stuff;
					regs->cx=(unsigned long) fiber_processo->fiber_stuff.len_fiber_stuff;
				}
			}
			}	
		 rcu_read_unlock();
	
	
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

	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;
	struct Fiber* running_fiber;

	printk(KERN_INFO "DEBUG EXIT \n");

	//Itera sulla lista processi
	//Cerca processo corrente
	
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node,current->tgid){
	
        if (fiber_processo->id == current->tgid) {
		hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
			
		
			if (lista_thread_iter->id == current->pid) {
				running_fiber=lista_thread_iter->runner;
				if(running_fiber!=NULL){
					spin_lock(&running_fiber->lock_fiber);

					//Salvataggio registri nel vecchio fiber
					memcpy(running_fiber->regs,current_pt_regs(),sizeof(struct pt_regs));

					//FPU
					fpu__save(running_fiber->fpu);
					
	
					//Manipolazione sistema interno
					lista_thread_iter->runner= NULL; // non è più running (vuol dire killato)
					spin_unlock(&running_fiber->lock_fiber);
					synchronize_rcu();

				}
				}
				}        
			}
	}
	 rcu_read_unlock();
	
	
	

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
	//printk(KERN_INFO "DEBUG OPEN\n");
		
	
	

	//Registrare processo in struttura
	//Creare gestore fiber per il processo
	struct Fiber_Processi* processo = (struct Fiber_Processi*) kzalloc(sizeof(struct Fiber_Processi),GFP_KERNEL);
	//processo->lock_fiber = __SPIN_LOCK_UNLOCKED(processo->lock_fiber);   

	processo->id = current->tgid;	//Current è globale
	int n_fib=1000;
	
	processo->last_fib_id=0;
	processo->fiber_stuff.fiber_base_stuff=kzalloc(sizeof(struct pid_entry)*(n_fib),GFP_KERNEL);
	processo->fiber_stuff.len_fiber_stuff=0;
	hash_init(processo->listafiber);   
	hash_init(processo->listathread);   

	rcu_read_lock();

	//Inserimento in lista
    hash_add_rcu(processi, &(processo->node), processo->id);  
	rcu_read_unlock();


	return 0;
}

static int fib_release(struct inode *inode, struct file *file){
	//printk(KERN_INFO "DEBUG RELEASE\n");
	//Rilasciare processo da struttura
	int i;
	int j;
	int m;

	//Rimozione da lista

	struct Fiber_Processi* bersaglio;
	struct Thread* bersaglio_thread;

	struct Fiber* bersaglio_fiber;
	struct hlist_node* processo_node;
	struct hlist_node* thread_node;
	struct hlist_node* fib_node;

	rcu_read_lock();
	hash_for_each_safe(processi, i, processo_node, bersaglio, node){
		
        if (bersaglio->id == current->tgid) {
			hash_for_each_safe(bersaglio->listathread, j, thread_node, bersaglio_thread, node){

				hash_del_rcu(&(bersaglio_thread->node));
		
			
				if(bersaglio_thread!=NULL){
				kfree(bersaglio_thread);
				}

			}
			//Deallocazione profonda di bersaglio
			//Deallocazione lista fiber
			hash_for_each_safe(bersaglio->listafiber, m, fib_node, bersaglio_fiber, node){

				if(bersaglio_fiber->regs!=NULL){
					kfree(bersaglio_fiber->regs);
				}
				if(bersaglio_fiber->fpu!=NULL){
				
					kfree(bersaglio_fiber->fpu);
				}
				if(bersaglio_fiber->bitmap_fls!=NULL){
					kfree(bersaglio_fiber->bitmap_fls);
				}
				
				if(bersaglio_fiber->fls!=NULL){
					kfree(bersaglio_fiber->fls);
				}
				hash_del_rcu(&(bersaglio_fiber->node));
				if(bersaglio_fiber!=NULL){

				kfree(bersaglio_fiber);
				}
			}
			if(bersaglio->fiber_stuff.fiber_base_stuff!=NULL){

				kfree(bersaglio->fiber_stuff.fiber_base_stuff);
			}

		hash_del_rcu(&(bersaglio->node));
		if(bersaglio!=NULL){
			kfree(bersaglio);
		}

		}		

	}
	rcu_read_unlock();
	

	//printk(KERN_INFO "DEBUG RELEASEOK\n");
	

	return 0;
}

static ssize_t myread(struct file *filp, char __user *buf, size_t len, loff_t *off){
	
	pid_t pid;
	long tmp_pid;
	unsigned long fib_id;
	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;
	struct Fiber* lista_fiber_iter;
	size_t i=-1;
	char fiber_stat[FIB_LOG_LEN] = "";
	
	printk(KERN_INFO "DEBUG READFILE\n");
	
	//Ottiene gli id (fiber id e pid)
	
	kstrtoul(filp->f_path.dentry->d_name.name+1,10, &fib_id);
	kstrtol(filp->f_path.dentry->d_parent->d_parent->d_name.name,10, &tmp_pid);
	pid=tmp_pid;
	printk(KERN_INFO "DEBUG READFILE parent %s\n",filp->f_path.dentry->d_name.name);

	//Itera sulla lista processi
	//Cerca processo corrente
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, pid){
		
        if (fiber_processo->id == pid) {
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
				if (lista_thread_iter->id == current->pid) {
					char* running_str;

					if (lista_thread_iter->runner!=NULL){
						running_str = "";
					} else {
						running_str = "not ";
					}
					hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter, node, fib_id){
							if (lista_fiber_iter->id == fib_id) {
								sprintf(fiber_stat,"##FIBER STATISTICS##\n\n - This fiber is currently %srunning.\n - The initial entry point is: %p\n - This fiber was created by: %lu (pid)\n - Succesful activations: %lu\n - Unsuccesful activations: %lu\n - Total running time: %lu msec\n\0",running_str,lista_fiber_iter->entry_point,lista_fiber_iter->creator,lista_fiber_iter->correct_counter,lista_fiber_iter->failed_counter,lista_fiber_iter->exec_time/1000);

								if (*off < strnlen(fiber_stat, FIB_LOG_LEN)){
			
									i = min_t(size_t, len, strnlen(fiber_stat, FIB_LOG_LEN));
									if (copy_to_user(buf, fiber_stat, i)==0) {
										
										*off += i;
									}
								}
							}
				}
		}
		
        }
	}
	}
	 rcu_read_unlock();
	
	return i;
}


static long fib_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	//printk(KERN_INFO "DEBUG IOCTL CMD %d\n",cmd);
	if(cmd == FIB_FLS_ALLOC){
			struct fiber_arguments fa;
			long ret;
			//printk(KERN_INFO "DEBUG IOCTL FIB_FLS_ALLOC\n");
			fa.fls_index = (long)flsAlloc();
			if (copy_to_user((void*)arg, &fa,sizeof(struct fiber_arguments))){
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET PROBLEMI 3\n");
                    return -1;
			}
			return 0;
	}else if(cmd == FIB_FLS_GET){
			struct fiber_arguments fa;
			long ret;
			if (!access_ok(VERIFY_READ, arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET PROBLEMI 1\n");
                    return -1;
			}
            if (copy_from_user(&fa, (void*)arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET PROBLEMI 2\n");
                    return -1;
            }
      		//printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET\n");
			fa.fls_value = (long long) flsGetValue(fa.fls_index);//pos
			if (copy_to_user((void*)arg,&fa,sizeof(struct fiber_arguments))){
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_GET PROBLEMI 3\n");
                    return -1;
			}
			return 0;
			
		}else if(cmd == FIB_FLS_SET){
			struct fiber_arguments fa;
			if (!access_ok(VERIFY_READ, arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET PROBLEMI 1\n");

					return -1;
            }
			if (copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments))){
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET PROBLEMI 2\n");

				    return -1;

			}
			//printk(KERN_INFO "DEBUG IOCTL FIB_FLS_SET\n");
			flsSetValue(fa.fls_index,fa.fls_value);//pos, val
			return 0;
		}else if(cmd == FIB_FLS_DEALLOC){
			struct fiber_arguments fa;
			long ret;
			if (!access_ok(VERIFY_READ, arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC PROBLEMI 1\n");

                    return -1;
            }
			if (copy_from_user(&fa ,(void*)arg, sizeof(struct fiber_arguments))){
					printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC PROBLEMI 2\n");

			        return -1;

			}
			//printk(KERN_INFO "DEBUG IOCTL FIB_FLS_DEALLOC\n");
			ret=(long)flsFree(fa.fls_index);
			return ret;
		}else if (cmd ==FIB_CONVERT){
			//printk(KERN_INFO "DEBUG IOCTL FIB_CONVERT\n");
			//Controlla se è già un fiber
			//Chiamata a FIB_CREATE
			//Chiama FIB_SWITCH_TO
			long ret=(long) fib_convert();
			return ret;
		}else if (cmd == FIB_CREATE){
			//PASSA PARAMETRO CON STRUCT
			struct fiber_arguments fa;
			long ret;
			if (!access_ok(VERIFY_READ, arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_CREATE PROBLEMI 1\n");

                    return -1;
             }
             if (copy_from_user(&fa, (void*)arg, sizeof(struct fiber_arguments))) {
				 	printk(KERN_INFO "DEBUG IOCTL FIB_CREATE PROBLEMI 2\n");

                  return -1;
             }
			//printk(KERN_INFO "DEBUG IOCTL FIB_CREATE\n");
			//Allocazione e popolamento struttra Fiber
			//Inserimento oggetto in gestore
			//printk(KERN_INFO "DEBUG IOCTL FIB_CREATE ARG %p\n", fa.start_function_address);

			fa.fiber_id=fib_create(fa.start_function_address,fa.start_function_parameters,fa.stack_pointer,fa.stack_size)->id;
			  if (copy_to_user((void*)arg,&fa,sizeof(struct fiber_arguments))){
				 	printk(KERN_INFO "DEBUG IOCTL FIB_CREATE PROBLEMI 3\n");
				  
					return -1;
			  }
			  return 0;
		}else if (cmd == FIB_SWITCH_TO){
			struct fiber_arguments fa;
			long ret;
			if (!access_ok(VERIFY_READ, arg, sizeof(struct fiber_arguments))) {
					printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO PROBLEMI 1\n");

                    return -1;
             }
             if (copy_from_user(&fa, (void*)arg, sizeof(struct fiber_arguments))) {
			 	printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO PROBLEMI 2\n");

                  return -1;
             }
			//printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO\n");
			//Controllo se il fiber è running
			//Cambiamento di stato macchina

			ret=(long) fib_switch_to(fa.fiber_id);
			return ret;
	}  else {
			printk(KERN_INFO "DEBUG IOCTL PROBLEMI\n");
               return -EINVAL;
        }
}

//Funzioni ausiliarie
static pid_t fib_convert(){

	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;
	struct Thread* lista_thread_elem;
	struct Fiber* lista_fiber_elem;
	struct timespec* time_str;
	struct pt_regs* my_regs;
	int i;
	pid_t ret=-1;
	int flag_controllo=0;
	//Itera sulla lista processi
	//Cerca processo corrente
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, current->tgid){
	
        if (fiber_processo->id == current->tgid) {
			//Itera sulla lista fiber
			//Controlla se sono già un fiber
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
				if (lista_thread_iter->id == current->pid) {
					flag_controllo=1;
					break;
				}

			}
			if(flag_controllo==0){
				
				//Crea nuovo fiber
				int stack_size=1;
				my_regs = current_pt_regs();
				lista_thread_elem = do_thread_create(fiber_processo, current->pid);
				lista_fiber_elem = do_fib_create((void*)my_regs->ip,0,0,-1,fiber_processo,0);

				//Manipolazione stato macchina
				//printk(KERN_INFO "DEBUG CONVERT IP %p\n",my_regs->ip);
				
			
				time_str = kzalloc(sizeof(struct timespec),GFP_KERNEL);
				getnstimeofday(time_str);
				lista_fiber_elem->last_activation_time = time_str->tv_nsec;
				lista_fiber_elem->correct_counter = 1;
				lista_fiber_elem->selected_thread = lista_thread_elem;
				kfree(time_str);
				ret = lista_fiber_elem->id;
				lista_thread_elem->runner = lista_fiber_elem;

				//Collega entry in lista
				hash_add_rcu(fiber_processo->listafiber,&(lista_fiber_elem->node), lista_fiber_elem->id);

	
				}
			}
	}
	rcu_read_unlock();
	
	return 	ret;
	
}
static struct Thread* do_thread_create(struct Fiber_Processi* str_processo, pid_t id){
	
	//printk(KERN_INFO "DEBUG DOTHREADCREATE\n");
	struct Thread* str_thread =kzalloc(sizeof(struct Thread),GFP_KERNEL);
	str_thread->id=id;
	str_thread->runner=NULL;
	rcu_read_lock();
	hash_add_rcu(str_processo->listathread,&(str_thread->node), str_thread->id);
	rcu_read_unlock();

	return str_thread;

}

static struct Fiber* do_fib_create(void* func,void* parameters, void *stack_pointer, unsigned long stack_size,struct Fiber_Processi* str_processo,int flag){
	//printk(KERN_INFO "DEBUG DOFIBCREATE\n");

	
	pid_t pid = current->tgid;
	pid_t tid = current->pid;
	


	
	//Crea Fiber str

	struct Fiber* str_fiber =kzalloc(sizeof(struct Fiber),GFP_KERNEL);
	str_fiber->lock_fiber = __SPIN_LOCK_UNLOCKED(lock_fiber);   
   	//spin_lock(&str_fiber->lock_fiber);
   	
   	str_fiber->regs = kzalloc(sizeof(struct pt_regs),GFP_KERNEL);
	memcpy(str_fiber->regs, current_pt_regs(), sizeof(struct pt_regs)); 
	
	//FPU
	str_fiber->fpu = kzalloc(sizeof(struct fpu),GFP_KERNEL);
	copy_fxregs_to_kernel(str_fiber->fpu);    
	
	
	//Crea ptregs str
	if(flag==1){
		str_fiber->regs->ip = (unsigned long)func;	//Assegna instruction pointer alla funzione passata
		str_fiber->regs->sp=(unsigned long) stack_pointer + stack_size - 8;
		str_fiber->regs->bp =(unsigned long) str_fiber->regs->sp;
		str_fiber->regs->di=(unsigned long)parameters;
	}

	//Assegna fiber id
	
	str_processo->last_fib_id += 1;
	str_fiber->id = str_processo->last_fib_id;

	//
	str_fiber->entry_point = func;
	str_fiber->creator = tid;
	str_fiber->correct_counter = 0;
	str_fiber->failed_counter =0;
	str_fiber->exec_time = 0;
	str_fiber->last_activation_time = 0;
	str_fiber->fls= kzalloc(sizeof(long long)*FLS_SIZE,GFP_KERNEL);
	str_fiber->bitmap_fls= kzalloc(sizeof(unsigned long)*FLS_SIZE,GFP_KERNEL);

	//printk(KERN_INFO "DEBUG CREATE FUNC %p\n", func);
	//printk(KERN_INFO "DEBUG CREATE ID %d\n", str_fiber->id);


	
   	//spin_unlock(&str_fiber->lock_fiber);


	return str_fiber; 
}


static struct Fiber* fib_create(void* func, void* parameters,void *stack_pointer, unsigned long stack_size){
	struct Fiber_Processi* fiber_processo;
	struct Fiber* lista_fiber_elem;
	struct pid_entry fibers_entry_log;

	int i;
	//Itera sulla lista processi
	//Cerca processo corrente
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node,  current->tgid){
        if (fiber_processo->id == current->tgid) {
           //Crea Fiber str
			i=fiber_processo->fiber_stuff.len_fiber_stuff;

			lista_fiber_elem=do_fib_create(func, parameters, stack_pointer, stack_size,fiber_processo,1);
			
			lista_fiber_elem->selected_thread=NULL;

			fibers_entry_log.name=kmalloc(sizeof(char)*16,GFP_KERNEL);
			sprintf (fibers_entry_log.name, "f%lu", lista_fiber_elem->id);
			fibers_entry_log.len=strlen(fibers_entry_log.name);
			fibers_entry_log.mode=(S_IFREG|(S_IRUGO));
			fibers_entry_log.iop = NULL;
			fibers_entry_log.fop = &fops_proc;
			fiber_processo->fiber_stuff.fiber_base_stuff[i]=fibers_entry_log;
			fiber_processo->fiber_stuff.len_fiber_stuff=i+1;
			hash_add_rcu(fiber_processo->listafiber,&(lista_fiber_elem->node), lista_fiber_elem->id);

        }

	}
	rcu_read_unlock();

	return lista_fiber_elem; //Controllo

}
static long fib_switch_to(pid_t id){

	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;

	struct Fiber* lista_fiber_iter_new;
	struct Fiber* lista_fiber_iter_old;

	struct timespec* time_str;
	//printk(KERN_INFO "DEBUG SWITCH %d\n", id);

	//Itera sulla lista processi
	//Cerca processo corrente
	long ret=-1;
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node,current->tgid){
		
        if (fiber_processo->id == current->tgid) {
			
            //Itera sulla lista fiber
			//Cerca vecchio fiber
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
		
			if (lista_thread_iter->id == current->pid) {

				if(lista_thread_iter->runner!=NULL){ 
				lista_fiber_iter_old=lista_thread_iter->runner;
				spin_lock(&lista_fiber_iter_old->lock_fiber);
			

				//Manipolazione stato macchina			
                //Itera sulla lista fiber
				//Cerca nuovo fiber
				if(lista_fiber_iter_old->id == id){
							
					printk(KERN_INFO "DEBUG SWITCH PROBLEMI 4\n");

					lista_fiber_iter_old->failed_counter += 1;

					ret= -1;	//Problemi				
				}else{

					hash_for_each_possible_rcu(fiber_processo->listafiber, lista_fiber_iter_new, node, id){
	
					 if (lista_fiber_iter_new->id == id) {
						 spin_lock(&lista_fiber_iter_new->lock_fiber);

						if (lista_fiber_iter_new->selected_thread != NULL){
							//Già running
							printk(KERN_INFO "DEBUG SWITCH PROBLEMI 5\n");

							lista_fiber_iter_old->failed_counter += 1;

							ret= -1;	//Problemi
							
						}else{

								//printk(KERN_INFO "DEBUG SWITCH IP PRIMA %p %d %d\n",current_pt_regs()->ip, current->pid, id);

								//Salvataggio registri nel vecchio fiber
								memcpy(lista_fiber_iter_old->regs,current_pt_regs(),sizeof(struct pt_regs));

								//FPU

								copy_fxregs_to_kernel(lista_fiber_iter_old->fpu);
								lista_fiber_iter_old->selected_thread=NULL;


							
								memcpy(current_pt_regs(),lista_fiber_iter_new->regs,sizeof(struct pt_regs));

								//FPU
								//Manipolazione sistema interno
								copy_kernel_to_fxregs(&(lista_fiber_iter_new->fpu->state.fxsave));
								lista_fiber_iter_new->selected_thread=lista_thread_iter;

								lista_thread_iter->runner=lista_fiber_iter_new;
								//printk(KERN_INFO "DEBUG SWITCH IP DOPO %p %d %d\n",current_pt_regs()->ip, current->pid, id);
							
								//printk(KERN_INFO "DEBUG SWITCHOK\n");


								ret=0;
							}
							spin_unlock(&lista_fiber_iter_new->lock_fiber);

							}

						}

					}
					spin_unlock(&lista_fiber_iter_old->lock_fiber);


								synchronize_rcu();
		      
				}


					}
			

			}

		}
	}
	
	rcu_read_unlock();

	return ret;
	/*	
	time_str= kmalloc(sizeof(struct timespec),GFP_KERNEL);
	getnstimeofday(time_str);
	
	lista_fiber_iter_new->correct_counter += 1;
	lista_fiber_iter_new->last_activation_time =time_str->tv_nsec;
	lista_fiber_iter_old->exec_time += ((lista_fiber_iter_new->last_activation_time)-(lista_fiber_iter_old->last_activation_time));
	
	kfree(time_str);*/
	
}      



//FLS
static long flsAlloc(){

	
	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;

	long index=-1;
	//printk(KERN_INFO "DEBUG FLSALLOC\n");
	int i;


	
	//Itera sulla lista processi
	//Cerca processo corrente
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, current->tgid){
		
        if (fiber_processo->id == current->tgid) {
            
			//Itera sulla lista fiber
			//Cerca fiber
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
	//			printk(KERN_INFO "DEBUG FLSALLOCNUM %d\n",c);
				if (lista_thread_iter->id == current->pid) {
					if(lista_thread_iter->runner!=NULL){
					spin_lock(&lista_thread_iter->runner->lock_fiber);

					if(lista_thread_iter->runner->fls!=NULL &&  lista_thread_iter->runner->bitmap_fls!=NULL){

						
						for(i=0; i<FLS_SIZE; i++){								
							//printk(KERN_INFO "DEBUG FLSALLOCINDEX %d %d\n",i,lista_thread_iter->runner->bitmap_fls[i]);

							if(lista_thread_iter->runner->bitmap_fls[i]==0){
								index=i;
								break;
							}
						}
						if (index < FLS_SIZE){
							lista_thread_iter->runner->bitmap_fls[index]=1;
							//printk(KERN_INFO "DEBUG FLSALLOC %d %d\n", index,lista_thread_iter->runner->id);
		
						}
		
					}
					spin_unlock(&lista_thread_iter->runner->lock_fiber);
					synchronize_rcu();
				
					}
				}
			}	
		}
		}	

	rcu_read_unlock();

	return index;
	
}

static bool flsFree(long index){

	struct Fiber_Processi* fiber_processo;
	struct Fiber* running_fiber;
	struct Thread* lista_thread_iter;
	//printk(KERN_INFO "DEBUG FLSFREE \n");
	bool ret=false;
	
	//Itera sulla lista processi
	//Cerca processo corrente
	
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, current->tgid){

        if (fiber_processo->id == current->tgid) {
            
			//Itera sulla lista fiber
			//Cerca fiber
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
	
				if (lista_thread_iter->id== current->pid) {
					if(lista_thread_iter->runner!=NULL){

					 if(lista_thread_iter->runner->fls!=NULL  && lista_thread_iter->runner->bitmap_fls!=NULL){
							//printk(KERN_INFO "DEBUG FLSFREE %d\n",lista_thread_iter->runner->id);

	    
						if (index < FLS_SIZE || index >= 0){

							if( lista_thread_iter->runner->bitmap_fls[index]!= 0){
								spin_lock(&lista_thread_iter->runner->lock_fiber);

								lista_thread_iter->runner->bitmap_fls[index]=0;
								spin_unlock(&lista_thread_iter->runner->lock_fiber);

								ret=true;
							}
						}

				

				}
					synchronize_rcu();
			}
		}
	}
	}
	}
	rcu_read_unlock();

	return ret;

}

static long long flsGetValue(long pos){
	
	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;
	//printk(KERN_INFO "DEBUG FLSGET \n");

	long long ret=-1;
	//Itera sulla lista processi
	//Cerca processo corrente
	
	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, current->tgid){
		
        if (fiber_processo->id == current->tgid) {

			//Itera sulla lista fiber
			//Cerca fiber
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
		
				if (lista_thread_iter->id == current->pid) {
					if(lista_thread_iter->runner!=NULL){

						if(lista_thread_iter->runner->fls!=NULL && lista_thread_iter->runner->bitmap_fls!=NULL){
							//printk(KERN_INFO "DEBUG FLSGET %d\n",lista_thread_iter->runner->id);


						if (pos<FLS_SIZE || pos>=0) {
					
							if(lista_thread_iter->runner->bitmap_fls[pos] != 0){
								//spin_lock(&lista_thread_iter->runner->lock_fiber);

								ret=lista_thread_iter->runner->fls[pos]; 
								//printk(KERN_INFO "DEBUG FLSGET FINE, %ld %lli\n",pos,lista_thread_iter->runner->fls[pos]);

								//spin_unlock(&lista_thread_iter->runner->lock_fiber);
								//synchronize_rcu();

							}

						

					}

				}
				}
			}
		}
	}
	}
	rcu_read_unlock();

	return ret;

}

static void flsSetValue(long pos, long long val){
	
	struct Fiber_Processi* fiber_processo;
	struct Thread* lista_thread_iter;
	//printk(KERN_INFO "DEBUG FLSSET\n");

	//Itera sulla lista processi
	//Cerca processo corrente

	rcu_read_lock();
	hash_for_each_possible_rcu(processi, fiber_processo, node, current->tgid){
		
	
        if (fiber_processo->id ==  current->tgid) {
			//Itera sulla lista fiber
			//Cerca fiber
	
			hash_for_each_possible_rcu(fiber_processo->listathread, lista_thread_iter, node, current->pid){
					
				if (lista_thread_iter->id == current->pid) {
						if(lista_thread_iter->runner!=NULL){

						if(  lista_thread_iter->runner->fls!=NULL  && lista_thread_iter->runner->bitmap_fls!=NULL){
						//printk(KERN_INFO "DEBUG FLSSET %d %ld %lld\n",lista_thread_iter->runner->id,pos, val);

						
						if (pos<FLS_SIZE || pos>=0) {
			
							if(lista_thread_iter->runner->bitmap_fls[pos]!= 0){
								spin_lock(&lista_thread_iter->runner->lock_fiber);

								lista_thread_iter->runner->fls[pos]=val;
								spin_unlock(&lista_thread_iter->runner->lock_fiber);
			  
							}		
						}
				


				}
				
					synchronize_rcu();	
				
					}
				}
			}
	}
	}
	rcu_read_unlock();

	return;

}

static char *fiber_devnode(struct device *dev, umode_t *mode)
{
        if (mode)
                *mode = 0666;
        return kasprintf(GFP_KERNEL, "%s", dev_name(dev));
}

//INIT-EXIT
static int fib_driver_init(void){
	
	printk(KERN_INFO "DEBUG INIT\n");
	

	//Allocating Major number
	if((alloc_chrdev_region(&dev, 0, 1, DEV_NAME)) <0){
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
	dev_class->devnode = fiber_devnode;

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
    /*
    kp_exit.pre_handler = Pre_Handler_Exit; 
    kp_exit.symbol_name = "do_exit";
    register_kprobe(&kp_exit);
    * */

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
	//unregister_kprobe(&kp_readdir);
	//unregister_kprobe(&kp_lookup);

	//unregister_kprobe(&kp_exit);

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
