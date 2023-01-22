/*
 * da_separate_v2_driver codec driver.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/clk.h>
#include <linux/version.h>
#include "audio_log.h"
#include "da_separate_v2.h"

#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
#include "hicodec_debug.h"
#endif

#define LOG_TAG "da_separate_v2"

/* DA_SEPARATE_V2 REGISTER BASE ADDR */
static void __iomem *reg_base_addr[DA_SEPARATE_V2_REG_CNT];

static const struct da_separate_v2_reg_page reg_page_array[] = {
	{PAGE_SoCCODEC,  DA_SEPARATE_V2_SoCCODEC_START, DA_SEPARATE_V2_SoCCODEC_END, "PAGE_SoCCODEC"},
	{PAGE_ASPCFG,    DA_SEPARATE_V2_ASPCFG_START,   DA_SEPARATE_V2_ASPCFG_END,   "PAGE_ASPCFG"},
	{PAGE_AO_IOC,    DA_SEPARATE_V2_AOIOC_START,    DA_SEPARATE_V2_AOIOC_END,    "PAGE_AO_IOC"},
#ifdef CONFIG_HISI_HI6555V500_PMU
	{PAGE_PMU_CODEC, PMUCODEC_START,                PMUCODEC_END,                "PAGE_PMU_CODEC"},
	{PAGE_PMU_CTRL,  PMUCTRL_START,                 PMUCTRL_END,                 "PAGE_PMU_CTRL"},
	{PAGE_PMU_HKADC, PMUHKADC_START,                PMUHKADC_END,                "PAGE_PMU_HKADC"},
#else
	{PAGE_PMU_CODEC, DA_SEPARATE_V2_PMUCODEC_START, DA_SEPARATE_V2_PMUCODEC_END, "PAGE_PMU_CODEC"},
#endif
	{PAGE_VIRCODEC,  DA_SEPARATE_V2_VIRCODEC_START, DA_SEPARATE_V2_VIRCODEC_END, "PAGE_VIRCODEC"},
};

static bool _reg_value_valid(struct da_separate_v2_priv *priv,
							unsigned int reg_type, unsigned int reg_value)
{
	bool is_valid = false;
	unsigned int reg_page_array_size = ARRAY_SIZE(reg_page_array);
	unsigned int i = 0;

	for (i = 0; i < reg_page_array_size; i++) {
		if ((reg_type ==reg_page_array[i].page_tag) &&
			((reg_value >= reg_page_array[i].page_reg_begin) && (reg_value <= reg_page_array[i].page_reg_end))) {
			is_valid = true;
			break;
		}
	}

	if (is_valid) {
		if ((PAGE_ASPCFG == reg_type || PAGE_SoCCODEC == reg_type) && priv->asp_pd) {
			AUDIO_LOGE("asp power down");
			is_valid = false;
		}
	}
	else {
		AUDIO_LOGE("reg_type:%d, reg_value:0x%x is invalid", reg_type, reg_value);
	}

	return is_valid;
}

static void _da_separate_v2_runtime_info_print(struct da_separate_v2_priv *priv)
{
	struct device *dev = NULL;

	WARN_ON(!priv);
	WARN_ON(!priv->codec);

	dev = priv->codec->dev;
	WARN_ON(!dev);

	AUDIO_LOGD("get suspend usage_count:0x%x child_count:0x%x status:0x%x disable_depth:%d asp_subsys_clk:%d, asp_49m_clk:%d",
		atomic_read(&(dev->power.usage_count)),
		atomic_read(&(dev->power.child_count)),
		dev->power.runtime_status,
		dev->power.disable_depth,
		clk_get_enable_count(priv->asp_subsys_clk),
		clk_get_enable_count(priv->asp_49m_clk));
}

static int _da_separate_v2_runtime_get_sync(struct da_separate_v2_priv *priv, unsigned int reg_type)
{
	bool runtime_flag = false;
	struct device *dev = NULL;
	int pm_ret = 0;

	IN_FUNCTION;

	WARN_ON(!priv);
	WARN_ON(!priv->codec);
	WARN_ON(!priv->codec->dev);

	runtime_flag = priv->pm_runtime_support && (PAGE_SoCCODEC == reg_type || PAGE_ASPCFG == reg_type);
	dev = priv->codec->dev;

	if (runtime_flag) {
		pm_ret = pm_runtime_get_sync(dev);
		AUDIO_LOGD("get pm resume  ret:%d", pm_ret);
		if (pm_ret < 0) {
			AUDIO_LOGE("pm resume error, ret:%d", pm_ret);
			return pm_ret;
		}

		_da_separate_v2_runtime_info_print(priv);
	}

	OUT_FUNCTION;
	return pm_ret;
}

static void _da_separate_v2_runtime_put_sync(struct da_separate_v2_priv *priv, unsigned int reg_type)
{
	bool runtime_flag = false;
	struct device *dev = NULL;

	IN_FUNCTION;

	WARN_ON(!priv);
	WARN_ON(!priv->codec);
	WARN_ON(!priv->codec->dev);

	runtime_flag = priv->pm_runtime_support && (PAGE_SoCCODEC == reg_type || PAGE_ASPCFG == reg_type);
	dev = priv->codec->dev;

	if (runtime_flag) {
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);

		_da_separate_v2_runtime_info_print(priv);
	}

	OUT_FUNCTION;
}

static unsigned int _da_separate_v2_reg_read(struct da_separate_v2_priv *priv, unsigned int reg)
{
	volatile unsigned int ret = 0;
	unsigned int reg_type = 0;
	unsigned int reg_value = 0;
	unsigned long flags = 0;
	int pm_ret;

	reg_type  = reg & PAGE_TYPE_MASK;
	reg_value = reg & PAGE_VALUE_MASK;

	if (!_reg_value_valid(priv, reg_type, reg_value)) {
		AUDIO_LOGE("invalid reg:0x%pK", (void *)(unsigned long)reg);
		return INVALID_REG_VALUE;
	}

	pm_ret = _da_separate_v2_runtime_get_sync(priv, reg_type);
	if (pm_ret < 0 ) {
		AUDIO_LOGE("runtime resume fail");
		return INVALID_REG_VALUE;
	}

	spin_lock_irqsave(&priv->lock, flags);

	switch (reg_type) {
	case PAGE_SoCCODEC:
		ret = readl(reg_base_addr[DA_SEPARATE_V2_SOCCODEC] + reg_value);
		AUDIO_LOGD("PAGE_SoCCODEC: offset = 0x%x, value = 0x%x", reg_value, ret);
		break;
#ifdef CONFIG_HISI_HI6555V500_PMU
	case PAGE_PMU_CODEC:
	case PAGE_PMU_CTRL:
	case PAGE_PMU_HKADC:
		ret = pmic_read_reg(reg);
		AUDIO_LOGD("PAGE_PMUCODEC: reg = 0x%x, value = 0x%x", reg, ret);
		break;
#else
	case PAGE_PMU_CODEC:
		ret = pmic_read_reg(reg_value);
		AUDIO_LOGD("PAGE_PMUCODEC: offset = 0x%x, value = 0x%x", reg_value, ret);
		break;
#endif
	case PAGE_ASPCFG:
		ret = readl(reg_base_addr[DA_SEPARATE_V2_ASPCFG] + reg_value);
		AUDIO_LOGD("PAGE_ASPCFG: offset = 0x%x, value = 0x%x", reg_value, ret);
		break;
	case PAGE_AO_IOC:
		ret = readl(reg_base_addr[DA_SEPARATE_V2_AOIOC] + reg_value);
		AUDIO_LOGD("PAGE_AO_IOC: offset = 0x%x, value = 0x%x", reg_value, ret);
		break;
	case PAGE_VIRCODEC:
		ret = priv->v_codec_reg[reg_value];
		AUDIO_LOGD("PAGE_VIRCODEC: offset = 0x%x, ret = 0x%x", reg_value, ret);
		break;
	default:
		AUDIO_LOGE("reg=0x%pK", (void *)(unsigned long)reg);
		ret = INVALID_REG_VALUE;
		break;
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	_da_separate_v2_runtime_put_sync(priv, reg_type);

	return ret;
}

static void _da_separate_v2_reg_write(struct da_separate_v2_priv *priv, unsigned int reg, unsigned int value)
{
	unsigned int reg_type = 0;
	unsigned int reg_value = 0;
	unsigned long flags = 0;
	int ret = 0;

	reg_type  = reg & PAGE_TYPE_MASK;
	reg_value = reg & PAGE_VALUE_MASK;

	if (!_reg_value_valid(priv, reg_type, reg_value)) {
		AUDIO_LOGE("invalid reg:0x%pK, value:%d", (void *)(unsigned long)reg, value);
		return;
	}

	ret = _da_separate_v2_runtime_get_sync(priv, reg_type);
	if (ret < 0 ) {
		AUDIO_LOGE("runtime resume fail");
		return;
	}

	spin_lock_irqsave(&priv->lock, flags);

#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
	hicodec_debug_reg_rw_cache(reg, value, HICODEC_DEBUG_FLAG_WRITE);
#endif

	switch (reg_type) {
	case PAGE_SoCCODEC:
		AUDIO_LOGD("PAGE_SoCCODEC: offset = 0x%x, value = 0x%x", reg_value, value);
		writel(value, reg_base_addr[DA_SEPARATE_V2_SOCCODEC] + reg_value);
		break;
#ifdef CONFIG_HISI_HI6555V500_PMU
	case PAGE_PMU_CODEC:
	case PAGE_PMU_CTRL:
	case PAGE_PMU_HKADC:
		AUDIO_LOGD("PAGE_PMUCODEC: reg = 0x%x, value = 0x%x", reg, value);
		pmic_write_reg(reg, value);
		break;
#else
	case PAGE_PMU_CODEC:
		AUDIO_LOGD("PAGE_PMUCODEC: offset = 0x%x, value = 0x%x", reg_value, value);
		pmic_write_reg(reg_value, value);
		break;
#endif
	case PAGE_ASPCFG:
		AUDIO_LOGD("PAGE_ASPCFG: offset = 0x%x, value = 0x%x", reg_value, value);
		writel(value, reg_base_addr[DA_SEPARATE_V2_ASPCFG] + reg_value);
		break;
	case PAGE_AO_IOC:
		AUDIO_LOGD("PAGE_AO_IOC: offset = 0x%x, value = 0x%x", reg_value, value);
		writel(value, reg_base_addr[DA_SEPARATE_V2_AOIOC] + reg_value);
		break;
	case PAGE_VIRCODEC:
		AUDIO_LOGD("PAGE_VIRCODEC: offset = 0x%x, value = 0x%x", reg_value, value);
		priv->v_codec_reg[reg_value] = value;
		break;
	default:
		AUDIO_LOGE("reg=0x%pK, value=0x%x", (void *)(unsigned long)reg, value);
		break;
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	_da_separate_v2_runtime_put_sync(priv, reg_type);
}

int da_separate_v2_base_addr_map(struct platform_device *pdev)
{
	struct resource *res = NULL;
	unsigned int i;

	IN_FUNCTION;

	if (!pdev) {
		return -EINVAL;
	}

	for (i = 0; i < DA_SEPARATE_V2_REG_CNT; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			AUDIO_LOGE("platform_get_resource %d err", i);
			return -ENOENT;
		}

		reg_base_addr[i] = (char * __force)(ioremap(res->start, resource_size(res)));
		if (!reg_base_addr[i]) {
			AUDIO_LOGE("cannot map register memory");
			return -ENOMEM;
		}
	}

	OUT_FUNCTION;

	return 0;
}

void da_separate_v2_base_addr_unmap(void)
{
	unsigned int i;

	IN_FUNCTION;
	for (i = 0; i < DA_SEPARATE_V2_REG_CNT; i++) {
		if (reg_base_addr[i]) {
			iounmap(reg_base_addr[i]);
			reg_base_addr[i] = NULL;
		}
	}
	OUT_FUNCTION;
}

unsigned int da_separate_v2_reg_read(struct snd_soc_component *codec, unsigned int reg)
{
	volatile unsigned int ret;
	struct da_separate_v2_priv *priv = NULL;

	if (!codec) {
		AUDIO_LOGE("parameter is null");
		return INVALID_REG_VALUE;
	}

	priv = snd_soc_component_get_drvdata(codec);
	if (!priv) {
		AUDIO_LOGE("priv is null");
		return INVALID_REG_VALUE;
	}

	ret = _da_separate_v2_reg_read(priv, reg);
	if (INVALID_REG_VALUE == ret)
		AUDIO_LOGE("reg 0x%pK read value 0x%x is invalid", (void *)(unsigned long)reg, ret);

	return ret;
}

int da_separate_v2_reg_write(struct snd_soc_component *codec, unsigned int reg, unsigned int value)
{
	int ret = 0;
	struct da_separate_v2_priv *priv = NULL;

	if (!codec) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	priv = snd_soc_component_get_drvdata(codec);
	if (!priv) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	_da_separate_v2_reg_write(priv, reg, value);

	return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
unsigned int da_separate_v2_reg_read_by_codec(struct snd_soc_codec *codec, unsigned int reg)
{
	return da_separate_v2_reg_read(&codec->component, reg);
}

int da_separate_v2_reg_write_by_codec(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	return da_separate_v2_reg_write(&codec->component, reg, value);
}
#endif

int da_separate_v2_reg_update(unsigned int reg, unsigned int mask, unsigned int value)
{
	int change;
	unsigned int old, new;

	struct snd_soc_component *codec = da_separate_v2_get_codec();
	if (!codec) {
		AUDIO_LOGE("parameter is NULL");
		return -EIO;
	}

	old = da_separate_v2_reg_read(codec, reg);
	new = (old & ~mask) | (value & mask);
	change = old != new;
	if (change)
		da_separate_v2_reg_write(codec, reg, new);

	return change;
}

void da_separate_v2_set_reg_bits(unsigned int reg, unsigned int value)
{
	unsigned int val = 0;
	struct snd_soc_component *codec = da_separate_v2_get_codec();
	if (!codec) {
		AUDIO_LOGE("parameter is NULL");
		return;
	}

	val = da_separate_v2_reg_read(codec, reg) | (value);
	da_separate_v2_reg_write(codec, reg, val);
}

void da_separate_v2_clr_reg_bits(unsigned int reg, unsigned int value)
{
	unsigned int val = 0;
	struct snd_soc_component *codec = da_separate_v2_get_codec();

	if (!codec) {
		AUDIO_LOGE("parameter is NULL");
		return;
	}

	val = da_separate_v2_reg_read(codec, reg) & ~(value);
	da_separate_v2_reg_write(codec, reg, val);
}

