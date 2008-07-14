/* -*-linux-c-*-
 * bus.c - SIOS sensorboard bus
 *
 * Copyright (C) 2008 V2_lab, Simon de Bakker <simon@v2.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA, or http://www.gnu.org/licenses/gpl.html
 */

#include <linux/module.h>
#include <linux/init.h>

#include "sios/sios.h"

static void sios_dev_release(struct device *dev)
{
	struct sios_device *sdev = to_sios_device(dev);
	sdev->release(sdev);
}

void sios_device_put(struct sios_device *sdev)
{
	if (sdev)
		put_device(&sdev->dev);
}
EXPORT_SYMBOL_GPL(sios_device_put);

int sios_device_add(struct sios_device *sdev)
{
	int error;
	int rcnt;

	if (!sdev)
		return -EINVAL;

	if (!sdev->dev.parent)
		sdev->dev.parent = &sios_bus;

	if (sdev->release)
		sdev->dev.release = sios_dev_release;

	sdev->dev.bus = &sios_bus_type;
	strlcpy(sdev->dev.bus_id, sdev->name, BUS_ID_SIZE);

	error = device_add(&sdev->dev);
	if (error)
		goto err;

	for (rcnt=0; rcnt<sdev->num_resource; rcnt++) {
		struct sios_resource *r = &sdev->resource[rcnt];
		r->dev = sdev;
		if (!r->name) {
			snprintf(r->s_name, BUS_ID_SIZE, "%s:%02d-%02d", sdev->dev.bus_id, r->start, r->end);
			r->name = r->s_name;
		}
		error = sios_request_resource(sdev, r);
		if (error) {
			printk(KERN_WARNING "bus resource request failed: name=%s, res=%d-%d\n", 
			       r->name, r->start, r->end);
			goto release;
		}
		error = sios_resource_create_file(sdev, r);
		if (error) {
		        printk(KERN_WARNING "bus resource create file failed: name=%s, res=%d-%d\n",
			    r->name, r->start, r->end);
			sios_release_resource(r);
			goto release;
		}
	}

	return 0;

release:
	for (rcnt--; rcnt>=0; --rcnt) {
		struct sios_resource *r = &sdev->resource[rcnt];
		sios_resource_remove_file(r);
		sios_release_resource(r);
	}
	device_del(&sdev->dev);
err:	
	return error;
}
EXPORT_SYMBOL_GPL(sios_device_add);

void sios_device_del(struct sios_device *sdev)
{
	int i;

	if (!sdev)
		return;

	for (i=0; i<sdev->num_resource; i++) {
		struct sios_resource *r = &sdev->resource[i];
		sios_resource_remove_file(r);
		sios_release_resource(r);
	}
	device_del(&sdev->dev);
}
EXPORT_SYMBOL_GPL(sios_device_del);

int sios_device_register(struct sios_device *sdev)
{
	device_initialize(&sdev->dev);
	return sios_device_add(sdev);
}
EXPORT_SYMBOL_GPL(sios_device_register);

void sios_device_unregister(struct sios_device *sdev)
{
	sios_device_del(sdev);
	sios_device_put(sdev);
}
EXPORT_SYMBOL_GPL(sios_device_unregister);

static ssize_t show_sios_drv_version(struct device_driver *drv, char *buf)
{
	struct sios_driver *sdrv = to_sios_driver(drv);
	return snprintf(buf, PAGE_SIZE, "%s\n", sdrv->version);
}

static int sios_drv_probe(struct device *dev)
{
	struct sios_driver *sdrv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);

	printk(KERN_INFO "bus probe drv: %s, dev: %s\n", sdrv->driver.name, sdev->name);

	return sdrv->probe(sdev);
}

static int sios_drv_remove(struct device *dev)
{
	struct sios_driver *sdrv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);

	printk(KERN_INFO "bus remove drv: %s, dev: %s\n", sdrv->driver.name, sdev->name);

	return sdrv->remove(sdev);
}

static void sios_drv_shutdown(struct device *dev)
{
	struct sios_driver *sdrv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);

	sdrv->shutdown(sdev);
}

static int sios_drv_suspend(struct device *dev, pm_message_t state)
{
	struct sios_driver *sdrv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);

	return sdrv->suspend(sdev, state);
}

static int sios_drv_resume(struct device *dev)
{
	struct sios_driver *sdrv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);

	return sdrv->resume(sdev);
}

/**
 *	sios_driver_register
 *	@drv: sios driver structure
 */
int sios_driver_register(struct sios_driver *drv)
{
	int error;

	drv->driver.bus = &sios_bus_type;
	if (drv->probe)
		drv->driver.probe = sios_drv_probe;
	if (drv->remove)
		drv->driver.remove = sios_drv_remove;
	if (drv->shutdown)
		drv->driver.shutdown = sios_drv_shutdown;
	if (drv->suspend)
		drv->driver.suspend = sios_drv_suspend;
	if (drv->resume)
		drv->driver.resume = sios_drv_resume;

	error = driver_register(&drv->driver);
	if (error)
		return error;

	drv->version_attr.attr.name = "version";
	drv->version_attr.attr.owner = drv->module;
	drv->version_attr.attr.mode = S_IRUGO;
	drv->version_attr.show = show_sios_drv_version;
	drv->version_attr.store = NULL;

	error = driver_create_file(&drv->driver, &drv->version_attr);
	if (error) {
		printk(KERN_ERR "version attribute creation failed: %d\n", error);
		driver_unregister(&drv->driver);
	}

	return error;
}
EXPORT_SYMBOL_GPL(sios_driver_register);

/**
 *	sios_driver_unregister
 *	@drv: sios driver structure
 */
void sios_driver_unregister(struct sios_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL_GPL(sios_driver_unregister);


static void sios_release(struct device *dev)
{
	printk(KERN_INFO "sios_bus release: %s\n", dev->bus_id);
}

struct device sios_bus = {
	.bus_id = "sios",
	.release = sios_release,
};
EXPORT_SYMBOL_GPL(sios_bus);

static int sios_match(struct device *dev, struct device_driver *drv)
{
	int result;
	/* FIXME: transfer into something senseble */
	result = strncmp(dev->bus_id, drv->name, strlen(drv->name));
	printk(KERN_INFO "bus match dev='%s' == drv='%s' -> %d\n", dev->bus_id, drv->name, result);
	return (result == 0);
}

static int sios_uevent(struct device *dev, char **envp, 
		       int num_envp, char *buffer, int buffer_size)
{
	struct sios_device *sdev = to_sios_device(dev);

	envp[0] = buffer;
	snprintf(buffer, buffer_size, "MODALIAS=%s", sdev->name);
	return 0;
}

static int sios_suspend(struct device *dev, pm_message_t mesg)
{
	int ret = 0;

	if (dev->driver && dev->driver->suspend)
		ret = dev->driver->suspend(dev, mesg);

	return ret;
}

static int sios_suspend_late(struct device *dev, pm_message_t mesg)
{
	struct sios_driver *drv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);
	int ret = 0;

	if (dev->driver && drv->suspend_late)
		ret = drv->suspend_late(sdev, mesg);

	return ret;
}

static int sios_resume_early(struct device *dev)
{
	struct sios_driver *drv = to_sios_driver(dev->driver);
	struct sios_device *sdev = to_sios_device(dev);
	int ret = 0;

	if (dev->driver && drv->resume_early)
		ret = drv->resume_early(sdev);

	return ret;
}

static int sios_resume(struct device * dev)
{
	int ret = 0;

	if (dev->driver && dev->driver->resume)
		ret = dev->driver->resume(dev);

	return ret;
}

struct bus_type sios_bus_type = {
	.name = "sios",
	.match = sios_match,
	.uevent = sios_uevent,
	.suspend = sios_suspend,
	.suspend_late = sios_suspend_late,
	.resume_early = sios_resume_early,
	.resume = sios_resume,
};
EXPORT_SYMBOL_GPL(sios_bus_type);

static int __init sios_bus_init(void)
{
	int error;
	
	error = device_register(&sios_bus);
	if (error)
		return error;
	error = bus_register(&sios_bus_type);
	if (error)
		device_unregister(&sios_bus);
	return error;
}

static void __exit sios_bus_exit(void)
{
	device_unregister(&sios_bus);
	bus_unregister(&sios_bus_type);
}

MODULE_DESCRIPTION("SIOS bus driver");
MODULE_AUTHOR("Simon de Bakker <simon@v2.nl>");
MODULE_LICENSE("GPL");

module_init(sios_bus_init);
module_exit(sios_bus_exit);
