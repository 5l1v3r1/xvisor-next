/*
 * Driver core interface to the pinctrl subsystem.
 *
 * Copyright (C) 2012 ST-Ericsson SA
 * Written on behalf of Linaro for ST-Ericsson
 * Based on bits of regulator core, gpio core and clk core
 *
 * Author: Linus Walleij <linus.walleij@linaro.org>
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/device.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>

/**
 * pinctrl_bind_pins() - called by the device core before probe
 * @dev: the device that is just about to probe
 */
int pinctrl_bind_pins(struct device *dev)
{
	int ret;
	struct dev_pin_info *pins;

	dev->pins = devm_kzalloc(dev, sizeof(struct dev_pin_info), GFP_KERNEL);

	if (!dev->pins)
		return -ENOMEM;

	pins = (struct dev_pin_info *) dev->pins;

	pins->p =  devm_pinctrl_get(dev);
	if (IS_ERR(pins->p)) {
		dev_dbg(dev, "no pinctrl handle\n");
		ret = PTR_ERR(pins->p);
		goto cleanup_alloc;
	}

	pins->default_state = pinctrl_lookup_state(pins->p,
					PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pins->default_state)) {
		dev_dbg(dev, "no default pinctrl state\n");
		ret = 0;
		goto cleanup_get;
	}

	ret = pinctrl_select_state(pins->p, pins->default_state);
	if (ret) {
		dev_dbg(dev, "failed to activate default pinctrl state\n");
		goto cleanup_get;
	}

#ifdef CONFIG_PM
	/*
	 * If power management is enabled, we also look for the optional
	 * sleep and idle pin states, with semantics as defined in
	 * <linux/pinctrl/pinctrl-state.h>
	 */
	pins->sleep_state = pinctrl_lookup_state(pins->p,
					PINCTRL_STATE_SLEEP);
	if (IS_ERR(pins->sleep_state))
		/* Not supplying this state is perfectly legal */
		dev_dbg(dev, "no sleep pinctrl state\n");

	pins->idle_state = pinctrl_lookup_state(pins->p,
					PINCTRL_STATE_IDLE);
	if (IS_ERR(pins->idle_state))
		/* Not supplying this state is perfectly legal */
		dev_dbg(dev, "no idle pinctrl state\n");
#endif

	return 0;

	/*
	 * If no pinctrl handle or default state was found for this device,
	 * let's explicitly free the pin container in the device, there is
	 * no point in keeping it around.
	 */
cleanup_get:
	devm_pinctrl_put(pins->p);
cleanup_alloc:
	devm_kfree(dev, dev->pins);
	dev->pins = NULL;

	/* Only return deferrals */
	if (ret != -EPROBE_DEFER)
		ret = 0;

	return ret;
}

int vmm_platform_pinctrl_bind(struct vmm_device *dev)
{
	return pinctrl_bind_pins(dev);
}

int vmm_platform_pinctrl_init(struct vmm_device *dev)
{
	return VMM_OK;
}
