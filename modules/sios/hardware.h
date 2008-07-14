/* hardware.h - SIOS sensorboard driver
 * -*-linux-c-*-
 *
 * Copyright (C) 2005 V2_lab, Simon de Bakker <simon@v2.nl>
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

#ifndef __SIOS_HARDWARE_H__
#define __SIOS_HARDWARE_H__

#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/gpio.h>
#include <linux/types.h>

#include "sios-gpio.h"

#define SIOS_DEVICE_CLASS_NAME	"sios"
#define SIOS_DEVICE_PREFIX	SIOS_DEVICE_CLASS_NAME "_"
#define SIOS_BASE_MINOR		0
#define SIOS_MAX_MINOR		255

#define SENSORS_CLASS_NAME	"sensors"

/* AD7998 ADC 
 * possible address:
 * AD7998-0: 		0x21 0x22 
 * AD7998-1: 		0x23 0x24
 * Both (float as):	0x20
 */
#define SIOS_AD7998_ADDR		0x21	/* 0x24 */
#define SIOS_AD7994_ADDR		0x22	/* 0x22 */

/* AD5254 digipot 
 * possible addresses:
 * 0x2c 0x2d 0x2e 0x2f
 */
#define SIOS_AD5254_ADDR		0x2c

/* PCA9557 8-GPIO 
 * possible address:
 * 0x18 0x19 0x1a 0x1b 0x1c 0x1d 0x1e 0x1f 
 */
#define SIOS_PCA9557_ADDR		0x18

/* DS2482-800 8-chan 1-Wire master 
 * possible address:
 * 0x18 0x19 0x1a 0x1b 0x1c 0x1d 0x1e 0x1f 
 */
#define SIOS_DS2482_ADDR		0x19

#define GPIO_SIOS_ST(x) 	(GPSR(x) = GPIO_bit(x))
#define GPIO_SIOS_GST(x)	((GPLR(x) & GPIO_bit(x)) >> ((x) & 0x1F))
#define GPIO_SIOS_SST(x,y) 	((x) ? GPIO_SIOS_ST(y) : GPIO_SIOS_CL(y))
#define GPIO_SIOS_TOG(x) 	(GPIO_SIOS_GST(x) ? GPIO_SIOS_CL(x) : GPIO_SIOS_ST(x))

enum mm_chip_id {
	CHIP_ID_AD7998 = SIOS_AD7998_ADDR,
	CHIP_ID_AD7994 = SIOS_AD7994_ADDR,
	CHIP_ID_PCA9557 = SIOS_PCA9557_ADDR,
	CHIP_ID_AD5254 = SIOS_AD5254_ADDR,
	CHIP_ID_DS2482 = SIOS_DS2482_ADDR,
};

#define GPIO_SIOS_I2C_ALERT_MD	(GPIO_SIOS_I2C_ALERT | GPIO_IN)
#define SIOS_I2C_ALERT_IRQ	IRQ_GPIO(GPIO_SIOS_I2C_ALERT)
#define GPIO_SIOS_I2C_CNVT_MD	(GPIO_SIOS_I2C_CNVT | GPIO_OUT)
#define GPIO_SIOS_I2C_CNVG_MD	(GPIO_SIOS_I2C_CNVG | GPIO_OUT)
#define GPIO_SIOS_ROW_SELECT_STROBE_MD	(GPIO_SIOS_ROW_SELECT_STROBE | GPIO_OUT)

/*#define SIOS_RST_IRQ		IRQ_GPIO(GPIO_SIOS_RST) */
#define SIOS_PBST_IRQ		IRQ_GPIO(GPIO_SIOS_PBST)

/* 
 * Main power supply.
 * Should be on already and on at all times 
 */
static __inline void sios_power_on(void) { gpio_set_value(GPIO_SIOS_PWR, 1); }

/*
 * Turn off all power supplies including SIOS_PWR.
 * Only use this in case of immediate immergency.
 * For proper shutdown trigger a uevent so shutdown can be
 * handled in userspace.
 */ 
extern void sios_power_off(void);

/* auxiliary functions */
static __inline void sios_hotswap_on(void) { gpio_set_value(GPIO_SIOS_HS, 1); }
static __inline void sios_hotswap_off(void) { gpio_set_value(GPIO_SIOS_HS, 0); }
static __inline void sios_peripheral_on(void) { gpio_set_value(GPIO_SIOS_VDD2, 1); }
static __inline void sios_peripheral_off(void) { gpio_set_value(GPIO_SIOS_VDD2, 0); }
static __inline void sios_usb_hipower_on(void) { gpio_set_value(GPIO_SIOS_USBHP, 1); }
static __inline void sios_usb_hipower_off(void) { gpio_set_value(GPIO_SIOS_USBHP, 0); }

#endif
