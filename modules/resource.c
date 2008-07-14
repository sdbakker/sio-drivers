/* -*-linux-c-*- */

#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <linux/device.h>

#include "sios/resource.h"
#include "sios/hardware.h"
#include "sios/sios.h"

static LIST_HEAD(resource_list);
static DEFINE_RWLOCK(resource_lock);

static
ssize_t show_dev_resources(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct sios_resource_attribute *sattr = to_sios_res_attr(attr);
	struct sios_resource *res = sattr->res;
	char *flags, *type;

	if (res->type & SIOS_IO_GPIO)
		type = "GPIO";
	else if (res->type & SIOS_IO_XGPIO)
		type = "XGPIO";
	else
		type = "unknown";

	if (res->type & SIOS_IRQ)
		flags = "|IRQ";
	else if (res->type & SIOS_FIRQ)
		flags = "|FIRQ";
	else
		flags = "";

	return snprintf(buf, PAGE_SIZE, "%s\t%s%s\t%d\t%d\n", 
			res->name, type, flags,
			res->start, res->end);
}

int __sios_resource_create_file(struct sios_device *sdev, 
				struct sios_resource_attribute *sattr)
{
	struct device *dev = &sdev->dev;
	struct device_attribute *attr = &sattr->dev_attr;
	return device_create_file(dev, attr);
}
EXPORT_SYMBOL_GPL(__sios_resource_create_file);

int sios_resource_create_file(struct sios_device *sdev, struct sios_resource *res)
{
	struct device_attribute *dev_attr;
	struct device *dev = &sdev->dev;

	if (!sdev || !res)
		return -EINVAL;

	res->res_attr.res = res;
	dev_attr = &res->res_attr.dev_attr;
	if (dev->driver)
		dev_attr->attr.owner = dev->driver->owner;
	dev_attr->attr.name = res->name;
	dev_attr->attr.mode = S_IRUGO;
	dev_attr->show = show_dev_resources;
	dev_attr->store = NULL;
	res->res_attr.res = res;

	return __sios_resource_create_file(sdev, &res->res_attr);
}
EXPORT_SYMBOL_GPL(sios_resource_create_file);

void __sios_resource_remove_file(struct sios_device *sdev,
				 struct sios_resource_attribute *sattr)
{
	struct device *dev = &sdev->dev;
	struct device_attribute *attr = &sattr->dev_attr;
	device_remove_file(dev, attr);
}
EXPORT_SYMBOL_GPL(__sios_resource_remove_file);

void sios_resource_remove_file(struct sios_resource *res)
{
	struct sios_device *sdev = res->dev;
	struct sios_resource_attribute *attr = &res->res_attr;
	__sios_resource_remove_file(sdev, attr);
}
EXPORT_SYMBOL_GPL(sios_resource_remove_file);

static struct sios_resource *__sios_request_resource(struct sios_resource *res)
{
	struct sios_resource *pos = NULL;
	struct sios_resource *conflict = NULL;
	
	list_for_each_entry(pos, &resource_list, list) {
		if ((pos->type & ~(SIOS_IRQ|SIOS_FIRQ)) != 
		    (res->type & ~(SIOS_IRQ|SIOS_FIRQ)))
			continue;
		if (res->end < pos->start)
			continue;
		if (res->start > pos->end)
			continue;
		conflict = pos;
		break;
	}

	return conflict;
}

int sios_init_resource(struct sios_device *sdev, struct sios_resource *res)
{
	if (!res || !sdev)
		return -EINVAL;

	INIT_LIST_HEAD(&res->list);
	res->dev = sdev;
	return 0;
}
EXPORT_SYMBOL_GPL(sios_init_resource);

int sios_request_resource(struct sios_device *sdev, struct sios_resource *res)
{
	struct sios_resource *conflict;
	int error;

	error = sios_init_resource(sdev, res);
	if (error)
		return error; 

	if (res->type & (SIOS_IO_GPIO|SIOS_IO_XGPIO)) {
		int i = res->start;
		for (; i<=res->end; i++) {
			if (!__check_sios_gpio(res->start, res->type & SIOS_RESTYPE_MASK))
				return -EINVAL;
		}
	}

	write_lock(&resource_lock);
	conflict = __sios_request_resource(res);
	if (conflict)
		goto _conflict;
	list_add(&res->list, &resource_list);
_conflict:
	write_unlock(&resource_lock);
	return conflict ? -EBUSY : 0;
}
EXPORT_SYMBOL_GPL(sios_request_resource);

void sios_release_resource(struct sios_resource *res)
{
	if (!res)
		return;

	write_lock(&resource_lock);
	list_del_init(&res->list);
	write_unlock(&resource_lock);
}
EXPORT_SYMBOL_GPL(sios_release_resource);

int __check_sios_gpio(int pin, sios_restype_t type)
{
#define SGPIO(x) case x: return 1;
	if (type == SIOS_IO_GPIO) {
		switch(pin) {
#include "sios/gpio_res.h"
		}
	} else if (type == SIOS_IO_XGPIO) {
		switch(pin) {
#include "sios/xgpio_res.h"
		}
	}
#undef SGPIO
	return 0;
}
EXPORT_SYMBOL_GPL(__check_sios_gpio);
