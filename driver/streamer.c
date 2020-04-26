#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define DEVICE_NAME "myStreamer"
#define CLASS_NAME  "myStreamer"

#define SUCCESS 0

#define DEVICE_BUFFER_SIZE 64

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

static char* device_buffer = NULL;
static int device_buffer_index = 0;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct task_struct* device_sim_struct;
static struct completion read_buffered_data;

static int device_sim_thread(void* data) {
    unsigned long timeout;
    (void) data; // Unused.

    printk(KERN_INFO "%s: Device simulation thread has been started.\n", DEVICE_NAME);

    while (!kthread_should_stop()) {
        // wait for the buffer to be emptied
        timeout = wait_for_completion_timeout(&read_buffered_data, msecs_to_jiffies(500));

	// If there was a timeout on the wait, just go back to waiting.
	if (timeout == 0) {
            continue;
	}

        printk(KERN_INFO "%s: Generated random data to simulate data being read in from a device.\n", DEVICE_NAME);

        // Fill that with random data.
        get_random_bytes(device_buffer, DEVICE_BUFFER_SIZE);
    }

    printk(KERN_INFO "%s: Device simulation thread has been stopped.\n", DEVICE_NAME);

    return SUCCESS;
}

int init_module(void) {
    printk(KERN_INFO "%s: Module init start.\n", DEVICE_NAME);

    // Register the char device
    dev_major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (dev_major_number < 0) {
        printk(KERN_ALERT "%s: Failed to create character device. %d\n", DEVICE_NAME, dev_major_number);
        return dev_major_number;
    }

    // Create the device class
    dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(dev_class)) {
        unregister_chrdev(dev_major_number, DEVICE_NAME);
        printk(KERN_ALERT "%s: Failed to create device class.\n", DEVICE_NAME);
        return PTR_ERR(dev_class);
    }

    // Create the device itself
    dev_device = device_create(dev_class, NULL, MKDEV(dev_major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(dev_device)) {
        class_destroy(dev_class);
        unregister_chrdev(dev_major_number, DEVICE_NAME);
        printk(KERN_ALERT "%s: Failed to create device.\n", DEVICE_NAME);
        return PTR_ERR(dev_device);
    }

    init_completion(&read_buffered_data);

    device_buffer = (char*)kmalloc(DEVICE_BUFFER_SIZE, GFP_KERNEL);
    if (device_buffer == NULL) {
        printk(KERN_ALERT "%s: Failed to create device buffer.\n", DEVICE_NAME);
        return PTR_ERR(device_buffer);
    }

    device_sim_struct = kthread_run(device_sim_thread, NULL, "dev_sim_thread");

    // Something bad happened when making the thread...
    if (IS_ERR(device_sim_struct)) {
        printk(KERN_ALERT "%s: Failed to create device simulation thread from open call.\n", DEVICE_NAME);

        // Capture the error.
        return PTR_ERR(device_sim_struct);
    }

    printk(KERN_INFO "%s: Module init complete.\n", DEVICE_NAME);

    return SUCCESS;
}

void cleanup_module(void) {
    printk(KERN_INFO "%s: Module exit.\n", DEVICE_NAME);

    // Stop the thread
    kthread_stop(device_sim_struct);

    kfree(device_buffer);

    // Break everything down on exit
    device_destroy(dev_class, MKDEV(dev_major_number, 0));
    class_unregister(dev_class);
    class_destroy(dev_class);
    unregister_chrdev(dev_major_number, DEVICE_NAME);
}


static int device_open(struct inode* inode, struct file* fd) {
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

static int device_release(struct inode* inode, struct file* fd) {
    module_put(THIS_MODULE);

    return SUCCESS;
}

static ssize_t device_read(struct file* fd, char* buffer, size_t buff_size, loff_t* offset) {
    ssize_t copied_data;
    long copy_status;

    printk(KERN_INFO "%s: App is reading data.\n", DEVICE_NAME);
    printk(KERN_INFO "%s: device_buffer_index: %d.\n", DEVICE_NAME, device_buffer_index);
    printk(KERN_INFO "%s: buff_size: %ld.\n", DEVICE_NAME, buff_size);

    copied_data = (buff_size > (DEVICE_BUFFER_SIZE - device_buffer_index) ? (DEVICE_BUFFER_SIZE - device_buffer_index) : buff_size );

    printk(KERN_INFO "%s: Copying %zd bytes to userspace.\n", DEVICE_NAME, copied_data);

    copy_status = copy_to_user(buffer, &device_buffer[device_buffer_index], copied_data);

    if (copy_status != 0) {
        printk(KERN_ALERT "%s: Failed to copy data to userspace (%ld).\n", DEVICE_NAME, copy_status);
        return -EFAULT;
    }

    device_buffer_index += copied_data;

    // Report back saying that the buffered data has been read by someone.
    if (device_buffer_index >= DEVICE_BUFFER_SIZE) {
        device_buffer_index = 0;
        complete(&read_buffered_data);
    }

    return copied_data;
}

static ssize_t device_write(struct file* fd, const char* buffer, size_t buff_size, loff_t* offset) {
    return -EINVAL; // no writing...
}
