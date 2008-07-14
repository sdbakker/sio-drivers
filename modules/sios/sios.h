/* -*-linux-c-*-
 * sios.h
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

#ifndef _SIOS_H_
#define _SIOS_H_

#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#include "resource.h"

struct sios_device {
	const char *name;
	void (*release)(struct sios_device *sdev);
	struct device dev;
	int num_resource;
	struct sios_resource *resource;
};

#define to_sios_device(x) container_of((x), struct sios_device, dev)

extern int __must_check sios_device_register(struct sios_device *sdev);
extern void sios_device_unregister(struct sios_device *sdev);

extern struct bus_type sios_bus_type;
extern struct device sios_bus;

extern void sios_device_put(struct sios_device *sdev);
extern int __must_check sios_device_add(struct sios_device *sdev);
extern void sios_device_del(struct sios_device *sdev);

struct sios_driver {
	const char *version;
	struct module *module;
	struct device_driver driver;

	int (*probe)(struct sios_device *);
	int (*remove)(struct sios_device *);
	void (*shutdown)(struct sios_device *);
	int (*suspend)(struct sios_device *, pm_message_t state);
	int (*suspend_late)(struct sios_device *, pm_message_t state);
	int (*resume_early)(struct sios_device *);
	int (*resume)(struct sios_device *);

	struct driver_attribute version_attr;
};

#define to_sios_driver(drv) (container_of((drv), struct sios_driver, driver))

extern int __must_check sios_driver_register(struct sios_driver *sdrv);
extern void sios_driver_unregister(struct sios_driver *sdrv);

#endif /* _SIOS_H_ */
