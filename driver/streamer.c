
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "myStreamer"
#define CLASS_NAME  "myStreamer"

#define SUCCESS 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ian Wakely");
MODULE_DESCRIPTION("Example module for streaming data from kernel space to userspace.");
MODULE_VERSION("1.0");

// Device file operations
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int dev_major_number;
static struct class* dev_class = NULL;
static struct device* dev_device = NULL;

static int open_instances = 0;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

int init_module(void) {
    printk(KERN_INFO "%s: Module init start.\n", DEVICE_NAME);

    // Register the char device
    dev_major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (dev_major_number < 0) {
        printk(KERN_ALERT "%s: Failed to create character device. %d\n", DEVICE_NAME, dev_major_number);
        return dev_major_number;
    }

    dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(dev_class)) {
        unregister_chrdev(dev_major_number, DEVICE_NAME);
        printk(KERN_ALERT "%s: Failed to create device class.\n", DEVICE_NAME);
        return PTR_ERR(dev_class);
    }

    dev_device = device_create(dev_class, NULL, MKDEV(dev_major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)) {
        class_destroy(dev_class);
        unregister_chrdev(dev_major_number, DEVICE_NAME);
        printk(KERN_ALERT "%s: Failed to create device.\n", DEVICE_NAME);
        return PTR_ERR(dev_device);
    }

    printk(KERN_INFO "%s: Module init complete.\n", DEVICE_NAME);

    return SUCCESS;
}

void cleanup_module(void) {
    printk(KERN_INFO "%s: Module exit.\n", DEVICE_NAME);

    device_destroy(dev_class, MKDEV(dev_major_number, 0));
    class_unregister(dev_class);
    class_destroy(dev_class);
    unregister_chrdev(dev_major_number, DEVICE_NAME);
}


static int device_open(struct inode* inode, struct file* fd) {
    try_module_get(THIS_MODULE);

    open_instances++;

    return SUCCESS;
}

static int device_release(struct inode* inode, struct file* fd) {
    module_put(THIS_MODULE);

    open_instances--;

    return SUCCESS;
}

static ssize_t device_read(struct file* fd, char* buffer, size_t buff_size, loff_t* offset) {
    return 0;
}

static ssize_t device_write(struct file* fd, const char* buffer, size_t buff_size, loff_t* offset) {
    return -EINVAL;
}
