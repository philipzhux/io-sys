#define IRQ_NUM 1
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include "ioc_hw5.h"

MODULE_LICENSE("GPL");

#define PREFIX_TITLE "OS_AS5"
#define MY_STUID 119010486

// DMA
#define DMA_BUFSIZE 64
#define DMASTUIDADDR 0x0        // Student ID
#define DMARWOKADDR 0x4         // RW function complete
#define DMAIOCOKADDR 0x8        // ioctl function complete
#define DMAIRQOKADDR 0xc        // ISR function complete
#define DMACOUNTADDR 0x10       // interrupt count function complete
#define DMAANSADDR 0x14         // Computation answer
#define DMAREADABLEADDR 0x18    // READABLE variable for synchronize
#define DMABLOCKADDR 0x1c       // Blocking or non-blocking IO
#define DMAOPCODEADDR 0x20      // data_a opcode
#define DMAOPERANDBADDR 0x21    // data_b operand1
#define DMAOPERANDCADDR 0x25    // data_c operand2
void *dma_buf;
int irqq_count;
// Declaration for file operations
static ssize_t drv_read(struct file *filp, char __user *buffer, size_t, loff_t*);
static int drv_open(struct inode*, struct file*);
static ssize_t drv_write(struct file *filp, const char __user *buffer, size_t, loff_t*);
static int drv_release(struct inode*, struct file*);
static long drv_ioctl(struct file *, unsigned int , unsigned long );

// cdev file_operations
static struct file_operations fops = {
      owner: THIS_MODULE,
      read: drv_read,
      write: drv_write,
      unlocked_ioctl: drv_ioctl,
      open: drv_open,
      release: drv_release,
};
static struct DEVICE_UNIQUE{} dev_unique;
// in and out function
void myoutc(unsigned char data,unsigned short int port);
void myouts(unsigned short data,unsigned short int port);
void myouti(unsigned int data,unsigned short int port);
unsigned char myinc(unsigned short int port);
unsigned short myins(unsigned short int port);
unsigned int myini(unsigned short int port);

// Work routine
static struct work_struct *work;

// For input data structure
typedef struct{
    char a;
    int b;
    short c;
} DataIn;


// DEVICE
#define DEV_NAME "mydev"        // name for alloc_chrdev_region
#define DEV_BASEMINOR 0         // baseminor for alloc_chrdev_region
#define DEV_COUNT 1             // count for alloc_chrdev_region
static int dev_major;
static int dev_minor;
static struct cdev *dev_cdev;



// Arithmetic funciton
static void drv_arithmetic_routine(struct work_struct* ws);


// Input and output data from/to DMA
void myoutc(unsigned char data,unsigned short int port) {
    *(volatile unsigned char*)(dma_buf+port) = data;
}
void myouts(unsigned short data,unsigned short int port) {
    *(volatile unsigned short*)(dma_buf+port) = data;
}
void myouti(unsigned int data,unsigned short int port) {
    *(volatile unsigned int*)(dma_buf+port) = data;
}



unsigned char myinc(unsigned short int port) {
    return *(volatile unsigned char*)(dma_buf+port);
}
unsigned short myins(unsigned short int port) {
    return *(volatile unsigned short*)(dma_buf+port);
}
unsigned int myini(unsigned short int port) {
    return *(volatile unsigned int*)(dma_buf+port);
}

static int prime(int base, short nth)
{
    int fnd=0;
    int i, num, isPrime;

    num = base;
    while(fnd != nth) {
        isPrime=1;
        num++;
        for(i=2;i<=num/2;i++) {
            if(num%i == 0) {
                isPrime=0;
                break;
            }
        }
        
        if(isPrime) {
            fnd++;
        }
    }
    return num;
}

static int drv_open(struct inode* ii, struct file* ff) {
	try_module_get(THIS_MODULE);
    	printk("%s:%s(): device open\n", PREFIX_TITLE, __func__);
	return 0;
}
static int drv_release(struct inode* ii, struct file* ff) {
	module_put(THIS_MODULE);
    	printk("%s:%s(): device close\n", PREFIX_TITLE, __func__);
	return 0;
}
static ssize_t drv_read(struct file *filp, char __user *buffer, size_t ss, loff_t* lo) {
	/* Implement read operation for your device */
	int ans;
	ans = myini(DMAANSADDR);
	put_user(ans,(int*) buffer);
	myouti(0,DMAREADABLEADDR);
	return 0;
}
static ssize_t drv_write(struct file *filp, const char __user *buffer, size_t ss, loff_t* lo) {
	DataIn data_k;
	int IOMode;
	IOMode = myini(DMABLOCKADDR);
	copy_from_user(&data_k,buffer,sizeof(DataIn));
	myoutc(data_k.a,DMAOPCODEADDR);
	myouti(data_k.b,DMAOPERANDBADDR);
	myouts(data_k.c,DMAOPERANDCADDR);
	INIT_WORK(work, drv_arithmetic_routine);
	myouti(0,DMAREADABLEADDR);
	// Decide io mode
	if(IOMode) {
		// Blocking IO
		printk("%s:%s(): queue work\n", PREFIX_TITLE, __func__);
		schedule_work(work);
		printk("%s:%s(): block\n", PREFIX_TITLE, __func__);
		flush_scheduled_work();
    	} 
	else {
		// Non-locking IO
		printk("%s:%s(): queue work\n", PREFIX_TITLE, __func__);
		schedule_work(work);
   	 }
	return 0;
}
static long drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int user_input;
	switch(cmd){
		case HW5_IOCSETSTUID:
		get_user(user_input, (int *) arg);
		myouti(user_input,DMASTUIDADDR);
		printk("%s:%s(): My STUID is = %d\n", PREFIX_TITLE, __func__,myini(DMASTUIDADDR));
		break;
		case HW5_IOCSETIOCOK:
		printk("%s:%s(): IOC OK\n", PREFIX_TITLE, __func__);
		break;
		case HW5_IOCSETIRQOK:
		printk("%s:%s(): IRQ OK\n", PREFIX_TITLE, __func__);
		break;
		case HW5_IOCSETRWOK:
		printk("%s:%s(): RM OK\n", PREFIX_TITLE, __func__);
		break;
		case HW5_IOCSETBLOCK:
		get_user(user_input, (int *) arg);
		myouti(user_input,DMABLOCKADDR);
		if(user_input) printk("%s:%s(): Blocking IO\n", PREFIX_TITLE, __func__);
		else printk("%s:%s(): Non-Blocking IO\n", PREFIX_TITLE, __func__);
		break;
		case HW5_IOCWAITREADABLE:
		printk("%s:%s(): wait readable 1\n", PREFIX_TITLE, __func__);
		while(!myini(DMAREADABLEADDR)) msleep(200);
		put_user(1,(int*) arg);
		break;
	}
	return 0;
}

static void drv_arithmetic_routine(struct work_struct* ws) {
	/* Implement arthemetic routine */
	int ans;
	char data_a = myini(DMAOPCODEADDR);
	int data_b = myini(DMAOPERANDBADDR);
	short data_c = myini(DMAOPERANDCADDR);
	switch(data_a) {
        case '+':
            ans=data_b+data_c;
            break;
        case '-':
            ans=data_b-data_c;
            break;
        case '*':
            ans=data_b*data_c;
            break;
        case '/':
            ans=data_b/data_c;
            break;
        case 'p':
            ans = prime(data_b, data_c);
            break;
        default:
            ans=0;
    }
	printk("%s:%s(): %d %c %d = %d\n", PREFIX_TITLE, __func__, data_b, data_a, data_c, ans);
	myouti(ans,DMAANSADDR);
	myouti(1,DMAREADABLEADDR);
}

void irq_handler(int irq, void *dev_id, struct pt_regs *regs){
	irqq_count++;
}

static int __init init_modules(void) {
    
	dev_t dev;

	printk("%s:%s():...............Start...............\n", PREFIX_TITLE, __func__);
  	
	dev_cdev = cdev_alloc();

	// Register chrdev
	if(alloc_chrdev_region(&dev, DEV_BASEMINOR, DEV_COUNT, DEV_NAME) < 0) {
		printk(KERN_ALERT"Register chrdev failed!\n");
		return -1;
    	} else {
		printk("%s:%s(): register chrdev(%i,%i)\n", PREFIX_TITLE, __func__, MAJOR(dev), MINOR(dev));
    	}

    	dev_major = MAJOR(dev);
    	dev_minor = MINOR(dev);

	// Init cdev
   	dev_cdev->ops = &fops;
    dev_cdev->owner = THIS_MODULE;

	if(cdev_add(dev_cdev, dev, 1) < 0) {
		printk(KERN_ALERT"Add cdev failed!\n");
		return -1;
   	}
	int irq_return;
	irqq_count = 0;
	typedef irqreturn_t (*irq_handler_t)(int, void *);
	irq_return = request_irq(IRQ_NUM,   /* The number of the keyboard IRQ on PCs */ 
              (irq_handler_t)irq_handler, /* our handler */
              IRQF_SHARED, 
              "test_keyboard_irq_handler", &dev_unique);
	printk("%s:%s(): request_irq %d returns %d\n",IRQ_NUM,irq_return, PREFIX_TITLE, __func__);

	// Alloc DMA buffer
	printk("%s:%s(): allocate dma buffer\n", PREFIX_TITLE, __func__);
	dma_buf = kmalloc(DMA_BUFSIZE,GFP_KERNEL);

	// Alloc work routine
    work = kmalloc(sizeof(typeof(*work)), GFP_KERNEL);

	return 0;
}

static void __exit exit_modules(void) {

	unregister_chrdev_region(MKDEV(dev_major,dev_minor), DEV_COUNT);
	cdev_del(dev_cdev);
	// Free work routine and dma_buf
	printk("%s:%s(): interrupt count = %d\n", PREFIX_TITLE, __func__,irqq_count);
	free_irq(IRQ_NUM,&dev_unique);
	kfree(work);
	printk("%s:%s(): free dma buffer\n", PREFIX_TITLE, __func__);
	kfree(dma_buf);
	printk("%s:%s(): unregister chrdev\n", PREFIX_TITLE, __func__);
	printk("%s:%s():..............End..............\n", PREFIX_TITLE, __func__);

}



module_init(init_modules);
module_exit(exit_modules);
