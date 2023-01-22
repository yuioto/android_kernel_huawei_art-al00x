/*
 * da_separate_v2 i2s pinctrl driver.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/pinctrl/consumer.h>
#include "audio_log.h"
#include "da_separate_v2_utility.h"
#include "da_separate_v2_i2s.h"

#define LOG_TAG "da_separate_v2_i2s"

static struct da_separate_v2_i2s_priv da_separate_v2_i2s_privdata = {0};

static struct da_separate_v2_i2s_priv* _get_privdata(void)
{
	return &da_separate_v2_i2s_privdata;
}

static int _set_i2s2_pinctrl_state(enum da_separate_v2_pinctrl_state state)
{
	int ret;
	struct da_separate_v2_i2s_priv *i2s_priv = _get_privdata();
	struct da_separate_v2_i2s2_priv *priv = NULL;

	IN_FUNCTION;

	WARN_ON(!i2s_priv);

	priv = i2s_priv->i2s2_priv;

	if (!priv) {
		AUDIO_LOGE("can not set i2s2 state because priv is NULL");
		return -ENODEV;
	}

	switch (state) {
	case STATE_DEFAULT:
		ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
		if (ret)
			AUDIO_LOGE("can not set pins to default state, ret=%d", ret);

		break;
	case STATE_IDLE:
		ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
		if (ret)
			AUDIO_LOGE("can not set pins to idle state, ret=%d", ret);

		break;
	default:
		ret = -EFAULT;
		AUDIO_LOGE("pinctrl state error:%d", state);
		break;
	}

	OUT_FUNCTION;

	return ret;

}

int da_separate_v2_i2s2_set_pinctrl_default(void)
{
	int ret;

	IN_FUNCTION;

	ret = _set_i2s2_pinctrl_state(STATE_DEFAULT);
	if (ret) {
		AUDIO_LOGE("i2s2 pinctrl set default state fail");
		return ret;
	}

	AUDIO_LOGI("i2s2 pinctrl set default state ok");

	OUT_FUNCTION;

	return ret;
}

int da_separate_v2_i2s2_set_pinctrl_idle(void)
{
	int ret;

	IN_FUNCTION;

	ret = _set_i2s2_pinctrl_state(STATE_IDLE);

	if (ret) {
		AUDIO_LOGE("i2s2 pinctrl set idle state fail");
		return ret;
	}

	AUDIO_LOGI("i2s2 pinctrl set idle state ok");

	OUT_FUNCTION;

	return ret;
}

static int _i2s2_pinctrl_init(struct platform_device *pdev)
{
	struct pinctrl *p = NULL;
	struct pinctrl_state *state = NULL;
	struct da_separate_v2_i2s2_priv *priv = NULL;

	IN_FUNCTION;
	priv = platform_get_drvdata(pdev);
	WARN_ON(!priv);

	/* I2S2 iomux config */
	p = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(p)) {
		AUDIO_LOGE("can not get pinctrl");
		return PTR_ERR(p);
	}

	priv->pctrl = p;

	state = pinctrl_lookup_state(p, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(state)) {
		AUDIO_LOGE("can not get default state (%li)", PTR_ERR(state));
		return PTR_ERR(state);
	}

	priv->pin_default = state;

	state = pinctrl_lookup_state(p, PINCTRL_STATE_IDLE);
	if (IS_ERR(state)) {
		AUDIO_LOGE("can not get idle state (%li)", PTR_ERR(state));
		return PTR_ERR(state);
	}

	priv->pin_idle = state;

	AUDIO_LOGI("pinctrl init ok");

	OUT_FUNCTION;

	return 0;
}

static void _i2s2_pinctrl_deinit(struct platform_device *pdev)
{
	struct da_separate_v2_i2s2_priv *priv = NULL;

	IN_FUNCTION;
	priv = platform_get_drvdata(pdev);
	WARN_ON(!priv);

	devm_pinctrl_put(priv->pctrl);

	AUDIO_LOGI("pinctrl deinit ok");

	OUT_FUNCTION;
}

/*lint -e429*/
static int da_separate_v2_i2s2_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct da_separate_v2_i2s_priv *i2s_priv = _get_privdata();
	struct da_separate_v2_i2s2_priv *priv = NULL;

	AUDIO_LOGI("Begin");
	WARN_ON(!i2s_priv);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		AUDIO_LOGE("memory alloc failed");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);

	ret = _i2s2_pinctrl_init(pdev);
	if (ret) {
		AUDIO_LOGE("pinctrl init failed! err code 0x%x", ret);
		return ret;
	}

	i2s_priv->i2s2_priv = priv;

	AUDIO_LOGI("End");
	return ret;
}
/*lint +e429*/

static void da_separate_v2_i2s2_shutdown(struct platform_device *pdev)
{
	struct da_separate_v2_i2s_priv *priv = _get_privdata();

	AUDIO_LOGI("Begin");
	WARN_ON(!priv);

	_i2s2_pinctrl_deinit(pdev);
	priv->i2s2_priv = NULL;

	AUDIO_LOGI("End");
}


static const struct of_device_id da_separate_v2_i2s2_match[] = {
	{ .compatible = "hisilicon,codec-i2s2", },
	{},
};

static struct platform_driver da_separate_v2_i2s2_driver = {
	.driver = {
		.name  = "da_separate_v2-i2s2",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(da_separate_v2_i2s2_match),
	},
	.probe  = da_separate_v2_i2s2_probe,
	.shutdown = da_separate_v2_i2s2_shutdown,
};

static int __init da_separate_v2_i2s2_init(void)
{
	AUDIO_LOGI("Begin");
	return platform_driver_register(&da_separate_v2_i2s2_driver);
}
module_init(da_separate_v2_i2s2_init);

static void __exit da_separate_v2_i2s2_exit(void)
{
	AUDIO_LOGI("Begin");
	platform_driver_unregister(&da_separate_v2_i2s2_driver);
}
module_exit(da_separate_v2_i2s2_exit);

MODULE_DESCRIPTION("da_separate_v2 i2s pinctrl driver");
MODULE_LICENSE("GPL");

