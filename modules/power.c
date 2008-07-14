/* -*-linux-c-*- */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#include <asm/arch/gpio.h>

#include "sios/sios.h"
#include "sios/resource.h"
#include "sios/hardware.h"

#define POWER_OFF_ON_RELEASE 0

static struct sios_resource power_res[] = {
	[0] = {
		.start = GPIO_SIOS_HS,
		.end = GPIO_SIOS_USBHP,
		.type = SIOS_IO_GPIO,
	},
};

static int to_bool(const char *buf, size_t count)
{
	int on = 1;

	if (count < 1)
		return 0;

	switch(buf[0]) {
	case 'f': 
	case 'F': 
	case 'n': 
	case 'N':
	case '0':
		on = 0;
	case 'o': 
	case 'O':
		if (count >= 2) {
			switch(buf[1]) {
			case 'f':
			case 'F':
				on = 0;
			}
		}
	}

	return on;
}

static ssize_t show_sios_hs(struct device *dev, struct device_attribute *attr,
			      char * buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", 
			gpio_get_value(GPIO_SIOS_HS) ? "on" : "off");
}

static ssize_t store_sios_hs(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	gpio_set_value(GPIO_SIOS_HS, to_bool(buf, count));
	return count;
}
DEVICE_ATTR(HotSwap, S_IRUGO | S_IWUSR, show_sios_hs, store_sios_hs);

static ssize_t show_sios_vdd2(struct device *dev, struct device_attribute *attr,
			      char * buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", 
			gpio_get_value(GPIO_SIOS_VDD2) ? "on" : "off");
}

static ssize_t store_sios_vdd2(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	gpio_set_value(GPIO_SIOS_VDD2, to_bool(buf, count));
	return count;
}
DEVICE_ATTR(VDD2, S_IRUGO | S_IWUSR, show_sios_vdd2, store_sios_vdd2);

static ssize_t show_sios_usbhp(struct device *dev, struct device_attribute *attr,
			      char * buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", 
			gpio_get_value(GPIO_SIOS_USBHP) ? "on" : "off");
}

static ssize_t store_sios_usbhp(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	gpio_set_value(GPIO_SIOS_USBHP, to_bool(buf, count));
	return count;
}
DEVICE_ATTR(USBHP, S_IRUGO | S_IWUSR, show_sios_usbhp, store_sios_usbhp);

static ssize_t show_sios_pwr(struct device *dev, struct device_attribute *attr,
			      char * buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", 
			gpio_get_value(GPIO_SIOS_PWR) ? "on" : "off");
}

static ssize_t store_sios_pwr(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	gpio_set_value(GPIO_SIOS_PWR, to_bool(buf, count));
	return count;
}
DEVICE_ATTR(PWR, S_IRUGO | S_IWUSR, show_sios_pwr, store_sios_pwr);

void sios_power_off(void)
{
	sios_peripheral_off();
	sios_hotswap_off();
	sios_usb_hipower_off();

	/* Shut down ! */
	gpio_set_value(GPIO_SIOS_PWR, 0);
}
EXPORT_SYMBOL_GPL(sios_power_off);

static int sios_power_up(struct sios_device *sdev)
{
	gpio_direction_output(GPIO_SIOS_PWR, 1);
	gpio_direction_output(GPIO_SIOS_HS, 0);
	gpio_direction_output(GPIO_SIOS_VDD2, 0);
	gpio_direction_output(GPIO_SIOS_USBHP, 0);

	/* FIXME: where to go? */
	gpio_direction_output(GPIO_SIOS_RST, 0);

	sios_peripheral_on();
	udelay(5);

	return 0;
}

static void sios_power_release(struct sios_device *sdev)
{
#if POWER_OFF_ON_RELEASE
	sios_hotswap_off();
	sios_usb_hipower_off();
	sios_peripheral_off();
#endif
}

static int sios_power_probe(struct sios_device *sdev)
{
	/* If we could we could check here if we really are a SIOS platform
	 * before powering up */
	printk(KERN_INFO "power probe, dev='%s'\n", sdev->name);
	return sios_power_up(sdev);
}

struct sios_device power_dev = {
	.name = "sios:power",
	.release = sios_power_release,
	.num_resource = 1,
	.resource = power_res,
};

struct sios_driver power_drv = {
	.version = "$Revision: 1.0 $",
	.module = THIS_MODULE,
	.driver = {
		.name = "sios:power",
	},
//	.probe = sios_power_probe,
};

static int __init sios_power_init(void)
{
	int error;

       	error = sios_driver_register(&power_drv);
	if (error)
		return error;

	error = sios_device_register(&power_dev);
	if (error) {
		sios_driver_unregister(&power_drv);
		return error;
	}
		
	error = sios_power_up(&power_dev);
	if (error)
		goto err;

	if ((error = device_create_file(&power_dev.dev, &dev_attr_HotSwap)))
		goto err;
	if ((error = device_create_file(&power_dev.dev, &dev_attr_VDD2)))
		goto err_1;
	if ((error = device_create_file(&power_dev.dev, &dev_attr_USBHP)))
		goto err_2;
	if ((error = device_create_file(&power_dev.dev, &dev_attr_PWR)))
		goto err_3;

	return error;

err_3:
	device_remove_file(&power_dev.dev, &dev_attr_USBHP);
err_2:
	device_remove_file(&power_dev.dev, &dev_attr_VDD2);
err_1:
	device_remove_file(&power_dev.dev, &dev_attr_HotSwap);
err:
	sios_device_unregister(&power_dev);
	sios_driver_unregister(&power_drv);

	return error;
}

static void __exit sios_power_exit(void)
{
	device_remove_file(&power_dev.dev, &dev_attr_USBHP);
	device_remove_file(&power_dev.dev, &dev_attr_VDD2);
	device_remove_file(&power_dev.dev, &dev_attr_PWR);
	device_remove_file(&power_dev.dev, &dev_attr_HotSwap);

	sios_device_unregister(&power_dev);
	sios_driver_unregister(&power_drv);
}


MODULE_DESCRIPTION("SIOS power driver");
MODULE_AUTHOR("Simon de Bakker <simon@v2.nl>");
MODULE_LICENSE("GPL");

module_init(sios_power_init);
module_exit(sios_power_exit);
