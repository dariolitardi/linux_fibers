#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_INFO
#include <linux/init.h>		// Macro __init,__exit
//#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define NAME "lkmc_character_device_create"//"MyDevice"

// Variabili

static int major = -1;
static struct cdev mycdev;
static struct class *myclass = NULL;

// Funzioni

static int open(struct inode *inode, struct file *file){
	printk(KERN_INFO "TEST OPEN\n");
	try_module_get(THIS_MODULE);
	return 0;
}

static int release(struct inode *inode, struct file *file){
	printk(KERN_INFO "TEST RELEASE\n");
	module_put(THIS_MODULE);
	return 0;
}

static long ioctl_handler(	struct file *file,			/* ditto */
					unsigned int ioctl_num,		/* number and param for ioctl */
					unsigned long ioctl_param){
	switch(ioctl_num){
	case 1:
		printk(KERN_INFO "TEST IOCTL1\n");
	break;
	case 2:
		printk(KERN_INFO "TEST IOCTL2\n");
	break;
	}
	return 0;
}

// Strutture

const struct file_operations fops ={
	.open		= open, //try_module_get(THIS)
	.release	= release, //module_put(THIS)
	.read		= NULL,
	.write		= NULL,
	.unlocked_ioctl		= ioctl_handler, //K3.18 ++
};

// Agganci

static void cleanup(int device_created){
	if (device_created) {
		device_destroy(myclass, major);
		cdev_del(&mycdev);
	}
	if (myclass)
		class_destroy(myclass);
	if (major != -1)
		unregister_chrdev_region(major, 1);
}

static int __init ingresso(void){
	//int num = register_chrdev(66,"mytest",&fops);
	printk(KERN_INFO "TEST CARICAMENTO\n");

	int device_created = 0;

	/* cat /proc/devices */
	if (alloc_chrdev_region(&major, 0, 1, NAME "_proc") < 0)
		goto error;
	/* ls /sys/class */
	if ((myclass = class_create(THIS_MODULE, NAME "_sys")) == NULL)
		goto error;
	/* ls /dev/ */
	if (device_create(myclass, NULL, major, NULL, NAME "_dev") == NULL)
		goto error;
	device_created = 1;
	cdev_init(&mycdev, &fops);
	if (cdev_add(&mycdev, major, 1) == -1)
		goto error;

	printk(KERN_INFO "TEST CARICATO\n");
	return 0;
error:
	cleanup(device_created);
	return -1;
}

static void __exit uscita(void){
	printk(KERN_INFO "TEST RIMOZIONE\n");

	cleanup(1);

	printk(KERN_INFO "TEST RIMOSSO\n");
}

module_init(ingresso);
module_exit(uscita);

/*
Oct  4 12:05:18 aosv kernel: [ 8876.368433] Test: Unknown symbol __class_create (err 0)
Oct  4 12:05:18 aosv kernel: [ 8876.368447] Test: Unknown symbol class_destroy (err 0)
Oct  4 12:05:18 aosv kernel: [ 8876.368462] Test: Unknown symbol device_create (err 0)
Oct  4 12:05:18 aosv kernel: [ 8876.368474] Test: Unknown symbol device_destroy (err 0)
*/
