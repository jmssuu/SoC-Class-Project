//=========================定義include檔及define檔=======================
#define SOCLE_FPGA_MAJOR		0     // 0: dynamic major number
#define SOCLE_FPGA_MINOR		0     //次編號

#include<linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>                     //定義file結構
#include <linux/types.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>                  //copy from/to user


//=======================================================================

//============================定義錯誤訊息顯示格式========================
//#define SOCLE_FPGA_DEBUG
#ifdef SOCLE_FPGA_DEBUG
#define FPGA_DBG(fmt, args...) printk(KERN_DEBUG "SOCLE_FPGA: " fmt, ## args)
#else
#define FPGA_DBG(fmt, args...)
#endif
//=======================================================================

//===============================定義裝置編號=============================
static int socle_FPGA_major = SOCLE_FPGA_MAJOR;  //主編號
static int socle_FPGA_minor = SOCLE_FPGA_MINOR;  //次編號
module_param(socle_FPGA_major, int, 644);
module_param(socle_FPGA_minor, int, 644);
//=======================================================================

//==========================定義各裝置的結構型別==========================
static struct socle_FPGA_Driver {
	int busy;
	dev_t FPGA_devt;
	struct class *class;
	struct class_device *class_dev;
	struct cdev *cdev;
} socle_FPGA;
//=======================================================================

static ssize_t socle_FPGA_read    (struct file *fd, char __user *buf, size_t size, loff_t *f_pos);
static ssize_t socle_FPGA_write   (struct file *fd, const char __user *buf, size_t size, loff_t *f_pos);
static int     socle_FPGA_ioctl   (struct inode *inodep, struct file *filep, unsigned int cmd, unsigned long arg);
static int     socle_FPGA_open    (struct inode *inodep, struct file *filep);
static int     socle_FPGA_release (struct inode *inodep, struct file *filep);
u32 val;
int wcounts;
int rcounts;
volatile unsigned long  *FPGA_wSTART;

//===========================驅動程式初始化===============================
static struct file_operations socle_FPGA_fops = {
   	  .owner = THIS_MODULE,             //定義在<linux/module.h>
	   .read = socle_FPGA_read,         //讀取檔案
	  .write = socle_FPGA_write,
	  .ioctl = socle_FPGA_ioctl,        //裝置專屬指令
	   .open = socle_FPGA_open,         //開檔案
	.release = socle_FPGA_release       //釋放裝置檔
};
//=======================================================================

//=========================Application讀取資料=========================
static int socle_FPGA_read(struct file *fd, char __user *buf, size_t size, loff_t *f_pos)
{
       copy_to_user(buf, &val,size);
	printk("FPGA>Sent data:%d\n",val);
    	return 0;
}
//=======================================================================

//=========================Application存放資料=========================
static int socle_FPGA_write(struct file *fd, const char __user *buf, size_t size, loff_t *f_pos)
{
        copy_from_user(&val, buf, size);
	printk("FPGA>Get data:%d",val);
        return 0;
}
//=======================================================================


//============================定義專屬指令===============================
static int socle_FPGA_ioctl(struct inode *inodep, struct file *filep, unsigned int cmd, unsigned long arg)
{
	
	return 0;
}
//=======================================================================

//===================為後續作業進行必要初始化準備==========================
//ex:檢查裝置特有的錯誤、目標裝置初始化、更新f_op指標等等
static int socle_FPGA_open(struct inode *inodep, struct file *filep)
{

	FPGA_DBG("socle_FPGA_open\n");

	return 0;
}
//=======================================================================


//================釋放裝置檔，當file將被釋放前會被觸發====================
//ex:釋放open儲存於filp->private_data的東西
static int socle_FPGA_release(struct inode *inodep, struct file *filep)
{

	FPGA_DBG("socle_FPGA_release\n");
	return 0;
}
//=======================================================================


//=====================透過cdev_init()初始化此結構=======================
static int socle_FPGA_setup_cdev(struct socle_FPGA_Driver *p_FPGA)
{
	int ret, err;

	p_FPGA->FPGA_devt = MKDEV(socle_FPGA_major, socle_FPGA_minor);
	
	if (socle_FPGA_major) {
	ret = register_chrdev_region(p_FPGA->FPGA_devt, 1, "FPGA-Driver");
	} else {
ret = alloc_chrdev_region(&p_FPGA->FPGA_devt, socle_FPGA_minor, 1, "FPGA-Driver");
		socle_FPGA_major = MAJOR(p_FPGA->FPGA_devt);
		socle_FPGA_minor = MINOR(p_FPGA->FPGA_devt);
	}
	if (ret < 0)
		return ret;

	p_FPGA->class = class_create(THIS_MODULE, "socle-FPGA-Driver");
	if (IS_ERR(p_FPGA->class)) {
       printk("socle_FPGA_setup_cdev: Can't create FPGA Driver class!\n");
		ret = PTR_ERR(p_FPGA->class);
		goto error1;
	}

	p_FPGA->cdev = cdev_alloc();		
	if (NULL == p_FPGA->cdev) {
		printk("socle_FPGA_setup_cdev: Can't alloc FPGA Driver cdev!\n");
		ret = -ENOMEM;
		goto error2;
	}

	p_FPGA->cdev->owner = THIS_MODULE;
	p_FPGA->cdev->ops = &socle_FPGA_fops;

	err = cdev_add(p_FPGA->cdev, p_FPGA->FPGA_devt, 1);
	if (err) {
		printk("socle_FPGA_setup_cdev: Can't add FPGA cdev to system!\n");
		ret = -EAGAIN;
		goto error2;
	}

p_FPGA->class_dev = class_device_create(p_FPGA->class, NULL, p_FPGA->FPGA_devt, NULL, "FPGA-Driver");
	if (IS_ERR(p_FPGA->class_dev)) {
	printk("socle_FPGA_setup_cdev: Can't create FPGA class_dev to system!\n");
		ret = PTR_ERR(p_FPGA->class_dev);
		goto error3;
	}
printk("FPGA-Driver_class_dev info: FPGA-Driver (%d:%d)\n", MAJOR(p_FPGA->FPGA_devt), MINOR(p_FPGA->FPGA_devt));

	return 0;

error3:
	cdev_del(p_FPGA->cdev);
error2:
	class_destroy(p_FPGA->class);
error1:
	unregister_chrdev_region(p_FPGA->FPGA_devt, 1);
	return ret;
}
//=======================================================================

//============================載入核心時執行==============================
static int __init socle_FPGA_init(void)
{
	int ret = 0;

	socle_FPGA_setup_cdev(&socle_FPGA); //_init()初始化socle_FPGA_setup_cdev

	return ret;
}
//=======================================================================

//===========================向系統註消註冊===============================
static void socle_FPGA_remove_cdev(struct socle_FPGA_Driver *p_FPGA)
{
	class_device_unregister(p_FPGA->class_dev);
	cdev_del(p_FPGA->cdev);
	class_destroy(p_FPGA->class);
	unregister_chrdev_region(p_FPGA->FPGA_devt, 1);
}
//=======================================================================

//=========================結束驅動程式工作===============================
static void __exit socle_FPGA_cleanup(void)
{
	socle_FPGA_remove_cdev(&socle_FPGA);

}
//=======================================================================


module_init(socle_FPGA_init);
module_exit(socle_FPGA_cleanup);

MODULE_DESCRIPTION("Socle FPGA Driver");
MODULE_LICENSE("GPL");

