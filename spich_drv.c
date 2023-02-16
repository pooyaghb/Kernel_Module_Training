#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define DEVICE_NAME "spich_dev"
#define CLASS_NAME  "spich_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("POOYA");
MODULE_DESCRIPTION("A Simple SPI Device Driver with Char file ops");

static struct class          *spich_class;
static struct cdev            spich_dev;
static        dev_t           spich_num;

static int     spich_open    (struct inode *inode_str, 
		struct file *fie_str);
static int     spich_release (struct inode *inode_str, 
		struct file *file_str);
static ssize_t spich_read    (struct file *file_str, 
		char __user *user_buf, 
		size_t size, 
		loff_t *offset);
static ssize_t spich_write   (struct file *file_str, 
		const char __user *user_buf, 
		size_t size, 
		loff_t *offset);

static const struct file_operations spich_ops = {
	.owner   = THIS_MODULE,
	.open    = spich_open,
	.release = spich_release,
	.read    = spich_read,  
	.write   = spich_write,   
};

static struct spi_device *spi_dev0;

static u32 tx_buf = 0x0011;
static u32 rx_buf = 0xFF00;

static int spi_probe (struct spi_device *spidev);
static int spi_remove(struct spi_device *spidev);

static struct of_device_id of_device_ids[] = {
	{
		.compatible = "pizza",
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_device_ids);

static struct spi_device_id spi_device_ids[] = {
	{"spi_dev", 0}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(spi, spi_device_ids);

static struct spi_driver spi_drv = {
	.driver   = {
		.name = "spi_drv",
		.of_match_table = of_device_ids,
	},
	.probe    = spi_probe,
	.remove   = spi_remove,
	.id_table = spi_device_ids,
};

static int spi_probe (struct spi_device *spidev)
{

	printk("spi_probe\n");

	spidev->max_speed_hz = 2000000;
	spidev->mode = SPI_MODE_3;
	spidev->bits_per_word = 8;

	if(spi_setup(spidev))
	{
		printk("Can't setup spi\n");
		return -1;
	}

	spi_dev0 = spidev;
	
	return 0;
}


static int spi_remove(struct spi_device *spidev)
{
	printk("spi_remove\n");
	return 0;
}

static int __init spi_init(void)
{
	
	if(alloc_chrdev_region(&spich_num, 0, 1, DEVICE_NAME))
	{
		printk("Can't alloc chrdev region\n");
		return -1;
	}

	if((spich_class = class_create(THIS_MODULE, CLASS_NAME)) == NULL)
	{
		printk("Can't create class\n");
		unregister_chrdev_region(spich_num, 1);
		return -1;
	}

	cdev_init(&spich_dev, &spich_ops);
	spich_dev.owner = THIS_MODULE;
	if(cdev_add(&spich_dev, spich_num, 1))
	{
		printk("Can't add device\n");
		class_destroy(spich_class);
		unregister_chrdev_region(spich_num, 1);
		return -1;
	}

	if(device_create(spich_class, NULL, spich_num, NULL, DEVICE_NAME) == NULL)
	{
		printk("Can't create device\n");
		cdev_del(&spich_dev);
		class_destroy(spich_class);
		unregister_chrdev_region(spich_num, 1);
		return -1;
	}
	
	return 0;
}

static void __exit spi_exit(void)
{
	
	device_destroy(spich_class, spich_num);
	class_destroy(spich_class);
	cdev_del(&spich_dev);
	unregister_chrdev_region(spich_num, 1);
}

module_init(spi_init);
module_exit(spi_exit);


static int spich_open    (struct inode *inode_str, 
		struct file *file_str)
{
	printk("spich_open\n");

	if(spi_register_driver(&spi_drv))
	{
		printk("Can't register spi_dev\n");
		return -1;
	}

	return 0;
}

static int spich_release (struct inode *inode_str, 
		struct file *file_str)
{
	printk("spich_release\n");
	spi_unregister_driver(&spi_drv);
	return 0;
}

static ssize_t spich_read    (struct file *file_str, 
		char __user *user_buf, 
		size_t size, 
		loff_t *offset)
{
	unsigned long not_copied, copied;
	ssize_t dif_copied;

	printk("spich_read\n");

	copied = min(size, sizeof(rx_buf));
	not_copied = copy_to_user(user_buf, &rx_buf, copied);
	dif_copied = copied - not_copied;
	return dif_copied;
}
static ssize_t spich_write   (struct file *file_str, 
		const char __user *user_buf, 
		size_t size, 
		loff_t *offset)
{
	unsigned long not_copied, copied;
	ssize_t dif_copied;

	struct spi_transfer spi_tr = 
	{
		.tx_buf = &tx_buf, 
		.rx_buf = &rx_buf, 
		.len = sizeof(tx_buf),
	};
	
	struct spi_message spi_msg;
	memset(&spi_msg, 0, sizeof(spi_msg));
	
	copied = min(size, sizeof(tx_buf));
	not_copied = copy_from_user(&tx_buf, user_buf, copied);
	dif_copied = copied - not_copied;

	spi_message_init(&spi_msg);
	spi_message_add_tail(&spi_tr, &spi_msg);
	spi_sync(spi_dev0, &spi_msg);
	printk("%04x\n", rx_buf);

	return dif_copied;
}







