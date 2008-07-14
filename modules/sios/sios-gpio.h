/* -*-linux-c-*- */

#ifndef _SIOS_GPIO_H_
#define _SIOS_GPIO_H_

#define GPIO_SIOS_RS_STROBE	28      /* row select strobe temp. attached to GPIO28*/
#define GPIO_SIOS_SPI_CSA0	66	/* SPI-bus Chip-select address bit */
#define GPIO_SIOS_SPI_CSA1	67	/* SPI-bus Chip-select address bit */
#define GPIO_SIOS_SPI_CSA2     	68	/* SPI-bus Chip-select address bit */
#define GPIO_SIOS_HS		69	/* Hot-Swap Power enable */
#define GPIO_SIOS_PWR		70	/* Gumstix Power enable */
#define GPIO_SIOS_VDD2		71	/* Peripheral Power enable */
#define GPIO_SIOS_USBHP		72	/* USB-HiPower enable */
#define GPIO_SIOS_RST		73	/* Reset signal to PCA9557 */
#define GPIO_SIOS_PBST		74	/* Powerbutton state input */
#define GPIO_SIOS_I2C_ALERT	75	/* I2C-bus interrupt/alert signal */
#define GPIO_SIOS_I2C_CNVT	76	/* AD7998 Conversion Start */
#define GPIO_SIOS_I2C_CNVG	77	/* AD7994 Conversion Start */
#define GPIO_SIOS_SPI_CLK	81
#define GPIO_SIOS_SPI_CS	82
#define GPIO_SIOS_SPI_MOSI	83
#define GPIO_SIOS_SPI_MISO	84

#endif
