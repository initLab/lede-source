/*
 * D-Link DAP-2553 rev. A1 support
 *
 * Copyright (c) 2012 Qualcomm Atheros
 * Copyright (c) 2012-2013 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (c) 2016 Stijn Tintel <stijn@linux-ipv6.be>
 * Copyright (c) 2016 Petko Bordjukov <bordjukov@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-dsa.h"
#include "dev-eth.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "nvram.h"

#define DAP2553_GPIO_LED_POWER		3
#define DAP2553_GPIO_LED_WLAN_5G	19
#define DAP2553_GPIO_LED_WLAN_2G	20

#define DAP2553_GPIO_BTN_RESET		21

#define DAP2553_KEYS_POLL_INTERVAL	20	/* msecs */
#define DAP2553_KEYS_DEBOUNCE_INTERVAL	(3 * DAP2553_KEYS_POLL_INTERVAL)

#define DAP2553_NVRAM_ADDR		0x1f040000
#define DAP2553_NVRAM_SIZE		0x10000

#define DAP2553_MAC_OFFSET		1
#define DAP2553_WMAC_CALDATA_OFFSET	0x1000

static struct gpio_led dap2553_leds_gpio[] __initdata = {
	{
		.name		= "d-link:green:power",
		.gpio		= DAP2553_GPIO_LED_POWER,
		.active_low	= 0,
	},
	{
		.name		= "d-link:green:wlan2g",
		.gpio		= DAP2553_GPIO_LED_WLAN_2G,
		.active_low	= 0,
	},
	{
		.name		= "d-link:green:wlan5g",
		.gpio		= DAP2553_GPIO_LED_WLAN_5G,
		.active_low	= 0,
	}
};

static struct gpio_keys_button dap2553_gpio_keys[] __initdata = {
	{
		.desc			= "Soft reset",
		.type			= EV_KEY,
		.code			= KEY_RESTART,
		.debounce_interval	= DAP2553_KEYS_DEBOUNCE_INTERVAL,
		.gpio			= DAP2553_GPIO_BTN_RESET,
		.active_low		= 1,
	},
};

static void dap2553_get_mac(const char *name, char *mac)
{
	u8 *nvram = (u8 *) KSEG1ADDR(DAP2553_NVRAM_ADDR);
	int err;

	err = ath79_nvram_parse_mac_addr(nvram, DAP2553_NVRAM_SIZE,
					 name, mac);
	if (err)
		pr_err("no MAC address found for %s\n", name);
}

static void __init dap2553_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	u8 mac[ETH_ALEN], wmac[ETH_ALEN];

	dap2553_get_mac("lanmac=", mac);
	dap2553_get_mac("wlanmac=", wmac);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(dap2553_leds_gpio),
				 dap2553_leds_gpio);

	ath79_register_gpio_keys_polled(-1, DAP2553_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(dap2553_gpio_keys),
					dap2553_gpio_keys);

	ath79_register_wmac(art + DAP2553_WMAC_CALDATA_OFFSET, wmac);

	ath79_register_mdio(0, 0x0);

	/* GMAC0 is connected to the RGMII interface */
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, DAP2553_MAC_OFFSET);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio0_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0x56000000;

	ath79_register_eth(0);

	ath79_register_m25p80(NULL);

	ath79_register_pci();
}

MIPS_MACHINE(ATH79_MACH_DAP_2553_A1, "DAP-2553-A1",
		"D-Link DAP-2553 rev. A1",
		dap2553_setup);
