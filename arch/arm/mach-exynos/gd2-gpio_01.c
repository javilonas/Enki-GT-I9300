/*
 *  linux/arch/arm/mach-exynos/midas-gpio.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - GPIO setting in set board
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <plat/devs.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <mach/gpio-midas.h>
#include <plat/cpu.h>
#include <mach/pmu.h>

struct gpio_init_data {
	uint num;
	uint cfg;
	uint val;
	uint pud;
	uint drv;
};

/*
 * gd2 GPIO Init Table
 */
static struct gpio_init_data gd2_init_gpios[] = {
        {EXYNOS4_GPA1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPA1(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */

        {EXYNOS4_GPB(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* CODEC_SDA_1.8V */
	{EXYNOS4_GPB(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* CODEC_SCL_1.8V */

	{EXYNOS4_GPD0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* AP_PMIC_SDA_1.8V */
	{EXYNOS4_GPD0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* AP_PMIC_SCL_1.8V */

	{EXYNOS4_GPD1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* SENSOR_SDA_1.8V */
	{EXYNOS4_GPD1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* SENSOR_SCL_1.8V */
	{EXYNOS4_GPD1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* RGB_SDA_1.8V */
	{EXYNOS4_GPD1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* RGB_SCL_1.8V */

	{EXYNOS4_GPX0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* ACC_INT */
	{EXYNOS4_GPX0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_ZERO,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPX0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* GYRO_INT */
	{EXYNOS4_GPX0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* BOOT_MODE */
	{EXYNOS4_GPX0(7), S3C_GPIO_SFN(0xF), S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* AP_PMIC_IRQ */

	{EXYNOS4_GPX1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* JOG_PUSH */

	{EXYNOS4_GPX2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* PS_ALS_INT */
	{EXYNOS4_GPX2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* MSENSOR_INT */
	{EXYNOS4_GPX2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_ONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* JOG_R */
	{EXYNOS4_GPX2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_ONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* FUEL_ALERT */
	{EXYNOS4_GPX2(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* BT_HOST_WAKEUP */
	{EXYNOS4_GPX2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* nPower */

	{EXYNOS4_GPX3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* EAR_SEND_END */
	{EXYNOS4_GPX3(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* AP2MDM_HSIC_PORT_ACTIVE */
	{EXYNOS4_GPX3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* JOG_L */
	{EXYNOS4_GPX3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* T_FLASH_DETECT */

	{EXYNOS4_GPK3(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* WLAN_SDIO_CMD */
	{EXYNOS4_GPK3(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* WLAN_SDIO_D(0) */
	{EXYNOS4_GPK3(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* WLAN_SDIO_D(1) */
	{EXYNOS4_GPK3(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* WLAN_SDIO_D(2) */
	{EXYNOS4_GPK3(6), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* WLAN_SDIO_D(3) */

	{EXYNOS4_GPL1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_UP, S5P_GPIO_DRVSTR_LV1}, /* GPIO_CAM_SCL - PULL UP */
	{EXYNOS4_GPL1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_UP, S5P_GPIO_DRVSTR_LV1}, /* GPIO_CAM_SDA - PULL UP */

	{EXYNOS4_GPL2(7), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* STR_PU_KEY */

	{EXYNOS4_GPY0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY0(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY0(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY0(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY0(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY0(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */

	{EXYNOS4_GPY1(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY1(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* D4_INT */
	{EXYNOS4_GPY1(3), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* D4_PCU_MRB */

	{EXYNOS4_GPY2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY2(2), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */
	{EXYNOS4_GPY2(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* AP2MDM_PMIC_RESET_N */
	{EXYNOS4_GPY2(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* S_LED_SDA */
	{EXYNOS4_GPY2(5), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* S_LED_SCL */

	{EXYNOS4212_GPJ0(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV4}, /* WLAN_EN */

	{EXYNOS4212_GPJ1(3), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV2}, /* CAM_MCLK_TP */
	{EXYNOS4212_GPJ1(4), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* BT_WAKE */

	{EXYNOS4212_GPM1(0), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* SHUTTER_KEY_EN */
	{EXYNOS4212_GPM1(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_DOWN, S5P_GPIO_DRVSTR_LV1}, /* NC */

	{EXYNOS4212_GPM2(0), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* IF_PMIC_SDA_1.8V */
	{EXYNOS4212_GPM2(1), S3C_GPIO_INPUT, S3C_GPIO_SETPIN_NONE,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* IF_PMIC_SCL_1.8V */

	{EXYNOS4212_GPM3(0), S3C_GPIO_OUTPUT, S3C_GPIO_SETPIN_ZERO,
		S3C_GPIO_PULL_NONE, S5P_GPIO_DRVSTR_LV1}, /* PMIC_DVS1 */
};

/*
 * gd2 GPIO Sleep Table
 */

static unsigned int gd2_sleep_gpio_table[][3] = {
	{EXYNOS4_GPA0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* BT_UART_RXD */
	{EXYNOS4_GPA0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP}, /* BT_UART_TXD */
	{EXYNOS4_GPA0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* BT_UART_CTS */
	{EXYNOS4_GPA0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP}, /* BT_UART_RTS */
	{EXYNOS4_GPA0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP}, /* GPS_UART_RXD */
	{EXYNOS4_GPA0(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP}, /* GPS_UART_TXD */
	{EXYNOS4_GPA0(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* GPS_UART_CTS */
	{EXYNOS4_GPA0(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* GPS_UART_RTS */

	{EXYNOS4_GPA1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AP_RXD */
	{EXYNOS4_GPA1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AP_TXD */
	{EXYNOS4_GPA1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* TSP_SDA_1.8V */
	{EXYNOS4_GPA1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* TSP_SCL_1.8V */
	{EXYNOS4_GPA1(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPA1(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPB(0),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* CODEC_SDA_1.8V */
	{EXYNOS4_GPB(1),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* CODEC_SCL_1.8V */
	{EXYNOS4_GPB(2),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* DDC_SDA_1.8V */
	{EXYNOS4_GPB(3),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* DDC_DSCL_1.8V */
	{EXYNOS4_GPB(4),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_SPI_SCK */
	{EXYNOS4_GPB(5),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_SPI_CS */
	{EXYNOS4_GPB(6),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_SPI_DOUT */
	{EXYNOS4_GPB(7),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_SPI_DIN */

	{EXYNOS4_GPC0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* LCD_1.8V_EN(NC) */
	{EXYNOS4_GPC0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPC1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPC1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* FUEL_SDA_1.8V */
	{EXYNOS4_GPC1(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* FUEL_SCL_1.8V */

	{EXYNOS4_GPD0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* VIBTONE_PWM */
	{EXYNOS4_GPD0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPD0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_PMIC_SDA_1.8V */
	{EXYNOS4_GPD0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_PMIC_SCL_1.8V */

	{EXYNOS4_GPD1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* D4_SDA_1.8V */
	{EXYNOS4_GPD1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* D4_SCL_1.8V */
	{EXYNOS4_GPD1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* SENSOR_SDA_1.8V */
	{EXYNOS4_GPD1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* SENSOR_SCL_1.8V */

	{EXYNOS4_GPF0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_HSYNC */
	{EXYNOS4_GPF0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_VSYNC */
	{EXYNOS4_GPF0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* OTG_EN */
	{EXYNOS4_GPF0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_VLCLK */
	{EXYNOS4_GPF0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_0 */
	{EXYNOS4_GPF0(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_1 */
	{EXYNOS4_GPF0(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_2 */
	{EXYNOS4_GPF0(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_3 */

	{EXYNOS4_GPF1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_4 */
	{EXYNOS4_GPF1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_5 */
	{EXYNOS4_GPF1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_6 */
	{EXYNOS4_GPF1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_B_7 */
	{EXYNOS4_GPF1(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_0 */
	{EXYNOS4_GPF1(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_1 */
	{EXYNOS4_GPF1(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_2 */
	{EXYNOS4_GPF1(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_3 */

	{EXYNOS4_GPF2(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_4 */
	{EXYNOS4_GPF2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_5 */
	{EXYNOS4_GPF2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_6 */
	{EXYNOS4_GPF2(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_G_7 */
	{EXYNOS4_GPF2(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_0 */
	{EXYNOS4_GPF2(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_1 */
	{EXYNOS4_GPF2(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_2 */
	{EXYNOS4_GPF2(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_3 */

	{EXYNOS4_GPF3(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_4 */
	{EXYNOS4_GPF3(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_5 */
	{EXYNOS4_GPF3(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_6 */
	{EXYNOS4_GPF3(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D_EVF_R_7 */
	{EXYNOS4_GPF3(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* RESERVED_TP */
	{EXYNOS4_GPF3(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MHL_INT */

	{EXYNOS4_GPK0(0),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_CLK */
	{EXYNOS4_GPK0(1),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_CMD */
	{EXYNOS4_GPK0(2),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* eMMC_EN */
	{EXYNOS4_GPK0(3),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(0) */
	{EXYNOS4_GPK0(4),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(1) */
	{EXYNOS4_GPK0(5),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(2) */
	{EXYNOS4_GPK0(6),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(3) */

	{EXYNOS4_GPK1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPK1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPK1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPK1(3),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(4) */
	{EXYNOS4_GPK1(4),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(5) */
	{EXYNOS4_GPK1(5),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(6) */
	{EXYNOS4_GPK1(6),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* NAND_D(7) */

	{EXYNOS4_GPK2(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_CLK */
	{EXYNOS4_GPK2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_CMD */
	{EXYNOS4_GPK2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPK2(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_D(0) */
	{EXYNOS4_GPK2(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_D(1) */
	{EXYNOS4_GPK2(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_D(2) */
	{EXYNOS4_GPK2(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* T_FLASH_D(3) */

	{EXYNOS4_GPK3(0),  S3C_GPIO_SLP_OUT0,  S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_CLK */
	{EXYNOS4_GPK3(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_CMD */
	{EXYNOS4_GPK3(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPK3(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_D(0) */
	{EXYNOS4_GPK3(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_D(1) */
	{EXYNOS4_GPK3(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_D(2) */
	{EXYNOS4_GPK3(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* WLAN_SDIO_D(3) */

	{EXYNOS4_GPL0(0),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* BUCK2_SEL */
	{EXYNOS4_GPL0(1),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* BUCK3_SEL */
	{EXYNOS4_GPL0(2),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* BUCK4_SEL */
	{EXYNOS4_GPL0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPL0(4),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* HDMI_EN */
	{EXYNOS4_GPL0(6),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /*BT_EN */

	{EXYNOS4_GPL1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* GPIO_CAM_SCL */
	{EXYNOS4_GPL1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* GPIO_CAM_SDA */

	{EXYNOS4_GPL2(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* CP_D4_USB_SEL */
	{EXYNOS4_GPL2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* CP_D4_USB_EN */
	{EXYNOS4_GPL2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* HDMI_CT_CP_HPD */
	{EXYNOS4_GPL2(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EXTMIC EN */
	{EXYNOS4_GPL2(4),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* GPS_EN */
	{EXYNOS4_GPL2(5),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* AP2MDM_PON_RESET_N */
	{EXYNOS4_GPL2(6),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* CAMERA_POWER_ON */
	{EXYNOS4_GPL2(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* STR_PU_KEY */

	{EXYNOS4_GPX0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPX3(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EAR_SEND_END */

	{EXYNOS4_GPY0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY0(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPY1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_INT */
	{EXYNOS4_GPY1(3),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* D4_PCU_MRB */

	{EXYNOS4_GPY2(0),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE},  /* TF_EN */
	{EXYNOS4_GPY2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EXTMIC_EN */
	{EXYNOS4_GPY2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY2(3),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* AP2MDM_PMIC_RESET_N */
	{EXYNOS4_GPY2(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* S_LED_SDA */
	{EXYNOS4_GPY2(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* S_LED_SCL */

	{EXYNOS4_GPY3(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_A0 */
	{EXYNOS4_GPY3(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_A1 */
	{EXYNOS4_GPY3(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_A2 */
	{EXYNOS4_GPY3(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_PRCHG */
	{EXYNOS4_GPY3(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_SIGDEV */
	{EXYNOS4_GPY3(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_+FSET */
	{EXYNOS4_GPY3(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_SDOUT */
	{EXYNOS4_GPY3(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_CS */

	{EXYNOS4_GPY4(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AVS_SCK */
	{EXYNOS4_GPY4(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY4(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPY5(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY5(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPY6(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPY6(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4_GPZ(0),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MM_I2S_CLK */
	{EXYNOS4_GPZ(1),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPZ(2),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MM_I2S_SYNC */
	{EXYNOS4_GPZ(3),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MM_I2S_DI */
	{EXYNOS4_GPZ(4),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MM_I2S_DO */
	{EXYNOS4_GPZ(5),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4_GPZ(6),   S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	/* Exynos4212 specific gpio */
	{EXYNOS4212_GPJ0(0),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* WLAN_EN */
	{EXYNOS4212_GPJ0(1),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /*AP2MDM_ERR_FATAL*/
	{EXYNOS4212_GPJ0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* LCD_ID */
	{EXYNOS4212_GPJ0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* MDM2AP_HSIC_READY */
	{EXYNOS4212_GPJ0(4),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* CODEC_LDO_EN */
	{EXYNOS4212_GPJ0(5),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* AP2MDM_STATUS */
	{EXYNOS4212_GPJ0(6),  S3C_GPIO_SLP_PREV,  S3C_GPIO_PULL_NONE}, /* MICBIAS_EN */
	{EXYNOS4212_GPJ0(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* VGH_DET */

	{EXYNOS4212_GPJ1(0),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* MDM2AP_HSIC_PWR_ACTIVE */
	{EXYNOS4212_GPJ1(1),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* WCN_PRIORITY */
	{EXYNOS4212_GPJ1(2),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* MDM_LTE_FRAME_SYNC */
	{EXYNOS4212_GPJ1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* TP(CAM_MCLK) */
	{EXYNOS4212_GPJ1(4),  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, /* BT_WAKE */

	{EXYNOS4212_GPM0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM0(1),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_UP}, /* GPIO_MSENSE_RST_N */
	{EXYNOS4212_GPM0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM0(3),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_DOWN}, /* GPIO_PS_ALS_EN */
	{EXYNOS4212_GPM0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM0(5),  S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE}, /* D4_COLD */
	{EXYNOS4212_GPM0(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_RESERVED_1 */
	{EXYNOS4212_GPM0(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* SHUTTER_KEY1 */

	{EXYNOS4212_GPM1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* SHUTTER_KEY_EN */
	{EXYNOS4212_GPM1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_HW_REV0 */
	{EXYNOS4212_GPM1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_HW_REV0 */
	{EXYNOS4212_GPM1(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_HW_REV0 */
	{EXYNOS4212_GPM1(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* AP_HW_REV0 */
	{EXYNOS4212_GPM1(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* D4_ASV_READ */

	{EXYNOS4212_GPM2(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* IF_PMIC_SDA_1.8V */
	{EXYNOS4212_GPM2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE}, /* IF_PMIC_SCL_1.8V */
	{EXYNOS4212_GPM2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM2(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* TSP_INT */
	{EXYNOS4212_GPM2(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AP2MDM_WAKEUP */

	{EXYNOS4212_GPM3(0),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* PMIC_DVS1 */
	{EXYNOS4212_GPM3(1),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* PMIC_DVS2 */
	{EXYNOS4212_GPM3(2),  S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, /* PMIC_DVS3 */
	{EXYNOS4212_GPM3(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* AP2MDM_SOFT_RESET */
	{EXYNOS4212_GPM3(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM3(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM3(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM3(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4212_GPM4(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM4(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM4(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPM4(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_RESET */
	{EXYNOS4212_GPM4(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_SIO_CLK */
	{EXYNOS4212_GPM4(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_SIO_EN */
	{EXYNOS4212_GPM4(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* TP */
	{EXYNOS4212_GPM4(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* EVF_SIO_MOSI */

	{EXYNOS4212_GPV0(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV0(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4212_GPV1(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV1(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4212_GPV2(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV2(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4212_GPV3(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(2),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(3),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(4),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(5),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(6),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV3(7),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */

	{EXYNOS4212_GPV4(0),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
	{EXYNOS4212_GPV4(1),  S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN}, /* NC */
}; /* gd2_sleep_gpio_table */

/*
 * GD2 Main rev0.4 GPIO Sleep Table
 */
static unsigned int gd2_sleep_gpio_table_rev4[][3] = {
	{EXYNOS4212_GPM0(5),  S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE}, /* D4_COLD */
};

struct gd2_sleep_table {
	unsigned int (*ptr)[3];
	int size;
};

#define GPIO_TABLE(_ptr) \
	{.ptr = _ptr, \
	.size = ARRAY_SIZE(_ptr)} \

#define GPIO_TABLE_NULL \
	{.ptr = NULL, \
	.size = 0} \

static struct gd2_sleep_table gd2_sleep_table[] = {
	GPIO_TABLE(gd2_sleep_gpio_table),	/* Rev0.0 - Universal */
	GPIO_TABLE_NULL,
	GPIO_TABLE_NULL,
	GPIO_TABLE_NULL,
	GPIO_TABLE_NULL,
	GPIO_TABLE(gd2_sleep_gpio_table_rev4),	/* Rev0.4 - HW ID 5 */
};

static void config_sleep_gpio_table(int array_size,
				    unsigned int (*gpio_table)[3])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
		s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}
}

static void gd2_config_sleep_gpio_table(void)
{
	int i;
	int index = min(ARRAY_SIZE(gd2_sleep_table), system_rev + 1);

	for (i = 0; i < index; i++) {
		if (gd2_sleep_table[i].ptr == NULL)
			continue;

		config_sleep_gpio_table(gd2_sleep_table[i].size,
				gd2_sleep_table[i].ptr);
	}
}

/* To save power consumption, gpio pin set before enterling sleep */
void midas_config_sleep_gpio_table(void)
{
	gd2_config_sleep_gpio_table();
}

/* Intialize gpio set in midas board */
void midas_config_gpio_table(void)
{
	u32 i, gpio;

	printk(KERN_DEBUG "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(gd2_init_gpios); i++) {
		gpio = gd2_init_gpios[i].num;
		if (gpio <= EXYNOS4212_GPV4(1)) {
			s3c_gpio_cfgpin(gpio, gd2_init_gpios[i].cfg);
			s3c_gpio_setpull(gpio, gd2_init_gpios[i].pud);

			if (gd2_init_gpios[i].val != S3C_GPIO_SETPIN_NONE)
				gpio_set_value(gpio, gd2_init_gpios[i].val);

			s5p_gpio_set_drvstr(gpio, gd2_init_gpios[i].drv);
		}
	}
}
