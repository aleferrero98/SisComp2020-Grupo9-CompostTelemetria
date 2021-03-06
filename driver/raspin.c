#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>

MODULE_AUTHOR("Grupo 9");
MODULE_DESCRIPTION("Character Device Driver para pines GPIO de Raspberry PI");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev *cdev_array = NULL;
static struct class *class_raspin = NULL;
static volatile uint32_t *gpio_base = NULL;

static volatile int cdev_index = 0;
static volatile int open_counter = 0;

#define MAX_BUFLEN 64
unsigned char dev_buf[MAX_BUFLEN];

#define RPI_GPF_INPUT       0x00
#define RPI_GPF_OUTPUT      0x01

#define RPI_GPFSEL0_INDEX   0
#define RPI_GPFSEL1_INDEX   1
#define RPI_GPFSEL2_INDEX   2
#define RPI_GPFSEL3_INDEX   3

#define RPI_GPSET0_INDEX    7
#define RPI_GPCLR0_INDEX    10

#define RPI_GPIO_P2MASK     (uint32_t)0xffffffff

#define RPI_REG_BASE        0x3f000000
#define RPI_GPIO_OFFSET     0x200000
#define RPI_GPIO_BASE       (RPI_REG_BASE + RPI_GPIO_OFFSET)
#define RPI_GPIO_SIZE       0xC0

#define NUM_DEV             27

#define DEVICE_NAME         "raspin"

static int low = 0;
static int high = 1;

static int gpio_map(void)
{
    if (gpio_base == NULL)
        gpio_base = ioremap_nocache(RPI_GPIO_BASE, RPI_GPIO_SIZE);
    
    return 0;
}

static int rpi_gpio_function_set(int pin, uint32_t func)
{
    int index = RPI_GPFSEL0_INDEX + pin / 10;
    uint32_t shift = (pin % 10) * 3;
    uint32_t mask = ~(0x07 << shift);
    gpio_base[index] = (gpio_base[index] & mask) | ((func & 0x07) << shift);
    return 1;
}

static void rpi_gpio_set32(uint32_t mask, uint32_t val)
{
    gpio_base[RPI_GPSET0_INDEX] = val & mask;
}

static ssize_t dev_write(struct file *filp, const char *buf, size_t count, loff_t *pos)
{
    char cval;
    unsigned int gpio;
    
    gpio = iminor(filp->f_path.dentry->d_inode);

    if (count > 0) 
    {
        if (copy_from_user(&cval, buf, sizeof(char)))
            return -EFAULT;

        switch (cval) 
        {
            case '1':
                //rpi_gpio_set32(RPI_GPIO_P2MASK, 1 << gpio);
                gpio_set_value(gpio, high);
                break;
            case '0':
                //rpi_gpio_clear32(RPI_GPIO_P2MASK, 1 << gpio);
                gpio_set_value(gpio, low);
                break;
            case 'e':
                rpi_gpio_function_set(gpio, RPI_GPF_INPUT);
                break;
            case 's':
                rpi_gpio_function_set(gpio, RPI_GPF_OUTPUT);
                break;
        }
        
        return sizeof(char);
    }
  
    return 0;
}

static ssize_t dev_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    unsigned int gpio;
    ssize_t retval;
    char byte;
    
    gpio = iminor(filp->f_path.dentry->d_inode);
    for (retval = 0; retval < count; ++retval)
    {
        byte = '0' + gpio_get_value(gpio);
        if (copy_to_user(buf + retval, &byte, sizeof(byte)))
        {
            printk(KERN_INFO "fallo en copy_to_user\n");
            return -EFAULT;
        }
    }
    
    return retval;
}

static int dev_open(struct inode *inode, struct file *filep) 
{
    int retval;
    int *minor = (int *)kmalloc(sizeof(int), GFP_KERNEL);
    int major = MAJOR(inode->i_rdev);
    *minor = MINOR(inode->i_rdev);

    printk(KERN_INFO "Petición de apertura major:%d minor: %d \n", major, *minor);

    filep->private_data = (void *)minor;

    retval = gpio_map();
    if (retval != 0) 
    {
        printk(KERN_ERR "No se pudo abrir GPIO.\n");
        return retval;
    }
  
    open_counter++;
    return 0;
}

static int dev_release(struct inode *inode, struct file *filep) 
{
    kfree(filep->private_data);
    open_counter--;
  
    if (open_counter <= 0) 
    {
        iounmap(gpio_base);
        gpio_base = NULL;
    }
  
    return 0;
}

static struct file_operations raspin_fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read,
};

static int register_dev(void) 
{
    int retval;
    dev_t devno;
    int i;

    retval = alloc_chrdev_region(&dev, 0, NUM_DEV, DEVICE_NAME);

    if (retval < 0) {
        printk(KERN_ERR "alloc_chrdev_region falló.\n");
        return retval;
    }

    class_raspin = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(class_raspin)) 
        return PTR_ERR(class_raspin);

    for (i = 0; i < NUM_DEV; i++) 
    {
        devno = MKDEV(MAJOR(dev), i);

        cdev_init(&(cdev_array[cdev_index]), &raspin_fops);
        cdev_array[cdev_index].owner = THIS_MODULE;
        if (cdev_add(&(cdev_array[cdev_index]), devno, 1) < 0) 
            printk(KERN_ERR "fallo en cdev_add minor = %d\n", i); 
        else
            device_create(class_raspin, NULL, devno, NULL, DEVICE_NAME "%u", i);
    
        cdev_index++;
    }
    return 0;
}

static int __init init_mod(void)
{
    int retval;
    size_t size;
    uint32_t i;

    printk(KERN_INFO "Cargando devices...\n");
    if (gpio_map() != 0) 
    {
        printk(KERN_ALERT "No se pudieron mapear pines.\n");
        return -EBUSY;
    }

    /* Iniciar todos los pines como salidas en bajo */
    for (i = 0; i < NUM_DEV; i++)
    {
        rpi_gpio_function_set(i, RPI_GPF_OUTPUT);
        rpi_gpio_set32(RPI_GPIO_P2MASK, 1 << i);
    }

    size = sizeof(struct cdev) * NUM_DEV;
    cdev_array = (struct cdev *) kmalloc(size, GFP_KERNEL);

    if ((retval = register_dev()) != 0)
    {
        printk(KERN_ALERT "Falló el registro!\n");
        return retval;
    }
    
    return 0;
}

static void __exit cleanup_mod(void) 
{
    int i;
    dev_t devno;
    dev_t devno_top;

    for (i = 0; i < NUM_DEV; i++)
        cdev_del(&(cdev_array[i]));

    devno_top = MKDEV(MAJOR(dev), 0);
    for (i = 0; i < NUM_DEV; i++)
    {
        devno = MKDEV(MAJOR(dev), i);
        device_destroy(class_raspin, devno);
    }
    unregister_chrdev_region(devno_top, NUM_DEV);

    class_destroy(class_raspin);

    kfree(cdev_array);
    printk("modulo siendo removido a %lu\n", jiffies);
}

module_init(init_mod);
module_exit(cleanup_mod);
