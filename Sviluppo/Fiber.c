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
#include "Strutture.h"


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
static void fib_create(void);
static void fib_switch_to(void);

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

//Aggiungere i lock
static int fib_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEBUG OPEN\n");
	//Registrare processo in struttura
	//Creare gestore fiber per il processo
	struct Fiber_Processi* processo = (struct Fiber_Processi*) kmalloc(sizeof(struct Fiber_Processi),GFP_KERNEL);
	memset(processo,0,sizeof(struct Fiber_Processi));	// pulisce la lista dei processi (la nuova entry) per sicurezza perche potrebbe essere ancora in memoria
	processo->id = current->pid;	//Current è globale

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
	pid_t pid = current->pid;

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
			fib_create();
			break;
		case FIB_SWITCH_TO:
			printk(KERN_INFO "DEBUG IOCTL FIB_SWITCH_TO\n");
			//Controllo se il fiber è running
			//Cambiamento di stato macchina
			fib_switch_to();
		break;
	}
	return 0;
}

//Funzioni ausiliarie
static void fib_convert(){
	int num = 0;
	struct Fiber_Processi* tmp = Lista_Processi;
	struct Fiber_Processi* tmpNum = Lista_Processi;
	while (tmp != NULL && tmp->id != current->pid){
		tmp = tmp->next;
		num = 0;
	}
	tmpNum = tmp;
	while (tmpNum != NULL){
		tmpNum = tmpNum->next;
		num++;
	}
	printk(KERN_INFO "DEBUG PID %d\n",current->pid);
	printk(KERN_INFO "DEBUG ELEMS %d\n",num);
	printk(KERN_INFO "DEBUG STRUCT %pK\n",tmp);
}

static void fib_create(){

}

static void fib_switch_to(){

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


static int __init fib_driver_init(void){
	printk(KERN_INFO "DEBUG INIT\n");
	/*Allocating Major number*/
	if((alloc_chrdev_region(&dev, 0, 1, "fib_Dev")) <0){
		printk(KERN_INFO "Cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&fib_cdev,&fops);
	fib_cdev.owner = THIS_MODULE;
	fib_cdev.ops = &fops;

	/*Adding character device to the system*/
	if((cdev_add(&fib_cdev,dev,1)) < 0){
		printk(KERN_INFO "Cannot add the device to the system\n");
		goto r_class;
	}

	/*Creating struct class*/
	if((dev_class = class_create(THIS_MODULE,"fib_class")) == NULL){
		printk(KERN_INFO "Cannot create the struct class\n");
		goto r_class;
	}

	/*Creating device*/
	if((device_create(dev_class,NULL,dev,NULL,DEV_NAME)) == NULL){ // qui si crea il file nel filesystem dev che viene messe in memoria con le sue strutture
		printk(KERN_INFO "Cannot create the Device 1\n");
		goto r_device;
	}
	printk(KERN_INFO "Device Driver Insert...Done!!!\n");
	return 0;
r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev,1);
	return -1;
}

void __exit fib_driver_exit(void){
	printk(KERN_INFO "DEBUG EXIT\n");
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
