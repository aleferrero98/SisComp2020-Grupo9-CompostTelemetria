#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_AUTHOR("Grupo 9");
MODULE_DESCRIPTION("Character Device Driver para pines GPIO de Raspberry PI");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev *cdev_array = NULL;
static struct class *class_raspin = NULL;
static volatile uint32_t *gpio_base = NULL;

#define RPI_GPF_INPUT       0x00
#define RPI_GPF_OUTPUT      0x01

#define RPI_GPFSEL0_INDEX   0
#define RPI_GPFSEL1_INDEX   1
#define RPI_GPFSEL2_INDEX   2
#define RPI_GPFSEL3_INDEX   3

#define RPI_GPSET0_INDEX    7
#define RPI_GPCLR0_INDEX    10

#define RPI_REG_BASE        0x3f000000
#define RPI_GPIO_OFFSET     0x200000
#define RPI_GPIO_BASE       (RPI_REG_BASE + RPI_GPIO_OFFSET)
#define RPI_GPIO_SIZE       0xC0

// TODO: reemplazar por la cantidad de pines GPIO que hayan
#define NUM_DEV             1

#define DEVICE_NAME         "raspin"

static int gpio_map(void)
{
    if (gpio_base = NULL)
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

static struct file_operations raspin_fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read,
};

static int register_dev(void) {
  int retval;
  dev_t devno;
  int i;

  retval = alloc_chrdev_region(&dev, 0, NUM_DEV, DEVICE_NAME);

  if (retval < 0) {
    printk(KERN_ERR "alloc_chrdev_region falló.\n");
    return retval;
  }

  class_raspin = class_create(THIS_MODULE, DEVNAME_LED);
  if (IS_ERR(class_raspin)) 
  {
    return PTR_ERR(class_raspin);
  }

  for (i = 0; i < NUM_DEV; i++) 
  {
    devno = MKDEV(MAJOR(dev), i);

    cdev_init(&(cdev_array[cdev_index]), &raspin_fops);
    cdev_array[cdev_index].owner = THIS_MODULE;
    if (cdev_add(&(cdev_array[cdev_index]), devno, 1) < 0) 
    {
      printk(KERN_ERR "fallo en cdev_add minor = %d\n", i);
    } 
    else 
    {
      device_create(class_raspin, NULL, devno, NULL, DEVICE_NAME "%u", i);
    }
    cdev_index++;
  }
  return 0;
}

static int __init init_mod(void)
{
    int retval;
    size_t size;

    printk(KERN_INFO "Cargando devices...\n");
    if (gpio_map() != 0) 
    {
        printk(KERN_ALERT "No se pudieron mapear pines.\n");
        return -EBUSY;
    }

    /* Iniciar todos los pines como salidas en bajo */
    uint32_t i;
    for (i = 0; i < 40; i++)
    {
        // TODO: obviar los pines que no son GPIO
        rpi_gpio_function_set(i, RPI_GPF_OUTPUT);
        rpi_gpio_set32(RPI_GPIO_P2MASK, 1 << i);
    }

    size = size_of(struct cdev) * NUM_DEV_TOTAL;
    cdev_array = (struct cdev *) kmalloc(size, GFP_KERNEL);

    if ((retval = register_dev()) != 0)
    {
        printk(KERN_ALERT "Falló el registro!\n");
        return retval;
    }
    
    return 0;
}
