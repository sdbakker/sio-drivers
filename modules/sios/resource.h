/* -*-linux-c-*- */

#ifndef _SIOS_RESOURCE_
#define _SIOS_RESOURCE_

#include <linux/device.h>
#include "sios.h"

typedef int __bitwise sios_restype_t;
#define SIOS_IO_GPIO  (__force sios_restype_t) 0x0001
#define	SIOS_IO_XGPIO (__force sios_restype_t) 0x0002
#define SIOS_IRQ      (__force sios_restype_t) 0x0100
#define SIOS_FIRQ     (__force sios_restype_t) 0x0200

#define SIOS_RESTYPE_MASK 0x00ff
#define SIOS_RESFLAG_MASK 0xff00

struct sios_resource;

struct sios_resource_attribute {
	struct device_attribute dev_attr;
	struct sios_resource *res;
};

#define to_sios_res_attr(x) container_of((x), struct sios_resource_attribute, dev_attr)

struct sios_resource {
	const char *name;
	char s_name[BUS_ID_SIZE];
	struct sios_device *dev;
	sios_restype_t type;
	ssize_t start;
	ssize_t end;
	struct sios_resource_attribute res_attr;
	struct list_head list;
};

extern int sios_init_resource(struct sios_device *sdev, struct sios_resource *res);
extern int __must_check sios_request_resource(struct sios_device *sdev, struct sios_resource *res);
extern void sios_release_resource(struct sios_resource *res);

extern int __sios_resource_create_file(struct sios_device *sdev,
				       struct sios_resource_attribute *sattr);
extern int __must_check sios_resource_create_file(struct sios_device *sdev,
						 struct sios_resource *res);
extern void __sios_resource_remove_file(struct sios_device *sdev,
					struct sios_resource_attribute *sattr);
extern void sios_resource_remove_file(struct sios_resource *res);

extern int __must_check __check_sios_gpio(int pin, sios_restype_t type);
#define check_sios_gpio(x) __check_sios_gpio((x), SIOS_IO_GPIO)
#define check_sios_xgpio(x) __check_sios_gpipo((x), SIOS_IO_XGPIO);

#endif /* _SIOS_RESOURCE_ */
