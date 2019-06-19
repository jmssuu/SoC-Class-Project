//=========================定義include檔及define檔=======================
#define IP_MAJOR		0     // 0: dynamic major number
#define IP_MINOR		0     // 0: dynamic minor number
#define IP_BASEADDRESS 0x7A200000
#define SIZE_OF_DEVICE 0x4000*4



#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>                     //define file結構
#include <linux/types.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>                  //copy from/to user
#include <linux/ioctl.h>
#include <asm/io.h>

#define TX_IRQ_NUMBER 90
#define RX_IRQ_NUMBER 91



//===============================定義裝置編號=============================
static int IP_major = IP_MAJOR;  
static int IP_minor = IP_MINOR;  
module_param(IP_major, int, 0644);
module_param(IP_minor, int, 0644);

//=======================================================================

//===============================define device structure=============================
static struct IP_Driver {
    dev_t IP_devt;
    struct class *class;
    struct class_device *class_dev;
    struct cdev *cdev;
} IP;
//=======================================================================
volatile unsigned long *IP_wBASEADDRESS;

int value=0;

//=============================file operation=========================
static int IP_open(struct inode *inode, struct file *file){
    return nonseekable_open(inode, file);
}

static int IP_release(struct inode *inode, struct file *file){

    return 0;
}

static int IP_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    value = ioread32(IP_wBASEADDRESS+4);
    copy_to_user(buf, &value,size);
    return 0;
}

static int IP_write(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    copy_from_user(&value, buf, size);
    iowrite32(value, IP_wBASEADDRESS);
    return 0;
}

static long IP_ioctl(struct file *file, unsigned int cmnd, unsigned long arg){
    return 0;
}

static struct file_operations IP_fops = {
    .owner          = THIS_MODULE,
    .open           = IP_open,
    .release        = IP_release,
    .read           = IP_read,
    .write          = IP_write,
    .unlocked_ioctl = IP_ioctl
};



//==========================RX INIT HANDLER===========================
static irqreturn_t Rx_handler(int irq, void *dev_id){
    unsigned int Rx_buffer = 0;
    printk("Rx IRQ!!!!!\n");
    Rx_buffer = ioread32(IP_wBASEADDRESS+4);
    printk("Rx Buffer = %c \n",Rx_buffer);
    return IRQ_HANDLED;
}
//=======================================================================

//==========================cdev init()     =============================
static int IP_setup_cdev(struct IP_Driver *IP_p){

    int ret, err;

    IP_p->IP_devt = MKDEV(IP_major,IP_minor);

    if(IP_major){
        ret = register_chrdev_region(IP_p->IP_devt, 1, "IP-Driver");
    }else{
        ret = alloc_chrdev_region(&IP_p->IP_devt, IP_minor, 1, "IP-Driver");
        IP_major = MAJOR(IP_p->IP_devt);
        IP_minor = MINOR(IP_p->IP_devt);
    }
    if(ret <0)
        return ret;

    IP_p->class = class_create(THIS_MODULE, "IP-Driver");
    if(IS_ERR(IP_p->class)){
        printk("IP_setup_cdev: Can't create IP Driver class!\n");
        ret = PTR_ERR(IP_p->class);
        goto error1;
    }
    
    IP_p->cdev = cdev_alloc();
    if(NULL == IP_p->cdev){
        printk("IP_setup_cdev: Can't alloc IP Driver cdev!\n");
        ret = -ENOMEM;
        goto error2;
    }
    
    IP_p->cdev->owner = THIS_MODULE;
    IP_p->cdev->ops = &IP_fops;

    err = cdev_add(IP_p->cdev, IP_p->IP_devt, 1);
    if(err){
        printk("IP_setup_cdev: Can't add IP cdev to system!\n");
        ret = -EAGAIN;
        goto error2;
    }

    IP_p->class_dev = device_create(IP_p->class, NULL, IP_p->IP_devt, NULL, "IP-Driver");
    if(IS_ERR(IP_p->class_dev)){
        printk("IP_setup_cdev: Can't create IP class_dev to system!\n");
        ret = PTR_ERR(IP_p->class_dev);
        goto error3;
    }
    printk("IP-Driver_class_dev info: IP-Driver (%d:%d)\n",MAJOR(IP_p->IP_devt), MINOR(IP_p->IP_devt));
    
    //================request mem region======================
    if(!request_mem_region(IP_BASEADDRESS, SIZE_OF_DEVICE*4,"IP"))
    {
        printk("err:Request_mem_region\n");
        return -ENODEV;
    }
    //===========ioremap_nocache===========
    IP_wBASEADDRESS = (unsigned long *)ioremap_nocache(IP_BASEADDRESS, SIZE_OF_DEVICE*4);

    printk("IP > IP_ioport_write    : %p\n", IP_wBASEADDRESS);

    return 0;

    error3:
        cdev_del(IP_p->cdev);
    error2:
        class_destroy(IP_p->class);
    error1:
        unregister_chrdev_region(IP_p->IP_devt, 1);
        return ret;
}
//=======================================================================
static void IP_remove_cdev(struct IP_Driver *IP_p){

    device_unregister(IP_p->class_dev);
    cdev_del(IP_p->cdev);
    class_destroy(IP_p->class);
    unregister_chrdev_region(IP_p->IP_devt, 1);
    //==============ioremap_nocache==================
    iounmap((void *)IP_wBASEADDRESS);

    release_mem_region(IP_BASEADDRESS, SIZE_OF_DEVICE*4);

}
//=======================================================================

static int IP_init(void){
    int ret=0;
    
    printk(KERN_ALERT "hello XDC. \n");
    
    IP_setup_cdev(&IP);
    
    ret = request_irq(91,Rx_handler,IRQF_SHARED,"IP-Rx_ISR",THIS_MODULE);
    if(ret < 0)
        pr_err("%s\n", "request_irq failed");
    printk("Rx miniuart !!!!!!\n\n");

    iowrite32(0x00,IP_wBASEADDRESS);
    
    return ret;
}

static void IP_exit(void){
    printk(KERN_ALERT "Close Module. \n");
    IP_remove_cdev(&IP);

    free_irq(91,THIS_MODULE);
}

module_init(IP_init);
module_exit(IP_exit);

MODULE_DESCRIPTION("IP Driver");
MODULE_LICENSE("GPL");
