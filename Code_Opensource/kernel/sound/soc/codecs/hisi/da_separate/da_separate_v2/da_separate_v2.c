/*
 * da_separate_v2_driver codec driver.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/hisi/audio_log.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/hisi/drv_pmic_if.h>
#include <soc_crgperiph_interface.h>
#include <linux/version.h>
#include <linux/power/hisi_hi6521_charger_power.h>

#include "asoc_adapter.h"
#include "da_separate_v2.h"
#include "da_separate_mbhc.h"
#include "da_separate_v2_asp_reg_def.h"
#ifdef CONFIG_HISI_HI6555V500_PMU
#include "da_separate/da_separate_v5_pmu_reg_def.h"
#else
#include "da_separate_v2_pmu_reg_def.h"
#endif
#include "da_separate_v2_i2s.h"

#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
#include "da_separate_v2_debug.h"
#endif
/*lint -e548*/
#define LOG_TAG "da_separate_v2"


#define DA_SEPARATE_V2_CODEC_NAME "da_separate_v2-codec"

static struct snd_soc_component *soc_codec = NULL;

#define HAC_ENABLE                   1
#define GPIO_LEVEL_HIGH              1
#define GPIO_LEVEL_LOW               0
#define PGA_GAIN_STEP                200 /* unit:0.01db */
#define CLEAR_FIFO_DELAY_LEN_MS      1

static unsigned int hac_en_gpio = ARCH_NR_GPIOS;
static int hac_switch = 0;

static int hac_gpio_output_set(int hac_cmd)
{
	int ret;
	if (!gpio_is_valid(hac_en_gpio)) {
		AUDIO_LOGE("Failed to get the hac gpio\n");
		return -EFAULT;
	}

	if (HAC_ENABLE == hac_cmd) {
		AUDIO_LOGI("Enable hac gpio %u\n", hac_en_gpio);
		ret = gpio_direction_output(hac_en_gpio, GPIO_LEVEL_HIGH);
	} else {
		AUDIO_LOGI("Disable hac gpio %u\n", hac_en_gpio);
		ret = gpio_direction_output(hac_en_gpio, GPIO_LEVEL_LOW);
	}

	return ret;
}

static const char * const hac_switch_text[] = {"OFF", "ON"};

static const struct soc_enum hac_switch_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hac_switch_text), hac_switch_text),
};

static int hac_switch_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	if (NULL == kcontrol || NULL == ucontrol) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	ucontrol->value.integer.value[0] = hac_switch;

	return 0;
}

static int hac_switch_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int ret;

	if (NULL == kcontrol || NULL == ucontrol) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	hac_switch = ucontrol->value.integer.value[0];
	ret = hac_gpio_output_set(hac_switch);

	return ret;
}

static int switch_hs_gpio(int hs_cmd)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	if (priv == NULL) {
		AUDIO_LOGE("priv is null pointer\n");
		return -EFAULT;
	}

	if (!gpio_is_valid(priv->hs_en_gpio)) {
		AUDIO_LOGE("Failed to get the headset enable gpio\n");
		return -EFAULT;
	}

	gpio_set_value(priv->hs_en_gpio, hs_cmd);

	return 0;
}

static const char * const hs_switch_text[] = {"ON", "OFF"};

static const struct soc_enum hs_switch_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(hs_switch_text), hs_switch_text),
};

static int hs_switch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	if (kcontrol == NULL || ucontrol == NULL || priv == NULL) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	ucontrol->value.integer.value[0] = priv->hs_en_value;

	return 0;
}

static int hs_switch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	if (kcontrol == NULL || ucontrol == NULL || priv == NULL) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	priv->hs_en_value = ucontrol->value.integer.value[0];

	ret = switch_hs_gpio(priv->hs_en_value);
	if (ret != 0)
		AUDIO_LOGE("switch headset enable gpio failed\n");

	return ret;
}

static enum da_separate_v2_classd_supply_type _get_classd_supply_type(struct snd_soc_component *codec)
{
	unsigned int value = CLASSD_SUPPLY_INVALID;
	struct device_node *np = NULL;
	WARN_ON(!codec->dev);
	WARN_ON(!codec->dev->of_node);
	np = codec->dev->of_node;

	IN_FUNCTION;

	if (of_property_read_u32(np, "classd_supply_type", &value)) {
		AUDIO_LOGW("can not find classd_supply_type,pls check if not use classd\n");
		return CLASSD_SUPPLY_INVALID;
	}

	if (value >= CLASSD_SUPPLY_INVALID) {
		AUDIO_LOGE("find invalied classd_supply_type %u\n", value);
		return CLASSD_SUPPLY_INVALID;
	}

	AUDIO_LOGI("classd_supply_type is %u\n", value);

	OUT_FUNCTION;
	return value;
}

static int classd_schg_voltage_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *codec = NULL;
	struct da_separate_v2_priv *priv = NULL;

	IN_FUNCTION;
	if (!kcontrol || !ucontrol) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	codec = snd_soc_kcontrol_component(kcontrol);
	WARN_ON(!codec);
	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	ucontrol->value.integer.value[0] = priv->classd_scharger_voltage;

	OUT_FUNCTION;
	return 0;
}

static int classd_schg_voltage_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int ret;
	int classd_voltage;
	struct snd_soc_component *codec = NULL;
	struct da_separate_v2_priv *priv = NULL;

	IN_FUNCTION;
	if (!kcontrol || !ucontrol) {
		AUDIO_LOGE("input pointer is null\n");
		return -EFAULT;
	}

	codec = snd_soc_kcontrol_component(kcontrol);
	WARN_ON(!codec);
	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	if (CLASSD_SUPPLY_SCHARGER != priv->classd_supply_type) {
		AUDIO_LOGE("class supply type is not sharger, CLASSD_SCHG_VOLTAGE_CONFIG should not exist in path xml\n");
		return -ENXIO;
	}

	/* The default voltage is 4.5,set by pmu.
	 * And here set range 4.5~5.2
	 */
	classd_voltage = ucontrol->value.integer.value[0];
	if ((classd_voltage % CLASSD_VOLTAGE_DIV)
		|| (classd_voltage < CLASSD_MIN_VOLTAGE)
		|| (classd_voltage > CLASSD_MAX_VOLTAGE)){
		AUDIO_LOGE("classd voltage config:%d in xml is not support\n", classd_voltage);
		return -EINVAL;
	}

	ret = scharger_flash_bst_vo_config(classd_voltage);
	if (ret) {
		AUDIO_LOGI("classd voltage config:%d fail:%d\n", classd_voltage, ret);
		return ret;
	}

	priv->classd_scharger_voltage = classd_voltage;
	AUDIO_LOGI("classd voltage config:%d ok\n", classd_voltage);

	OUT_FUNCTION;
	return ret;
}

/* MAINPGA GAIN(MAINPGA_GAIN<2:0>)£»
 * range:0dB~14dB¡£
 * step:2dB¡£
 * 000: 0dB£»
 * 001: 2dB£»
 * 010: 4dB£»
 * 011: 6dB£»
 * 100: 8dB£»
 * 101: 10dB£»
 * 110: 12dB£»
 * 111: 14dB£» */
static DECLARE_TLV_DB_SCALE(da_separate_v2_ana_mainpga_gain_tlv, 0, PGA_GAIN_STEP, 0);

/* AUXPGA GAIN(AUXPGA_GAIN<2:0>)£»
 * range:0dB~14dB¡£
 * step:2dB¡£
 * 000: 0dB£»
 * 001: 2dB£»
 * 010: 4dB£»
 * 011: 6dB£»
 * 100: 8dB£»
 * 101: 10dB£»
 * 110: 12dB£»
 * 111: 14dB£» */
static DECLARE_TLV_DB_SCALE(da_separate_v2_ana_auxpga_gain_tlv, 0, PGA_GAIN_STEP, 0);

/* MIC3PGA GAIN(MIC3PGA_GAIN<2:0>)£»
 * range:0dB~14dB¡£
 * step:2dB¡£
 * 000: 0dB£»
 * 001: 2dB£»
 * 010: 4dB£»
 * 011: 6dB£»
 * 100: 8dB£»
 * 101: 10dB£»
 * 110: 12dB£»
 * 111: 14dB£» */
static DECLARE_TLV_DB_SCALE(da_separate_v2_ana_mic3pga_gain_tlv, 0, PGA_GAIN_STEP, 0);

#define DA_SEPARATE_V2_FIFO_FS_KCONTROLS	\
/*{*/																							\
	SOC_SINGLE("FS_3RDCODEC_L_DLINK", 	FS_CTRL2_REG, FS_CODEC3_L_DLINK_OFFSET, 	0x7, 0),	\
	SOC_SINGLE("FS_3RDCODEC_R_DLINK", 	FS_CTRL2_REG, FS_CODEC3_R_DLINK_OFFSET,		0x7, 0),	\
	SOC_SINGLE("FS_VOICE_L_DLINK",		FS_CTRL0_REG, FS_VOICE_L_DLINK_OFFSET,		0x3, 0),	\
	SOC_SINGLE("FS_VOICE_R_DLINK",		FS_CTRL2_REG, FS_VOICE_R_DLINK_OFFSET, 		0x3, 0),	\
	SOC_SINGLE("FS_AUDIO_L_UPLINK",		FS_CTRL0_REG, FS_AUDIO_L_UPLINK_OFFSET, 	0x1, 0),	\
	SOC_SINGLE("FS_AUDIO_R_UPLINK",		FS_CTRL0_REG, FS_AUDIO_R_UPLINK_OFFSET, 	0x1, 0),	\
	SOC_SINGLE("FS_VOICE_L_UPLINK",		FS_CTRL0_REG, FS_VOICE_L_UPLINK_OFFSET, 	0x3, 0),	\
	SOC_SINGLE("FS_VOICE_R_UPLINK", 	FS_CTRL0_REG, FS_VOICE_R_UPLINK_OFFSET, 	0x3, 0),	\
	SOC_SINGLE("FS_MIC3_UPLINK", 		FS_CTRL0_REG, FS_MIC3_UPLINK_OFFSET, 		0x7, 0),	\
	SOC_SINGLE("FS_I2S2", 				I2S2_PCM_CTRL_REG, 	FS_I2S2_OFFSET, 	 	0x7, 0), 	\
	SOC_SINGLE("FS_ECHO_L_UPLINK",		FS_CTRL0_REG, FS_ECHO_L_UPLINK_OFFSET, 		0x7, 0),	\
	SOC_SINGLE("FS_ECHO_R_UPLINK",		FS_CTRL0_REG, FS_ECHO_R_UPLINK_OFFSET, 		0x7, 0)		\
/*}*/																							\

#define DA_SEPARATE_V2_VOICE_AUDIO_DL_PGA_KCONTROLS	\
/*{*/																								 \
	SOC_SINGLE("VOICE_L_DN_PGA_GAIN", 	VOICE_L_DN_PGA_CTRL_REG, VOICE_L_DN_PGA_GAIN_OFFSET, 	0xff, 0),\
	SOC_SINGLE("VOICE_L_DN_PGA_BYPASS", VOICE_L_DN_PGA_CTRL_REG, VOICE_L_DN_PGA_BYPASS_OFFSET,	0x1,  0),\
	SOC_SINGLE("VOICE_R_DN_PGA_GAIN", 	VOICE_R_DN_PGA_CTRL_REG, VOICE_R_DN_PGA_GAIN_OFFSET, 	0xff, 0),\
	SOC_SINGLE("VOICE_R_DN_PGA_BYPASS", VOICE_R_DN_PGA_CTRL_REG, VOICE_R_DN_PGA_BYPASS_OFFSET,	0x1,  0),\
	SOC_SINGLE("AUDIO_L_DN_PGA_GAIN", 	AUDIO_L_DN_PGA_CTRL_REG, AUDIO_L_DN_PGA_GAIN_OFFSET, 	0xff, 0),\
	SOC_SINGLE("AUDIO_L_DN_PGA_BYPASS", AUDIO_L_DN_PGA_CTRL_REG, AUDIO_L_DN_PGA_BYPASS_OFFSET,	0x1,  0),\
	SOC_SINGLE("AUDIO_R_DN_PGA_GAIN", 	AUDIO_R_DN_PGA_CTRL_REG, AUDIO_R_DN_PGA_GAIN_OFFSET, 	0xff, 0),\
	SOC_SINGLE("AUDIO_R_DN_PGA_BYPASS", AUDIO_R_DN_PGA_CTRL_REG, AUDIO_R_DN_PGA_BYPASS_OFFSET,	0x1,  0),\
	SOC_SINGLE("SIDETONE_PGA_BYPASS",	SIDETONE_PGA_CTRL_REG, 	 SIDETONE_PGA_BYPASS_OFFSET, 	0x1,  0),\
	SOC_SINGLE("SIDETONE_PGA",			SIDETONE_PGA_CTRL_REG, 	 SIDETONE_PGA_GAIN_OFFSET, 		0xff, 0) \
/*}*/																								 \

#define DA_SEPARATE_V2_I2S_KCONTROLS	\
/*{*/																						     \
	SOC_SINGLE("I2S1_TX_CLK_SEL", 	  I2S1_TDM_CTRL0_REG, 	 I2S1_TX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S1_RX_CLK_SEL", 	  I2S1_TDM_CTRL0_REG, 	 I2S1_RX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S1_DIRECT_LOOP",	  I2S1_TDM_CTRL0_REG,	 I2S1_DIRECT_LOOP_OFFSET,   0x3, 0), \
	SOC_SINGLE("I2S2_TX_CLK_SEL", 	  I2S2_PCM_CTRL_REG, 	 I2S2_TX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S2_RX_CLK_SEL", 	  I2S2_PCM_CTRL_REG, 	 I2S2_RX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S1_FUNC_MODE", 	  I2S1_TDM_CTRL0_REG, 	 I2S1_FUNC_MODE_OFFSET,     0x7, 0), \
	SOC_SINGLE("I2S1_FRAME_MODE", 	  I2S1_TDM_CTRL0_REG, 	 I2S1_FRAME_MODE_OFFSET,     0x1, 0), \
	SOC_SINGLE("I2S1_CODEC_IO_WORDLENGTH", 	  I2S1_TDM_CTRL0_REG, 	 I2S1_CODEC_IO_WORDLENGTH_OFFSET,     0x3, 0), \
	SOC_SINGLE("I2S2_FUNC_MODE", 	  I2S2_PCM_CTRL_REG, 	 I2S2_FUNC_MODE_OFFSET,     0x7, 0), \
	SOC_SINGLE("I2S2_FRAME_MODE", 	  I2S2_PCM_CTRL_REG, 	 I2S2_FRAME_MODE_OFFSET,     0x1, 0), \
	SOC_SINGLE("I2S2_DIRECT_LOOP",	  I2S2_PCM_CTRL_REG,	 I2S2_DIRECT_LOOP_OFFSET,	0x3, 0), \
	SOC_SINGLE("I2S2_CODEC_IO_WORDLENGTH", 	  I2S2_PCM_CTRL_REG, 	 I2S2_CODEC_IO_WORDLENGTH_OFFSET,     0x3, 0), \
	SOC_SINGLE("TX_MIXER2_GAIN1_PGA", BT_TX_MIXER2_CTRL_REG, BT_TX_MIXER2_GAIN1_OFFSET, 0x3, 0), \
	SOC_SINGLE("TX_MIXER2_GAIN2_PGA", BT_TX_MIXER2_CTRL_REG, BT_TX_MIXER2_GAIN2_OFFSET, 0x3, 0), \
	SOC_SINGLE("BT_L_RX_PGA_GAIN",    BT_L_RX_PGA_CTRL_REG,  BT_L_RX_PGA_GAIN_OFFSET,   0xff, 0), \
	SOC_SINGLE("BT_R_RX_PGA_GAIN",    BT_R_RX_PGA_CTRL_REG,  BT_R_RX_PGA_GAIN_OFFSET,   0xff, 0), \
	SOC_SINGLE("I2S3_MST_SLV",        I2S3_PCM_CTRL_REG,     I2S3_MST_SLV_OFFSET,       0x1, 0), \
	SOC_SINGLE("I2S3", 				  I2S3_PCM_CTRL_REG, 	 FS_I2S3_OFFSET, 			0x7, 0),  \
	SOC_SINGLE("I2S3_DIRECT_LOOP",	  I2S3_PCM_CTRL_REG,	 I2S3_DIRECT_LOOP_OFFSET,   0x3, 0),  \
	SOC_SINGLE("I2S3_TX_CLK_SEL", 	  I2S3_PCM_CTRL_REG, 	 I2S3_TX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S3_RX_CLK_SEL",     I2S3_PCM_CTRL_REG,     I2S3_RX_CLK_SEL_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S3_FRAME_MODE",     I2S3_PCM_CTRL_REG,	 I2S3_FRAME_MODE_OFFSET,    0x1, 0), \
	SOC_SINGLE("I2S3_CODEC_IO_WORDLENGTH", 	  I2S3_PCM_CTRL_REG, 	 I2S3_CODEC_IO_WORDLENGTH_OFFSET,     0x3, 0) \
/*}*/																						     \

#define DA_SEPARATE_V2_SRC_KCONTROLS	\
/*{*/																						         \
	SOC_SINGLE("VOICE_L_DN_SRCUP_SRC1_MODE",SRCUP_CTRL_REG, VOICE_L_DN_SRCUP_SRC_MODE_OFFSET,	0x7, 0), \
	SOC_SINGLE("DACL_SRC2_MODE", 			SRCUP_CTRL_REG, DACL_SRCUP_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("DACL_SRC2", 				FS_CTRL0_REG, 	FS_DACL_SRCUP_IN_OFFSET, 		 	0x3, 0), \
	SOC_SINGLE("VOICE_R_DN_SRCUP_SRC3_MODE",SRCUP_CTRL_REG, VOICE_R_DN_SRCUP_SRC_MODE_OFFSET, 	0x7, 0), \
	SOC_SINGLE("DACR_SRC4_MODE", 			SRCUP_CTRL_REG, DACR_SRCUP_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("DACR_SRC4", 				FS_CTRL0_REG, 	FS_DACR_SRCUP_IN_OFFSET, 		 	0x3, 0), \
	SOC_SINGLE("AUDIO_L_UP_SRC5_MODE", 		SRCUP_CTRL_REG, AUDIO_L_UP_SRCUP_SRC_MODE_OFFSET, 	0x7, 0), \
	SOC_SINGLE("AUDIO_L_UP_SRC5", 			FS_CTRL0_REG, 	FS_AUDIO_L_UP_SRCUP_IN_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("AUDIO_R_UP_SRC6_MODE",		SRCUP_CTRL_REG, AUDIO_R_UP_SRCUP_SRC_MODE_OFFSET, 	0x7, 0), \
	SOC_SINGLE("AUDIO_R_UP_SRC6", 			FS_CTRL0_REG, 	FS_AUDIO_R_UP_SRCUP_IN_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("VOICE_L_UP_SRC7_MODE",		SRCDN_CTRL_REG, VOICE_L_UP_SRCDN_SRC_MODE_OFFSET, 	0x7, 0), \
	SOC_SINGLE("VOICE_L_UP_SRC7", 			FS_CTRL1_REG, 	FS_VOICE_L_UP_SRCDN_OUT_OFFSET, 	0x3, 0), \
	SOC_SINGLE("VOICE_R_UP_SRC8_MODE",		SRCDN_CTRL_REG, VOICE_R_UP_SRCDN_SRC_MODE_OFFSET, 	0x7, 0), \
	SOC_SINGLE("VOICE_R_UP_SRC8", 			FS_CTRL1_REG, 	FS_VOICE_R_UP_SRCDN_OUT_OFFSET, 	0x3, 0), \
	SOC_SINGLE("MIC3_UP_SRC9_MODE", 		SRCDN_CTRL_REG, MIC3_UP_SRCDN_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("MIC3_UP_SRC9",				FS_CTRL1_REG, 	FS_MIC3_UP_SRCDN_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("ECHO_L_UP_SRC10_MODE",		SRCDN_CTRL_REG, ECHO_L_UP_SRCDN_SRC_MODE_OFFSET,  	0x7, 0), \
	SOC_SINGLE("ECHO_L_UP_SRC10", 			FS_CTRL1_REG, 	FS_ECHO_L_UP_SRCDN_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("ECHO_R_UP_SRC11_MODE",		SRCDN_CTRL_REG, ECHO_R_UP_SRCDN_SRC_MODE_OFFSET,  	0x7, 0), \
	SOC_SINGLE("ECHO_R_UP_SRC11", 			FS_CTRL1_REG, 	FS_ECHO_R_UP_SRCDN_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("BT_L_RX_SRCUP_SRC12_MODE",	SRCUP_CTRL_REG, BT_L_RX_SRCUP_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("BT_L_RX_SRCUP_IN_SRC12", 	FS_CTRL1_REG, 	FS_BT_L_RX_SRCUP_IN_OFFSET, 		0x3, 0), \
	SOC_SINGLE("BT_L_RX_SRCUP_OUT_SRC12", 	FS_CTRL1_REG, 	FS_BT_L_RX_SRCUP_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("BT_R_RX_SRCDN_SRC13_MODE",	SRCUP_CTRL_REG, BT_R_RX_SRCUP_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("BT_R_RX_SRCUP_IN_SRC13", 	FS_CTRL1_REG, 	FS_BT_R_RX_SRCUP_IN_OFFSET, 		0x3, 0), \
	SOC_SINGLE("BT_R_RX_SRCUP_OUT_SRC13", 	FS_CTRL1_REG, 	FS_BT_R_RX_SRCUP_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("BT_L_RX_SRCDN_SRC14_MODE",	SRCDN_CTRL_REG, BT_L_RX_SRCDN_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("BT_L_RX_SRCDN_IN_SRC14", 	FS_CTRL1_REG, 	FS_BT_L_RX_SRCDN_IN_OFFSET, 		0x3, 0), \
	SOC_SINGLE("BT_R_RX_SRCUP_OUT_SRC14", 	FS_CTRL1_REG, 	FS_BT_L_RX_SRCDN_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("BT_R_RX_SRCDN_SRC15_MODE",	SRCDN_CTRL_REG, BT_R_RX_SRCDN_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("BT_R_RX_SRCDN_IN_SRC15", 	FS_CTRL1_REG, 	FS_BT_R_RX_SRCDN_IN_OFFSET, 		0x3, 0), \
	SOC_SINGLE("BT_R_RX_SRCDN_OUT_SRC15", 	FS_CTRL1_REG, 	FS_BT_R_RX_SRCDN_OUT_OFFSET, 	 	0x3, 0), \
	SOC_SINGLE("BT_TX_SRCDN_S2_SRC_MODE", 	SRCDN_CTRL_REG, BT_TX_SRCDN_SRC_MODE_OFFSET, 	 	0x7, 0), \
	SOC_SINGLE("BT_TX_SRCDN_OUT_S2_SRC", 	FS_CTRL1_REG, 	FS_BT_TX_SRCDN_OUT_OFFSET, 		 	0x3, 0)  \
/*}*/																							         \

#define DA_SEPARATE_V2_ADC_PGA_KCONTROLS	\
/*{*/								    														         \
	SOC_SINGLE("AUDIO_L_UP_PGA_GAIN", 	AUDIO_L_UP_PGA_CTRL_REG, AUDIO_L_UP_PGA_GAIN_OFFSET,   0xff, 0), \
	SOC_SINGLE("AUDIO_L_UP_PGA_BYPASS", AUDIO_L_UP_PGA_CTRL_REG, AUDIO_L_UP_PGA_BYPASS_OFFSET, 0x1,  0), \
	SOC_SINGLE("AUDIO_R_UP_PGA_GAIN", 	AUDIO_R_UP_PGA_CTRL_REG, AUDIO_R_UP_PGA_GAIN_OFFSET,   0xff, 0), \
	SOC_SINGLE("AUDIO_R_UP_PGA_BYPASS", AUDIO_R_UP_PGA_CTRL_REG, AUDIO_R_UP_PGA_BYPASS_OFFSET, 0x1,  0), \
	SOC_SINGLE("MIC3_UP_PGA_GAIN",		MIC3_UP_PGA_CTRL_REG, 	 MIC3_UP_PGA_GAIN_OFFSET, 	   0xff,  0),\
	SOC_SINGLE("MIC3_UP_PGA_BYPASS", 	MIC3_UP_PGA_CTRL_REG, 	 MIC3_UP_PGA_BYPASS_OFFSET,    0x1,  0)  \
/*}*/																								     \

#define DA_SEPARATE_V2_ADC_FILTER_KCONTROLS	\
/*{*/								    												    \
	SOC_SINGLE("ADCL_CIC_GAIN", 	ADC_FILTER_CTRL_REG, ADCL_CIC_GAIN_OFFSET,    0x3f, 0), \
	SOC_SINGLE("ADCR_CIC_GAIN", 	ADC_FILTER_CTRL_REG, ADCR_CIC_GAIN_OFFSET,    0x3f, 0), \
	SOC_SINGLE("ADC_MIC3_CIC_GAIN", ADC_FILTER_CTRL_REG, ADC_MIC3_CIC_GAIN_OFFSET,0x3f, 0)  \
/*}*/																						\

#define DA_SEPARATE_V2_DAC_PGA_KCONTROLS	\
/*{*/																						    \
	SOC_SINGLE("IN1_S2_L_PGA",		  DACL_MIXER4_CTRL0_REG, DACL_MIXER4_GAIN1_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN2_VOICE_DL_PGA",	  DACL_MIXER4_CTRL0_REG, DACL_MIXER4_GAIN2_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN3_AUDIO_DL_PGA",	  DACL_MIXER4_CTRL0_REG, DACL_MIXER4_GAIN3_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN4_SIDETONE_DL_PGA", DACL_MIXER4_CTRL0_REG, DACL_MIXER4_GAIN4_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN1_S2_R_PGA",		  DACR_MIXER4_CTRL0_REG, DACR_MIXER4_GAIN1_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN2_VOICE_DR_PGA",	  DACR_MIXER4_CTRL0_REG, DACR_MIXER4_GAIN2_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN3_AUDIO_DR_PGA",	  DACR_MIXER4_CTRL0_REG, DACR_MIXER4_GAIN3_OFFSET, 0x3, 0), \
	SOC_SINGLE("IN4_SIDETONE_DR_PGA", DACR_MIXER4_CTRL0_REG, DACR_MIXER4_GAIN4_OFFSET, 0x3, 0)  \
/*}*/																							\

#define DA_SEPARATE_V2_ANA_KCONTROLS	\
/*{*/																									     \
	SOC_SINGLE("ANA_HPL_PGA_MUTE",		CODEC_ANA_RW5_REG, 			CODEC_ANA_RW_05_HPL_MUTE_OFFSET, 0x1, 0),\
	SOC_SINGLE("ANA_HPL_PGA", 			CODEC_ANA_RW21_REG, 		CODEC_HSL_GAIN_OFFSET,           0xF, 0),\
	SOC_SINGLE("ANA_HPR_PGA_MUTE",		CODEC_ANA_RW5_REG, 			CODEC_ANA_RW_05_HPR_MUTE_OFFSET, 0x1, 0),\
	SOC_SINGLE("ANA_HPR_PGA", 			CODEC_ANA_RW21_REG, 		CODEC_HSR_GAIN_OFFSET,           0xF, 0),\
	SOC_SINGLE("ANA_SPK_PGA_MUTE", 		CTRL_REG_ANA_SPK_PGA_MUTE_REG, 	CTRL_REG_CLASSD_MUTE_OFFSET,     0x1, 0),\
	SOC_SINGLE("ANA_SPK_PGA", 			CTRL_REG_ANA_SPK_PGA_REG, 	CTRL_REG_CLASSD_GAIN_OFFSET,     0x3, 0),\
	SOC_SINGLE("ANA_EAR_PGA_MUTE",		CODEC_ANA_RW6_REG, 		CODEC_EAR_MUTE_OFFSET,           0x1, 0),\
	SOC_SINGLE("ANA_EAR_PGA", 			CODEC_ANA_RW26_REG, 		CODEC_EAR_GAIN_OFFSET,           0xF, 0),\
	SOC_SINGLE("ANA_LINEOUT_PGA_MUTE",	CODEC_ANA_RW6_REG, 		CODEC_LOUT_MUTE_OFFSET,          0x1, 0),\
	SOC_SINGLE("ANA_LINEOUT_PGA", 		CODEC_ANA_LINEOUT_REG, 		CODEC_LOUT_GAIN_OFFSET,          0xF, 0),\
	SOC_SINGLE("ANA_MAINMIC_PGA_MUTE",	CODEC_ANA_RW2_REG, 		CODEC_MAINPGA_MUTE_OFFSET,       0x1, 0),\
	SOC_SINGLE("ANA_MAINPGA_BOOST",	    CODEC_ANA_RW9_REG, 		CODEC_MAINPGA_BOOST_OFFSET,      0x1, 0),\
	SOC_SINGLE_TLV("ANA_MAINMIC_PGA", 	CODEC_ANA_RW9_REG, CODEC_MAINPGA_GAIN_OFFSET, 0x7, 0, da_separate_v2_ana_mainpga_gain_tlv),\
	SOC_SINGLE("ANA_AUXMIC_PGA_MUTE", 	CODEC_ANA_RW2_REG, 		CODEC_AUXPGA_MUTE_OFFSET,        0x1, 0),\
	SOC_SINGLE("ANA_AUXPGA_BOOST",		CODEC_ANA_RW10_REG, 		CODEC_AUXPGA_BOOST_OFFSET,      0x1, 0),\
	SOC_SINGLE_TLV("ANA_AUXMIC_PGA", 	CODEC_ANA_RW10_REG, CODEC_AUXPGA_GAIN_OFFSET,  0x7, 0, da_separate_v2_ana_auxpga_gain_tlv),\
	SOC_SINGLE("ANA_MIC3_PGA_MUTE", 	CODEC_ANA_RW2_REG, 		CODEC_MIC3PGA_MUTE_OFFSET,       0x1, 0),\
	SOC_SINGLE("ANA_MIC3PGA_BOOST",	    CODEC_ANA_RW11_REG, 		CODEC_MIC3PGA_BOOST_OFFSET,      0x1, 0),\
	SOC_SINGLE_TLV("ANA_MIC3_PGA",		CODEC_ANA_RW11_REG, CODEC_MIC3PGA_GAIN_OFFSET, 0x7, 0, da_separate_v2_ana_mic3pga_gain_tlv),\
	SOC_SINGLE("ANA_MIXER_EAR_L_PGA", 	CODEC_ANA_RW25_REG, 		CODEC_CR_MIXF_DACL_PGA_EN_OFFSET,0x1, 0),\
	SOC_SINGLE("ANA_MIXER_EAR_R_PGA", 	CODEC_ANA_RW25_REG, 		CODEC_CR_MIXF_DACR_PGA_EN_OFFSET,0x1, 0) \
/*}*/																								         \

#define DA_SEPARATE_V2_CLASSD_KCONTROLS \
/*{*/ \
	SOC_SINGLE("CLASSD_P_SEL", CTRL_REG_CLASSD_P_N_SEL_REG, CTRL_REG_CLASSD_P_SEL_OFFSET, 0x3, 0),\
	SOC_SINGLE("CLASSD_N_SEL", CTRL_REG_CLASSD_P_N_SEL_REG, CTRL_REG_CLASSD_N_SEL_OFFSET, 0x3, 0)\
/*}*/

#define DA_SEPARATE_V2_CUSTOMER	\
/*{*/								    									  \
	SOC_ENUM_EXT("HAC",   hac_switch_enum[0], hac_switch_get, hac_switch_put), \
	SOC_ENUM_EXT("HS_SWITCH", hs_switch_enum[0], hs_switch_get, hs_switch_put), \
	SOC_SINGLE_EXT("CLASSD_SCHG_VOLTAGE_CONFIG", DA_SEPARATE_V2_DDR_CODEC_VIR1_ADDR, 0, 0xffff, 0, \
				classd_schg_voltage_get, classd_schg_voltage_put) \
/*}*/																	\

static const struct snd_kcontrol_new da_separate_v2_snd_controls[] = {
	DA_SEPARATE_V2_FIFO_FS_KCONTROLS,
	DA_SEPARATE_V2_VOICE_AUDIO_DL_PGA_KCONTROLS,
	DA_SEPARATE_V2_I2S_KCONTROLS,
	DA_SEPARATE_V2_SRC_KCONTROLS,
	DA_SEPARATE_V2_ADC_PGA_KCONTROLS,
	DA_SEPARATE_V2_ADC_FILTER_KCONTROLS,
	DA_SEPARATE_V2_DAC_PGA_KCONTROLS,
	DA_SEPARATE_V2_ANA_KCONTROLS,
	DA_SEPARATE_V2_CLASSD_KCONTROLS,
	DA_SEPARATE_V2_CUSTOMER,
};

static int da_separate_v2_audio_dl_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* pga/fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				(1 << AUDIO_L_DN_PGA_CLKEN_OFFSET)|(1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET));

		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				(1 << AUDIO_L_DN_PGA_CLKEN_OFFSET) | (1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_audio_dr_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* pga/fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				(1 << AUDIO_R_DN_PGA_CLKEN_OFFSET)|(1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo/pga clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				(1 << AUDIO_R_DN_PGA_CLKEN_OFFSET) | (1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}
static int da_separate_v2_side_tone_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* pga/fifo/src clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG, (1 << SIDETONE_PGA_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, (1 << SIDETONE_PGA_CLKEN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;
	return 0;
}

static int da_separate_v2_voice_dl_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_L_DN_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* pga/fifo/src clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
							(1 << VOICE_L_DN_PGA_CLKEN_OFFSET)
						  | (1 << VOICE_L_DN_AFIFO_CLKEN_OFFSET)
						  | (1 << VOICE_L_DN_SRCUP_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
							(1 << VOICE_L_DN_PGA_CLKEN_OFFSET)
						  | (1 << VOICE_L_DN_AFIFO_CLKEN_OFFSET)
						  | (1 << VOICE_L_DN_SRCUP_CLKEN_OFFSET));

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_voice_dr_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, 1 << VOICE_R_DN_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* pga/fifo/src clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
							(1 << VOICE_R_DN_PGA_CLKEN_OFFSET)
						  | (1 << VOICE_R_DN_AFIFO_CLKEN_OFFSET)
						  | (1 << VOICE_R_DN_SRCUP_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
							(1 << VOICE_R_DN_PGA_CLKEN_OFFSET)
						  | (1 << VOICE_R_DN_AFIFO_CLKEN_OFFSET)
						  | (1 << VOICE_R_DN_SRCUP_CLKEN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_audio_ul_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_audio_ur_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}
static int da_separate_v2_voice_ul_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_voice_ur_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}


static int da_separate_v2_mic3_up_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << MIC3_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG, 1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG, 1 << MIC3_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_echo_l_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_L_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_L_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_L_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_echo_r_fifo_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* clear fifo */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_R_UP_AFIFO_CLKEN_OFFSET);

		udelay(CLEAR_FIFO_DELAY_LEN_MS);

		/* fifo clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_R_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* fifo clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, 1 << ECHO_R_UP_AFIFO_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}


static int da_separate_v2_adc_mic3_filter_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* filter/pga clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADC_MIC3_FILTER_CLKEN_OFFSET)
						   |(1 << MIC3_UP_PGA_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* filter/pga clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADC_MIC3_FILTER_CLKEN_OFFSET)
						   |(1 << MIC3_UP_PGA_CLKEN_OFFSET));

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_adcl_filter_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* filter/pga clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADCL_FILTER_CLKEN_OFFSET)
						   |(1 << AUDIO_L_UP_PGA_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* filter/pga clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADCL_FILTER_CLKEN_OFFSET)
						   |(1 << AUDIO_L_UP_PGA_CLKEN_OFFSET));

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_adcr_filter_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* filter/pga clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADCR_FILTER_CLKEN_OFFSET)
						   |(1 << AUDIO_R_UP_PGA_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* filter/pga clk  disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
							(1 << ADCR_FILTER_CLKEN_OFFSET)
						   |(1 << AUDIO_R_UP_PGA_CLKEN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}
static int da_separate_v2_dacl_filter_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* src/filter/src16/sdm clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
							(1 << DACL_SRCUP_CLKEN_OFFSET)
						   |(1 << DACL_FILTER_CLKEN_OFFSET)
						   |(1 << DACL_UP16_CLKEN_OFFSET)
						   |(1 << SDM_L_CLKEN_OFFSET));

		/* sif smt dacl enable */
		da_separate_v2_set_reg_bits(DAC_FILTER_CTRL_REG,
							 (1 << SIF_MST_DACL_EN_OFFSET)
							|(1 << SDM_L_CALT_VLD_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* src/filter/src16/sdm clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
							(1 << DACL_SRCUP_CLKEN_OFFSET)
						   |(1 << DACL_FILTER_CLKEN_OFFSET)
						   |(1 << DACL_UP16_CLKEN_OFFSET)
						   |(1 << SDM_L_CLKEN_OFFSET));

		/* sif smt dacl disable */
		da_separate_v2_clr_reg_bits(DAC_FILTER_CTRL_REG,
							 (1 << SIF_MST_DACL_EN_OFFSET)
							|(1 << SDM_L_CALT_VLD_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_dacr_filter_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* src/filter/src16/sdm clk enable */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
							(1 << DACR_SRCUP_CLKEN_OFFSET)
						   |(1 << DACR_FILTER_CLKEN_OFFSET)
						   |(1 << DACR_UP16_CLKEN_OFFSET)
						   |(1 << SDM_R_CLKEN_OFFSET));/*lint !e648*/

		/* sif smt dacr enable */
		da_separate_v2_set_reg_bits(DAC_FILTER_CTRL_REG,
							 (1 << SIF_MST_DACR_EN_OFFSET)
							|(1 << SDM_R_CALT_VLD_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* src/filter/src16/sdm clk disable */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
							(1 << DACR_SRCUP_CLKEN_OFFSET)
						   |(1 << DACR_FILTER_CLKEN_OFFSET)
						   |(1 << DACR_UP16_CLKEN_OFFSET)
						   |(1 << SDM_R_CLKEN_OFFSET));/*lint !e648*/

		/* sif smt dacr disable */
		da_separate_v2_clr_reg_bits(DAC_FILTER_CTRL_REG,
							 (1 << SIF_MST_DACR_EN_OFFSET)
							|(1 << SDM_R_CALT_VLD_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_ana_dacl_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* dem/dacl clk enable */
		da_separate_v2_set_reg_bits(DAC_CHAN_CTRL_REG,
							(1 << DACL_DWA_EN_OFFSET));

		da_separate_v2_clr_reg_bits(CODEC_ANA_RW4_REG,
							(1 << CODEC_ANA_RW_04_DAPL_PD_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* dem/dacl clk disable */
		da_separate_v2_clr_reg_bits(DAC_CHAN_CTRL_REG,
							(1 << DACL_DWA_EN_OFFSET));

		da_separate_v2_set_reg_bits(CODEC_ANA_RW4_REG,
							(1 << CODEC_ANA_RW_04_DAPL_PD_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_ana_dacr_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* dem/dacr clk enable */
		da_separate_v2_set_reg_bits(DAC_CHAN_CTRL_REG,
							(1 << DACR_DWA_EN_OFFSET));

		da_separate_v2_clr_reg_bits(CODEC_ANA_RW4_REG,
							(1 << CODEC_ANA_RW_04_DAPR_PD_OFFSET));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* dem/dacr clk disable */
		da_separate_v2_clr_reg_bits(DAC_CHAN_CTRL_REG,
							(1 << DACR_DWA_EN_OFFSET));

		da_separate_v2_set_reg_bits(CODEC_ANA_RW4_REG,
							(1 << CODEC_ANA_RW_04_DAPR_PD_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static void da_separate_v2_hplr_pga_power_off(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x40);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x4C);
	msleep(30);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x7C);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW19_REG, 0x8C);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0xFC);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW24_REG, 0x36);
	da_separate_v2_reg_write(codec, CODEC_ANA_RW20_REG, 0x00);

	OUT_FUNCTION;
}

static void da_separate_v2_ana_dac_power_off(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	/* close dac */
	da_separate_v2_ana_dacl_switch_event((struct snd_soc_dapm_widget *)NULL,
				(struct snd_kcontrol *)NULL, SND_SOC_DAPM_PRE_PMD);
	da_separate_v2_ana_dacr_switch_event((struct snd_soc_dapm_widget *)NULL,
				(struct snd_kcontrol *)NULL, SND_SOC_DAPM_PRE_PMD);

	/* close  daclr-d */
	da_separate_v2_reg_update(CLK_EN_CFG_REG, ANA_DACLR_EN_MASK, 0x0);

	OUT_FUNCTION;
}

static void da_separate_v2_headphone_pop_off(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	da_separate_v2_hplr_pga_power_off(codec);
	da_separate_v2_ana_dac_power_off(codec);

	OUT_FUNCTION;
}

static void da_separate_v2_classd_pop_off(struct da_separate_v2_priv *priv)
{
	enum da_separate_v2_classd_supply_type supply_type = priv->classd_supply_type;
	struct snd_soc_component *codec = priv->codec;

	IN_FUNCTION;

	if (supply_type >= CLASSD_SUPPLY_INVALID) {
		AUDIO_LOGI("no need to pop off\n");
		return;
	}

#ifdef CONFIG_HISI_HI6555V500_PMU
	snd_soc_component_update_bits(codec, CTRL_REGA_CLASSD_ONOFF_REG,
			BIT(CTRL_REGA_REG_CLASSD_EN_OFFSET), BIT(CTRL_REGA_REG_CLASSD_EN_OFFSET));

	snd_soc_component_update_bits(codec, CTRL_REGB_CLASSD_CTRL0_REG,
			BIT(CTRL_REGB_CLASSD_DRV_EN_OFFSET), BIT(CTRL_REGB_CLASSD_DRV_EN_OFFSET));
#else
	da_separate_v2_reg_update(CTRL_REG_CLASSD_CTRL0_REG, ANA_CLASSD_EN_MASK, 0x9);
#endif

	OUT_FUNCTION;
}

static int da_separate_v2_hplr_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	unsigned int val_l,val_r;

	IN_FUNCTION;

	WARN_ON(!codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*if hp path contain DAC DL/DR MIXER, mute audio DL&DR to avoid headphone pop*/
		val_l = da_separate_v2_reg_read(soc_codec, DACL_MIXER4_CTRL0_REG);
		val_l = val_l & (0x1 << DACL_MIXER4_IN3_MUTE_OFFSET);
		val_r = da_separate_v2_reg_read(soc_codec, DACR_MIXER4_CTRL0_REG);
		val_r = val_r & (0x1 << DACR_MIXER4_IN3_MUTE_OFFSET);

		if (!val_l) {
			da_separate_v2_set_reg_bits(DACL_MIXER4_CTRL0_REG, 0x1 << DACL_MIXER4_IN3_MUTE_OFFSET);
		}
		if (!val_r) {
			da_separate_v2_set_reg_bits(DACR_MIXER4_CTRL0_REG, 0x1 << DACR_MIXER4_IN3_MUTE_OFFSET);
		}
		if(!val_l || !val_r) {
			msleep(5);
		}

		da_separate_v2_reg_write(codec, CODEC_ANA_RW20_REG, 0x88);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW24_REG, 0x36);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW19_REG, 0x8C);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x7C);
		msleep(5);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW19_REG, 0xCC);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x4C);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW24_REG, 0x16);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x40);
		msleep(90);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW5_REG, 0x00);

		/*if hp path contain DAC DL/DR MIXER, unmute audio DL&DR to avoid headphone pop*/
		if (!val_l) {
			da_separate_v2_clr_reg_bits(DACL_MIXER4_CTRL0_REG, 0x1 << DACL_MIXER4_IN3_MUTE_OFFSET);
		}
		if (!val_r) {
			da_separate_v2_clr_reg_bits(DACR_MIXER4_CTRL0_REG, 0x1 << DACR_MIXER4_IN3_MUTE_OFFSET);
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		da_separate_v2_hplr_pga_power_off(codec);

		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static void da_separate_v2_power_codec(struct snd_soc_component *codec, int on)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);

	if (on) {
#ifdef CONFIG_HISI_HI6555V500_PMU
		snd_soc_component_update_bits(codec, CODEC_ANA_RW10_REG,
			BIT(DISCHARGE_DISABLE_OFFSET), BIT(DISCHARGE_DISABLE_OFFSET));
#endif

#ifndef AUDIO_FACTORY_MODE
		da_separate_v2_set_reg_bits(CODEC_ANA_RW12_REG, BIT(CODEC_IDEL_TONE_CTRL_OFFSET));
#endif
		da_separate_v2_set_reg_bits(CODEC_ANA_RW3_REG,
			BIT(CODEC_MUTE_ADCL_OFFSET) | BIT(CODEC_MUTE_ADCR_OFFSET) | BIT(CODEC_MUTE_ADC3_OFFSET));
		da_separate_v2_set_reg_bits(CODEC_ANA_RW22_REG,
			BIT(CODEC_STB_CTRL_SEC_OFFSET) | BIT(CODEC_STB_CTRL_OFFSET));
		da_separate_v2_set_reg_bits(CODEC_ANA_RW23_REG,
			BIT(CODEC_STB_1N_OFFSET) | BIT(CODEC_STB_N12DB_GAIN_OFFSET));
		da_separate_v2_reg_write(codec, CODEC_ANA_RW1_REG, 0x07);
		msleep(6);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW1_REG, 0x03);
		da_separate_v2_reg_write(codec, CODEC_ANA_RW1_REG, 0x01);
	} else {
		da_separate_v2_reg_write(codec, CODEC_ANA_RW1_REG, 0x04);
	}

	OUT_FUNCTION;
}

static int da_separate_v2_ana_sif_mixer_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*codec ana/sif en*/
		da_separate_v2_set_reg_bits(CLK_EN_CFG_REG,
							(1 << CODEC_ANA_EN_OFFSET)
						   |(1 << SIF_EN_OFFSET));
		break;
	case SND_SOC_DAPM_POST_PMD:
		/*codec ana/sif dis*/
		da_separate_v2_clr_reg_bits(CLK_EN_CFG_REG,
							(1 << CODEC_ANA_EN_OFFSET)
						   |(1 << SIF_EN_OFFSET));
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static void da_separate_v2_enable_ibias(struct snd_soc_component *codec, bool enable)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	AUDIO_LOGD("Begin, en=%d\n", enable);
	WARN_ON(!priv);

	if (!priv->ibias_hsmicbias_en && 0 == priv->ibias_work) {
		if (enable) {
			AUDIO_LOGI("power up\n");
			da_separate_v2_power_codec(codec, true);
		} else {
			AUDIO_LOGI("power down\n");
			da_separate_v2_power_codec(codec, false);
		}
	}
	AUDIO_LOGD("End\n");
}

static void da_separate_v2_enable_hsd(struct snd_soc_component *codec)
{
	IN_FUNCTION;

#ifdef CONFIG_HISI_HI6555V500_PMU
	snd_soc_component_update_bits(codec, CODEC_ANA_RW33_REG,
			BIT(PLUG_IN_DETECT_MODE_OFFSET), BIT(PLUG_IN_DETECT_MODE_OFFSET));
	snd_soc_component_update_bits(codec, CODEC_ANA_RW8_REG,
			BIT(MBHD_COMP_HSD_VOLTAGE_PD_OFFSET) | BIT(MBHD_COMP_HSD_PD_OFFSET), 0);
#else
	da_separate_v2_reg_write(codec, CODEC_ANA_RW31_REG, 0x1);
#endif

	OUT_FUNCTION;
}

static void da_separate_v2_ibias_hsmicbias_enable(struct snd_soc_component *codec, bool enable)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	if (!priv) {
		AUDIO_LOGE("priv pointer is null\n");
		return;
	}

	mutex_lock(&priv->ibias_mutex);
	if (enable) {
		if (!priv->ibias_hsmicbias_en) {
			da_separate_v2_enable_ibias(codec, true);
			priv->ibias_hsmicbias_en = true;
		}
	} else {
		if (priv->ibias_hsmicbias_en) {
			priv->ibias_hsmicbias_en = false;
			da_separate_v2_enable_ibias(codec, false);
		}
	}
	mutex_unlock(&priv->ibias_mutex);

	OUT_FUNCTION;
}

static void da_separate_v2_ibias_work_enable(struct snd_soc_component *codec, bool enable)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);

	mutex_lock(&priv->ibias_mutex);
	if (enable) {
		if (0 == priv->ibias_work)
			da_separate_v2_enable_ibias(codec, true);

		WARN_ON(MAX_INT32 == priv->ibias_work);
		++priv->ibias_work;
	} else {
		if (priv->ibias_work <= 0) {
			AUDIO_LOGE("ibias_work is %d, fail to disable ibias\n", priv->ibias_work);
			mutex_unlock(&priv->ibias_mutex);
			return;
		}

		--priv->ibias_work;

		if (0 == priv->ibias_work)
			da_separate_v2_enable_ibias(codec, false);
	}
	mutex_unlock(&priv->ibias_mutex);

	OUT_FUNCTION;
}

static int da_separate_v2_smt_ibias_supply_power_mode_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_separate_v2_ibias_work_enable(soc_codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_separate_v2_ibias_work_enable(soc_codec, false);
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_pll_supply_power_mode_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = NULL;
	struct da_separate_v2_priv *priv = NULL;
	int ret = 0;

	IN_FUNCTION;

	codec = snd_soc_dapm_to_component(w->dapm);
	WARN_ON(!codec);
	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	if (UDP_PLATFORM == priv->platform_type) {
		switch (event) {
		case SND_SOC_DAPM_PRE_PMU:
			priv->have_dapm = true;
			break;
		case SND_SOC_DAPM_POST_PMD:
			priv->have_dapm = false;
			break;
		default:
			AUDIO_LOGE("power mode event err : %d\n", event);
			break;
		}
	} else {
		AUDIO_LOGI("fpga clk is always on\n");
	}

	OUT_FUNCTION;

	return ret;
}

int da_separate_v2_micbias1_mic_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_separate_v2_reg_update(CODEC_ANA_RW7_REG, MICBISA1_SET_MASK, 0x0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_separate_v2_reg_update(CODEC_ANA_RW7_REG, MICBISA1_SET_MASK, 0xff);
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}
	OUT_FUNCTION;
	return 0;
}

int da_separate_v2_micbias2_mic_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_separate_v2_reg_update(CODEC_ANA_RW7_REG, MICBISA2_SET_MASK, 0x0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_separate_v2_reg_update(CODEC_ANA_RW7_REG, MICBISA2_SET_MASK, 0xff);
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}
	OUT_FUNCTION;
	return 0;
}

int da_separate_v2_hpmicbias_mic_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = NULL;
	struct da_separate_v2_priv *priv = NULL;
	unsigned int irq_mask = 0;
	unsigned int val_mic_mux,val_pga;

	IN_FUNCTION;

	codec = snd_soc_dapm_to_component(w->dapm);
	WARN_ON(!codec);
	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* to avoid irq while MBHD_COMP power up, mask all COMP irq */
		irq_mask = da_separate_v2_reg_read(codec, ANA_IRQM_REG0_REG);
		da_separate_v2_set_reg_bits(ANA_IRQM_REG0_REG, irq_mask | IRQ_MSK_COMP);
		da_separate_mbhc_set_micbias(priv->mbhc, true);
#ifdef CONFIG_HISI_HI6555V500_PMU
		msleep(25);
#else
		msleep(1);

		/*get orignal value of mic mux and main pga*/
		val_mic_mux = da_separate_v2_reg_read(codec, CODEC_ANA_RW9_REG);
		val_mic_mux = val_mic_mux & MAINPGA_MIC_IN_MASK;
		val_pga = da_separate_v2_reg_read(codec, CODEC_ANA_RW2_REG);
		val_pga = val_pga & (0x1 << CODEC_MAINPGA_PD_OFFSET);

		/*main PGA input select HP MIC*/
		da_separate_v2_reg_update(CODEC_ANA_RW9_REG, MAINPGA_MIC_IN_MASK, MAINPGA_MIC_IN_HP);

		/*close MAIN PGA*/
		da_separate_v2_set_reg_bits(CODEC_ANA_RW2_REG, 0x1 << CODEC_MAINPGA_PD_OFFSET);
		msleep(1);

		/*open MAIN PGA*/
		da_separate_v2_clr_reg_bits(CODEC_ANA_RW2_REG, 0x1 << CODEC_MAINPGA_PD_OFFSET);
		msleep(20);
		/*close MAIN PGA*/
		da_separate_v2_set_reg_bits(CODEC_ANA_RW2_REG, 0x1 << CODEC_MAINPGA_PD_OFFSET);
		msleep(1);

		/*open MAIN PGA*/
		da_separate_v2_clr_reg_bits(CODEC_ANA_RW2_REG, 0x1 << CODEC_MAINPGA_PD_OFFSET);
		msleep(10);
		/*close MAIN PGA*/
		da_separate_v2_set_reg_bits(CODEC_ANA_RW2_REG, 0x1 << CODEC_MAINPGA_PD_OFFSET);
		msleep(1);

		/*set orignal value of mic mux and main pga*/
		da_separate_v2_reg_update(CODEC_ANA_RW9_REG, MAINPGA_MIC_IN_MASK, val_mic_mux);
		da_separate_v2_reg_update(CODEC_ANA_RW2_REG, (0x1 << CODEC_MAINPGA_PD_OFFSET), val_pga);
		msleep(1);
#endif

		/* when pwr up finished clean it and cancel mask */
		da_separate_v2_reg_write(codec, ANA_IRQ_REG0_REG, IRQ_MSK_COMP);
		da_separate_v2_clr_reg_bits(ANA_IRQM_REG0_REG, IRQ_MSK_BTN_NOR);
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_separate_mbhc_set_micbias(priv->mbhc, false);
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}
	OUT_FUNCTION;
	return 0;
}

static int dmic_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_separate_v2_set_reg_bits(R_CODEC_DMA_SEL, BIT(DMIC_SIF_SEL));
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG, BIT(DMIC_CLKEN_OFFSET));
		break;
	case SND_SOC_DAPM_POST_PMD:
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG, BIT(DMIC_CLKEN_OFFSET));
		da_separate_v2_clr_reg_bits(R_CODEC_DMA_SEL, BIT(DMIC_SIF_SEL));
		break;
	default:
		AUDIO_LOGE("power mode event err: %d", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

int da_separate_v2_adc_drv_power_mode_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		msleep(10);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static int da_separate_v2_i2s1_i2s_tx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable i2s1 mode */
		da_separate_v2_clr_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << I2S1_TDM_MODE_OFFSET);

		/*enable i2s1 tx */
		da_separate_v2_set_reg_bits(I2S1_TDM_CTRL0_REG,
				1 << I2S1_IF_TX_ENA_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable i2s1 tx */
		da_separate_v2_clr_reg_bits(I2S1_TDM_CTRL0_REG,
				1 << I2S1_IF_TX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGE("i2s1_clk_i2s_tx_switch event err : %d\n", event);
		break;
	}

	return 0;
}

static int da_separate_v2_i2s1_i2s_rx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable i2s1 rx */
		da_separate_v2_set_reg_bits(I2S1_TDM_CTRL0_REG,
				1 << I2S1_IF_RX_ENA_OFFSET);

		/*enable smartpa feedback lr */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_L_UP_AFIFO_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_R_UP_AFIFO_CLKEN_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable smartpa feedback lr */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_L_UP_AFIFO_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_R_UP_AFIFO_CLKEN_OFFSET);

		/*disable i2s1 rx */
		da_separate_v2_clr_reg_bits(I2S1_TDM_CTRL0_REG,
				1 << I2S1_IF_RX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGE("i2s1_i2s_rx_switch event err : %d\n", event);
		break;
	}

	return 0;
}

static int da_separate_v2_i2s1_tdm_rx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable tdm rx */
		da_separate_v2_set_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << TDM1_IF_RX_ENA_OFFSET);

		/*enable smartpa feedback lr */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_L_UP_AFIFO_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_R_UP_AFIFO_CLKEN_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable smartpa feedback lr */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_L_UP_AFIFO_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << SPA_R_UP_AFIFO_CLKEN_OFFSET);

		/*disable tdm rx */
		da_separate_v2_clr_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << TDM1_IF_RX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGE("i2s1_i2s_rx_switch event err : %d\n", event);
		break;
	}

	return 0;
}

static int da_separate_v2_i2s1_tdm_tx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable tdm mode */
		da_separate_v2_set_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << I2S1_TDM_MODE_OFFSET);

		/*enable tdm tx */
		da_separate_v2_set_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << TDM1_IF_TX_ENA_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable tdm tx */
		da_separate_v2_clr_reg_bits(I2S1_TDM_CTRL1_REG,
				1 << TDM1_IF_TX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGE("i2s1_clk_tdm_tx_switch_event event err : %d\n", event);
		break;
	}

	return 0;
}

static int da_separate_v2_3rdCodec_dlr_fifo_i2s3_tx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable i2s3 tx */
		da_separate_v2_set_reg_bits(I2S3_PCM_CTRL_REG,
				1 << I2S3_IF_TX_ENA_OFFSET);

		/*enable 3rd codec dlr fifo*/
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
				1 << CODEC3_L_DN_AFIFO_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN1_REG,
				1 << CODEC3_R_DN_AFIFO_CLKEN_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable 3rd codec dlr fifo*/
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
				1 << CODEC3_R_DN_AFIFO_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN1_REG,
				1 << CODEC3_L_DN_AFIFO_CLKEN_OFFSET);

		/*disable i2s3 tx */
		da_separate_v2_clr_reg_bits(I2S3_PCM_CTRL_REG,
				1 << I2S3_IF_TX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGE("3rdCodec_dlr_fifo_i2s3_tx_switch event err : %d\n", event);
		break;
	}

	return 0;
}

static int da_separate_v2_s2_src_i2s2_tx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable s2 src downlink*/
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_TX_SRCDN_CLKEN_OFFSET);

		/*enable i2s2 tx */
		da_separate_v2_set_reg_bits(I2S2_PCM_CTRL_REG,
				1 << I2S2_IF_TX_ENA_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable i2s2 tx */
		da_separate_v2_clr_reg_bits(I2S2_PCM_CTRL_REG,
				1 << I2S2_IF_TX_ENA_OFFSET);

		/*disable s2 src downlink*/
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_TX_SRCDN_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGE("s2_src_i2s2_clk_tx_switch event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;
	return 0;
}

static int da_separate_v2_i2s2_rx_pga_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable s2 lr uplink pga clk*/
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_PGA_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_PGA_CLKEN_OFFSET);


		/*enable i2s2 rx*/
		da_separate_v2_set_reg_bits(I2S2_PCM_CTRL_REG,
				1 << I2S2_IF_RX_ENA_OFFSET);
		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable i2s2 rx*/
		da_separate_v2_clr_reg_bits(I2S2_PCM_CTRL_REG,
				1 << I2S2_IF_RX_ENA_OFFSET);

		/*disable s2 lr uplink pga clk*/
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_PGA_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_PGA_CLKEN_OFFSET);

		break;
	default:
		AUDIO_LOGE("i2s2_src_pga_event event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;
	return 0;
}

static int da_separate_v2_s2_src_rx_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/*enable s2 lr uplink src clk */
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_SRCDN_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_SRCDN_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_SRCUP_CLKEN_OFFSET);
		da_separate_v2_set_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_SRCUP_CLKEN_OFFSET);

		break;

	case SND_SOC_DAPM_POST_PMD:
		/*disable s2 lr uplink src clk */
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_SRCDN_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_SRCDN_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_L_RX_SRCUP_CLKEN_OFFSET);
		da_separate_v2_clr_reg_bits(CODEC_CLK_EN0_REG,
				1 << BT_R_RX_SRCUP_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGE("i2s2_src_rx_event event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;
	return 0;
}

static int da_separate_v2_i2s2_clk_switch_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	IN_FUNCTION;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		da_separate_v2_i2s2_set_pinctrl_default();
		break;

	case SND_SOC_DAPM_POST_PMD:
		da_separate_v2_i2s2_set_pinctrl_idle();
		break;
	default:
		AUDIO_LOGE("i2s2_clk_switch_event event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;
	return 0;
}

static void _classd_schg_supply_enable(struct da_separate_v2_priv *priv)
{
	int ret;
	struct regulator *regulator_schg = NULL;

	IN_FUNCTION;

	regulator_schg = priv->regulator_schg_boost3;
	if (IS_ERR(regulator_schg)) {
		AUDIO_LOGE("regulator_schg_boost3 is error:%ld\n", PTR_ERR(regulator_schg));
		return;
	}

	ret = regulator_enable(regulator_schg);
	if (ret) {
		AUDIO_LOGE("regulator_schg_boost3 enable fail, ret=%d\n", ret);
		return;
	}

	AUDIO_LOGI("classd schg regulator enable ok\n");

	OUT_FUNCTION;
}

static void _classd_schg_supply_disable(struct da_separate_v2_priv *priv)
{
	int ret;
	struct regulator *regulator_schg = NULL;

	IN_FUNCTION;

	regulator_schg = priv->regulator_schg_boost3;
	if (IS_ERR(regulator_schg)) {
		AUDIO_LOGE("regulator_schg_boost3 is error:%ld\n", PTR_ERR(regulator_schg));
		return;
	}

	if (!regulator_is_enabled(regulator_schg)) {
		AUDIO_LOGW("regulator_schg_boost3 is not enabled when disable\n");
		return;
	}

	ret = regulator_disable(regulator_schg);
	if (ret) {
		AUDIO_LOGE("regulator_schg_boost3 enable fail, ret=%d\n", ret);
		return;
	}

	AUDIO_LOGI("classd schg regulator disable ok\n");

	OUT_FUNCTION;
}

static void _classd_gpio_supply_switch(struct da_separate_v2_priv *priv, bool enable)
{
	IN_FUNCTION;

	if (!gpio_is_valid(priv->gpio_classd)) {
		AUDIO_LOGE("switch classd gpio:%d fail\n", priv->gpio_classd);
		return;
	}

	if (enable) {
		gpio_set_value(priv->gpio_classd, GPIO_LEVEL_HIGH);
		AUDIO_LOGI("pull up classd gpio ok\n");
	} else {
		gpio_set_value(priv->gpio_classd, GPIO_LEVEL_LOW);
		AUDIO_LOGI("pull down classd gpio ok\n");
	}

	OUT_FUNCTION;
}

static void _classd_gpio_supply_enable(struct da_separate_v2_priv *priv)
{
	IN_FUNCTION;
	_classd_gpio_supply_switch(priv, true);
	OUT_FUNCTION;
}

static void _classd_gpio_supply_disable(struct da_separate_v2_priv *priv)
{
	IN_FUNCTION;
	_classd_gpio_supply_switch(priv, false);
	OUT_FUNCTION;
}

static int da_separate_v2_classd_supply_power_mode_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = NULL;
	struct da_separate_v2_priv *priv = NULL;
	enum da_separate_v2_classd_supply_type supply_type;

	IN_FUNCTION;

	codec = snd_soc_dapm_to_component(w->dapm);
	WARN_ON(!codec);

	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	supply_type = priv->classd_supply_type;

	if (supply_type >= CLASSD_SUPPLY_INVALID)
	{
		AUDIO_LOGE("classd supply type is invalid:%d\n", supply_type);
		return 0;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (CLASSD_SUPPLY_SCHARGER == supply_type) {
			_classd_schg_supply_enable(priv);
		} else if (CLASSD_SUPPLY_GPIO == supply_type) {
			_classd_gpio_supply_enable(priv);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (CLASSD_SUPPLY_SCHARGER == supply_type) {
			_classd_schg_supply_disable(priv);
		} else if (CLASSD_SUPPLY_GPIO == supply_type) {
			_classd_gpio_supply_disable(priv);
		}
		break;
	default:
		AUDIO_LOGE("power mode event err : %d\n", event);
		break;
	}

	OUT_FUNCTION;

	return 0;
}

static const struct snd_kcontrol_new da_separate_v2_ana_dacl_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT0, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_dacr_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACR",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT1, 1, 0/*NO INVERT */),
};


static const struct snd_kcontrol_new da_separate_v2_ana_headset_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT9, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_spk_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			CTRL_REG_CLASSD_ANA_SPK_DAPM_REG, CTRL_REG_REG_CLASSD_EN_OFFSET, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_rcv_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			CODEC_ANA_RW6_REG, CODEC_EAR_PD_OFFSET, 1, 1/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_lout_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			CODEC_ANA_RW6_REG, CODEC_LOUT_PD_OFFSET, 1, 1/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_audio_ul_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT2, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_audio_ur_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT3, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_voice_ul_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT4, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_voice_ur_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT5, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_echo_l_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT6, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_echo_r_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT7, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_mic3_up_fifo_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("ENABLE",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT8, 1, 0/*NO INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_dac_dl_mixer_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("IN1_S2_L",
			DACL_MIXER4_CTRL0_REG, DACL_MIXER4_IN1_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN2_VOICE_DL",
			DACL_MIXER4_CTRL0_REG, DACL_MIXER4_IN2_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN3_AUDIO_DL",
			DACL_MIXER4_CTRL0_REG, DACL_MIXER4_IN3_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN4_SIDE_TONE_L",
			DACL_MIXER4_CTRL0_REG, DACL_MIXER4_IN4_MUTE_OFFSET, 1, 1/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_dac_dr_mixer_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("IN1_S2_R",
			DACR_MIXER4_CTRL0_REG, DACR_MIXER4_IN1_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN2_VOICE_DR",
			DACR_MIXER4_CTRL0_REG, DACR_MIXER4_IN2_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN3_AUDIO_DR",
			DACR_MIXER4_CTRL0_REG, DACR_MIXER4_IN3_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("IN4_SIDE_TONE_R",
			DACR_MIXER4_CTRL0_REG, DACR_MIXER4_IN4_MUTE_OFFSET, 1, 1/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_hpl_mixer_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW20_REG, CODEC_ANA_RW_20_HP_L_DACL_OFFSET, 1, 0/* INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW20_REG, CODEC_ANA_RW_20_HP_L_DACR_OFFSET, 1, 0/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_hpr_mixer_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW20_REG, CODEC_ANA_RW_20_HP_R_DACL_OFFSET, 1, 0/* INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW20_REG, CODEC_ANA_RW_20_HP_R_DACR_OFFSET, 1, 0/* INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_sif_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("SIF_DACL_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT11, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("SIF_DACR_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT12, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("SIF_ADCL_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT13, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("SIF_ADCR_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT14, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("SIF_ADC3_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT15, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_sif_en_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("ANA_SIFL_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT16, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("ANA_SIFR_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT17, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("ANA_MUX3_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT18, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("ANA_MUX4_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT19, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("ANA_MUX5_EN",
			DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT20, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ear_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW25_REG, CODEC_MIXOUT_EAR_DACL_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW25_REG, CODEC_MIXOUT_EAR_DACR_OFFSET, 1, 0/* not INVERT */),
#ifdef CONFIG_HISI_HI6555V500_PMU
	SOC_DAPM_SINGLE("DAC_ULTR",
			CODEC_ANA_RW25_REG, MIXOUT_EAR_DAC_ULTRA_OFFSET, 1, 0/* not INVERT */),
#endif
};

#ifdef CONFIG_HISI_HI6555V500_PMU
static const struct snd_kcontrol_new da_separate_line_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL", CODEC_ANA_RW27_REG,
		LOMIX_SEL_DACL_OFFSET, 1, 0),
	SOC_DAPM_SINGLE("DACR", CODEC_ANA_RW27_REG,
		LOMIX_SEL_DACR_OFFSET, 1, 0),
	SOC_DAPM_SINGLE("DAC_ULTR", CODEC_ANA_RW27_REG,
		LOMIX_SEL_DAC_ULTRA_OFFSET, 1, 0),
};
#endif

static const struct snd_kcontrol_new da_separate_v2_ana_adc1_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW12_REG, CODEC_ADCL_MIXIN_DACL_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW12_REG, CODEC_ADCL_MIXIN_DACR_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("MAIN MIC",
			CODEC_ANA_RW12_REG, CODEC_ADCL_MIXIN_MIC_PGA1_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("AUX MIC",
			CODEC_ANA_RW12_REG, CODEC_ADCL_MIXIN_MIC_PGA2_OFFSET, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_adc2_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW14_REG, CODEC_ADCR_MIXIN_DACL_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW14_REG, CODEC_ADCR_MIXIN_DACR_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("MAIN MIC",
			CODEC_ANA_RW14_REG, CODEC_ADCR_MIXIN_MIC_PGA1_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("AUX MIC",
			CODEC_ANA_RW14_REG, CODEC_ADCR_MIXIN_MIC_PGA2_OFFSET, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_ana_adc3_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("DACL",
			CODEC_ANA_RW16_REG, CODEC_ADC3_MIXIN_DACL_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("DACR",
			CODEC_ANA_RW16_REG, CODEC_ADC3_MIXIN_DACR_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("MAIN MIC",
			CODEC_ANA_RW16_REG, CODEC_ADC3_MIXIN_MIC_PGA1_OFFSET, 1, 0/* not INVERT */),
	SOC_DAPM_SINGLE("MIC3",
			CODEC_ANA_RW16_REG, CODEC_ADC3_MIXIN_MIC_PGA3_OFFSET, 1, 0/* not INVERT */),
};

static const struct snd_kcontrol_new da_separate_v2_s2_mixer_dapm_controls[] = {
	SOC_DAPM_SINGLE("S2_DACL_MIXER",
			BT_TX_MIXER2_CTRL_REG, BT_TX_MIXER2_IN1_MUTE_OFFSET, 1, 1/* INVERT */),
	SOC_DAPM_SINGLE("S2_DACR_MIXER",
			BT_TX_MIXER2_CTRL_REG, BT_TX_MIXER2_IN2_MUTE_OFFSET, 1, 1/* INVERT */),
};

static const char *da_separate_v2_dacl_mux_texts[] = {
	"DACL_MIXER",
	"S2_L",
	"S1_L",
};

static const struct soc_enum da_separate_v2_dacl_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, DACL_SRCUP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_dacl_mux_texts), da_separate_v2_dacl_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_dacl_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_dacl_mux_enum);

static const char *da_separate_v2_dacr_mux_texts[] = {
	"DACR_MIXER",
	"S2_R",
	"S1_R",
};

static const struct soc_enum da_separate_v2_dacr_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, DACR_SRCUP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_dacr_mux_texts), da_separate_v2_dacr_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_dacr_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_dacr_mux_enum);

static const char *da_separate_v2_adc1_mux_texts[] = {
	"ADC1",
	"ADC2",
	"ADC3",
	"DMIC_L",
};

static const struct soc_enum da_separate_v2_adc1_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, ADCL_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_adc1_mux_texts), da_separate_v2_adc1_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_adc1_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_adc1_mux_enum);

static const char *da_separate_v2_adc2_mux_texts[] = {
	"ADC1",
	"ADC2",
	"ADC3",
	"DMIC_R",
};

static const struct soc_enum da_separate_v2_adc2_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, ADCR_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_adc2_mux_texts), da_separate_v2_adc2_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_adc2_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_adc2_mux_enum);

static const char *da_separate_v2_adc3_mux_texts[] = {
	"ADC1",
	"ADC2",
	"ADC3",
};

static const struct soc_enum da_separate_v2_adc3_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, ADC_MIC3_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_adc3_mux_texts), da_separate_v2_adc3_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_adc3_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_adc3_mux_enum);

static const char *da_separate_v2_ana_dacl_mux_texts[] = {
	"SIFL",
	"ADC1",
	"ADC2",
	"ADC3",
};

static const struct soc_enum da_separate_v2_ana_dacl_mux_enum =
SOC_ENUM_SINGLE(DAC_CHAN_CTRL_REG, DACL_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_ana_dacl_mux_texts), da_separate_v2_ana_dacl_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_ana_dacl_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ana_dacl_mux_enum);

static const char *da_separate_v2_ana_dacr_mux_texts[] = {
	"SIFR",
	"ADC1",
	"ADC2",
	"ADC3",
};

static const struct soc_enum da_separate_v2_ana_dacr_mux_enum =
SOC_ENUM_SINGLE(DAC_CHAN_CTRL_REG, DACR_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_ana_dacr_mux_texts), da_separate_v2_ana_dacr_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_ana_dacr_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ana_dacr_mux_enum);

static const char *da_separate_v2_audio_ul_mux_texts[] = {
	"DACL_MIXER",
	"ADCL",
	"S2_L_SRC",
};

static const struct soc_enum da_separate_v2_audio_ul_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_AUDIO_L_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_audio_ul_mux_texts), da_separate_v2_audio_ul_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_audio_ul_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_audio_ul_mux_enum);

static const char *da_separate_v2_audio_ur_mux_texts[] = {
	"DACR_MIXER",
	"ADCR",
	"S2_R_SRC",
};

static const struct soc_enum da_separate_v2_audio_ur_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_AUDIO_R_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_audio_ur_mux_texts), da_separate_v2_audio_ur_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_audio_ur_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_audio_ur_mux_enum);

static const char *da_separate_v2_side_tone_mux_texts[] = {
	"ADC_MIC3_I",
	"ADCR_I",
	"ADCL_I",
};

static const struct soc_enum da_separate_v2_side_tone_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, SIDETONE_PGA_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_side_tone_mux_texts), da_separate_v2_side_tone_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_side_tone_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_side_tone_mux_enum);


static const char *da_separate_v2_voice_ul_mux[] = {
	"ADCL_SRC",
	"S2_L",
};

static const struct soc_enum da_separate_v2_voice_ul_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_VOICE_L_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_voice_ul_mux), da_separate_v2_voice_ul_mux);

static const struct snd_kcontrol_new da_separate_v2_voice_ul_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_voice_ul_mux_enum);

static const char *da_separate_v2_voice_ur_mux[] = {
	"ADCR_SRC",
	"S2_R",
};

static const struct soc_enum da_separate_v2_voice_ur_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_VOICE_R_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_voice_ur_mux), da_separate_v2_voice_ur_mux);

static const struct snd_kcontrol_new da_separate_v2_voice_ur_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_voice_ur_mux_enum);

static const char *da_separate_v2_mic3_up_mux[] = {
	"ADC3/ADC2/ADC1",
	"ADC MIC3_SRC",
};

static const struct soc_enum da_separate_v2_mic3_up_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, MIC3_UP_AFIFO_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_mic3_up_mux), da_separate_v2_mic3_up_mux);

static const struct snd_kcontrol_new da_separate_v2_mic3_up_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_mic3_up_mux_enum);

static const char *da_separate_v2_echo_l_mux[] = {
	"DACL_MIXER",
	"S3_L",
};

static const struct soc_enum da_separate_v2_echo_l_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_ECHO_L_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_echo_l_mux), da_separate_v2_echo_l_mux);

static const struct snd_kcontrol_new da_separate_v2_echo_l_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_echo_l_mux_enum);

static const char *da_separate_v2_echo_r_mux[] = {
	"DACR_MIXER",
	"S3_R",
};

static const struct soc_enum da_separate_v2_echo_r_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO24_ECHO_R_UP_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_echo_r_mux), da_separate_v2_echo_r_mux);

static const struct snd_kcontrol_new da_separate_v2_echo_r_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_echo_r_mux_enum);


static const char *da_separate_v2_src10_mux[] = {
	"BYPASS",
	"SRC10",
};

static const struct soc_enum da_separate_v2_src10_mux_enum =
SOC_ENUM_SINGLE(CODEC_CLK_EN1_REG, ECHO_L_UP_SRCDN_CLKEN_OFFSET,
		ARRAY_SIZE(da_separate_v2_src10_mux), da_separate_v2_src10_mux);

static const struct snd_kcontrol_new da_separate_v2_src10_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_src10_mux_enum);


static const char *da_separate_v2_src11_mux[] = {
	"BYPASS",
	"SRC11",
};

static const struct soc_enum da_separate_v2_src11_mux_enum =
SOC_ENUM_SINGLE(CODEC_CLK_EN1_REG, ECHO_R_UP_SRCDN_CLKEN_OFFSET,
		ARRAY_SIZE(da_separate_v2_src11_mux), da_separate_v2_src11_mux);

static const struct snd_kcontrol_new da_separate_v2_src11_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_src11_mux_enum);


static const char *da_separate_v2_ana_mux3_mux[] = {
	"ADC1",
	"SIFL",
	"SIFR",
};

static const struct soc_enum da_separate_v2_ana_mux3_mux_enum =
SOC_ENUM_SINGLE(ADC_CHAN_CTRL_REG, ADCL_DOUT_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_ana_mux3_mux), da_separate_v2_ana_mux3_mux);

static const struct snd_kcontrol_new da_separate_v2_ana_mux3_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ana_mux3_mux_enum);

static const char *da_separate_v2_ana_mux4_mux[] = {
	"ADC2",
	"SIFL",
	"SIFR",
};

static const struct soc_enum da_separate_v2_ana_mux4_mux_enum =
SOC_ENUM_SINGLE(ADC_CHAN_CTRL_REG, ADCR_DOUT_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_ana_mux4_mux), da_separate_v2_ana_mux4_mux);

static const struct snd_kcontrol_new da_separate_v2_ana_mux4_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ana_mux4_mux_enum);

static const char *da_separate_v2_ana_mux5_mux[] = {
	"ADC3",
	"SIFL",
	"SIFR",
};

static const struct soc_enum da_separate_v2_ana_mux5_mux_enum =
SOC_ENUM_SINGLE(ADC_CHAN_CTRL_REG, ADC_MIC3_DOUT_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_ana_mux5_mux), da_separate_v2_ana_mux5_mux);

static const struct snd_kcontrol_new da_separate_v2_ana_mux5_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ana_mux5_mux_enum);

static const char *da_separate_v2_main_hp_mic_mux[] = {
	"MUTE",
	"MAIN_MIC",
	"HP_MIC",
};

static const struct soc_enum da_separate_v2_main_hp_mic_mux_enum =
SOC_ENUM_SINGLE(CODEC_ANA_RW9_REG, CODEC_MAINPGA_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_main_hp_mic_mux), da_separate_v2_main_hp_mic_mux);

static const struct snd_kcontrol_new da_separate_v2_main_hp_mic_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_main_hp_mic_mux_enum);

static const char *da_separate_v2_i2s1_s1l_mux_texts[] = {
	"ADCL",
	"DACL_MIXER",
};

static const struct soc_enum da_separate_v2_i2s1_s1l_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO32_PA_L_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_i2s1_s1l_mux_texts), da_separate_v2_i2s1_s1l_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_i2s1_s1l_mux_dapm_controls =
SOC_DAPM_ENUM("Mux",da_separate_v2_i2s1_s1l_mux_enum);

static const char *da_separate_v2_i2s1_s1r_mux_texts[] = {
	"ADCR",
	"DACR_MIXER",
};

static const struct soc_enum da_separate_v2_i2s1_s1r_mux_enum =
SOC_ENUM_SINGLE(CODEC_DIN_MUX_REG, BM_26TO32_PA_R_DIN_SEL_OFFSET,
		ARRAY_SIZE(da_separate_v2_i2s1_s1r_mux_texts), da_separate_v2_i2s1_s1r_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_i2s1_s1r_mux_dapm_controls =
SOC_DAPM_ENUM("Mux",da_separate_v2_i2s1_s1r_mux_enum);

/* sif ad_da loopback mux */
static const char *da_separate_v2_ad_da_loopback_mux_texts[] = {
	"NORMAL",
	"AD_DA_LOOP",
	"MIC3_DA_LOOP",
};

static const struct soc_enum da_separate_v2_ad_da_loopback_mux_enum =
SOC_ENUM_SINGLE(DAC_FILTER_CTRL_REG, SIF_MST_P2S_LOOPBACK_OFFSET,
		ARRAY_SIZE(da_separate_v2_ad_da_loopback_mux_texts), da_separate_v2_ad_da_loopback_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_ad_da_loopback_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_ad_da_loopback_mux_enum);

/* sif da_ad loopback mux */
static const char *da_separate_v2_da_ad_loopback_mux_texts[] = {
	"NORMAL",
	"DAP_ADP_LOOP",
	"DAS_ADS_LOOP",
};

static const struct soc_enum da_separate_v2_da_ad_loopback_mux_enum =
SOC_ENUM_SINGLE(DAC_FILTER_CTRL_REG, SIF_MST_S2P_LOOPBACK_OFFSET,
		ARRAY_SIZE(da_separate_v2_da_ad_loopback_mux_texts), da_separate_v2_da_ad_loopback_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_da_ad_loopback_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_da_ad_loopback_mux_enum);

/* dac -> adc  loopback mux */
static const char *da_separate_v2_adc_loopback_mux_texts[] = {
	"NORMAL",
	"DA_AD_LOOP",
	"DA_MIC3_LOOP",
};

static const struct soc_enum da_separate_v2_adc_loopback_mux_enum =
SOC_ENUM_SINGLE(SIF_LOOPBACK_CFG_REG, ADC_LOOPBACK_OFFSET,
		ARRAY_SIZE(da_separate_v2_adc_loopback_mux_texts), da_separate_v2_adc_loopback_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_adc_loopback_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_adc_loopback_mux_enum);

/* dac_loopback mux */
static const char *da_separate_v2_dac_loopback_mux_texts[] = {
	"NORMAL",
	"DOUT_SDIN",
	"MIC3_SDIN",
};

static const struct soc_enum da_separate_v2_dac_loopback_mux_enum =
SOC_ENUM_SINGLE(SIF_LOOPBACK_CFG_REG, DAC_LOOPBACK_OFFSET,
		ARRAY_SIZE(da_separate_v2_dac_loopback_mux_texts), da_separate_v2_dac_loopback_mux_texts);

static const struct snd_kcontrol_new da_separate_v2_dac_loopback_mux_dapm_controls =
SOC_DAPM_ENUM("Mux", da_separate_v2_dac_loopback_mux_enum);

static const struct snd_kcontrol_new da_separate_v2_i2s1_i2s_tx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S_SWITCH_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT21, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s1_tdm_tx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("TDM_SWITCH_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT22, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_s2_src_i2s2_tx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S2_SWITCH_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT23, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_3rdCodec_dlr_fifo_i2s3_tx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S3_SWITCH_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT24, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s1_i2s_rx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S1_I2S_RX_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT25, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s1_tdm_rx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S1_TDM_RX_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT26, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_s2_src_rx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S2_SRC_RX_EN", DA_SEPARATE_V2_DDR_CODEC_VIR0_ADDR, CODEC_REG_BIT27, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s3_rx_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S3_RX_EN", I2S3_PCM_CTRL_REG, I2S3_IF_RX_ENA_OFFSET, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s1_clk_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S1_CLK_EN", CODEC_CLK_EN1_REG, I2S1_TDM_CLKEN_OFFSET, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s2_clk_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S2_CLK_EN", CODEC_CLK_EN1_REG, I2S2_PCM_CLKEN_OFFSET, 1, 0),
};

static const struct snd_kcontrol_new da_separate_v2_i2s3_clk_switch_dapm_controls[] = {
	SOC_DAPM_SINGLE("I2S3_CLK_EN", CODEC_CLK_EN1_REG, I2S3_PCM_CLKEN_OFFSET, 1, 0),
};

static const struct snd_soc_dapm_widget da_separate_v2_dapm_widgets[] = {
	/* INPUT - SOC */
	/* downlink */
	SND_SOC_DAPM_INPUT("AUDIO_DLINK INPUT"),
	SND_SOC_DAPM_INPUT("SMT_ANA_DAC_SDATA INPUT"),
	SND_SOC_DAPM_INPUT("VOICE_DLINK INPUT"),
	/* uplink */
	SND_SOC_DAPM_INPUT("ADC1 INPUT"),
	SND_SOC_DAPM_INPUT("ADC2 INPUT"),
	SND_SOC_DAPM_INPUT("ADC3 INPUT"),

	/* INPUT - ANA */
	SND_SOC_DAPM_INPUT("MAINMIC INPUT"),
	SND_SOC_DAPM_INPUT("HPMIC INPUT"),
	SND_SOC_DAPM_INPUT("AUXMIC INPUT"),
	SND_SOC_DAPM_INPUT("MIC3 INPUT"),
	SND_SOC_DAPM_INPUT("I2S1_2_3 INPUT"),
	SND_SOC_DAPM_INPUT("DMIC INPUT"),

	/* OUTPUT - SOC */
	SND_SOC_DAPM_OUTPUT("AUDIO_DLINK OUTPUT"),

	SND_SOC_DAPM_OUTPUT("AUDIO_UL OUTPUT"),
	SND_SOC_DAPM_OUTPUT("AUDIO_UR OUTPUT"),
	SND_SOC_DAPM_OUTPUT("VOICE_UL OUTPUT"),
	SND_SOC_DAPM_OUTPUT("VOICE_UR OUTPUT"),
	SND_SOC_DAPM_OUTPUT("MIC3_UP OUTPUT"),

	SND_SOC_DAPM_OUTPUT("ECHO OUTPUT"),

	/* OUTPUT - ANA */
	/* downlink */
	SND_SOC_DAPM_OUTPUT("SMT_HP_LR OUTPUT"),
	SND_SOC_DAPM_OUTPUT("ANA_EAR OUTPUT"),

	/* uplink */
	SND_SOC_DAPM_OUTPUT("ANA_SIF OUTPUT"),

	/* OUTPUT - I2S */
	SND_SOC_DAPM_OUTPUT("I2S1_2_3 OUTPUT"),


	SND_SOC_DAPM_PGA_E("AUDIO_DL PGA",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_audio_dl_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_PGA_E("AUDIO_DR PGA",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_audio_dr_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_PGA_E("DACL_FILTER",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_dacl_filter_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_PGA_E("DACR_FILTER",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_dacr_filter_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* PGA */
	SND_SOC_DAPM_PGA_E("ADCL_FILTER",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_adcl_filter_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* PGA */
	SND_SOC_DAPM_PGA_E("ADCR_FILTER",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_adcr_filter_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* PGA */
	SND_SOC_DAPM_PGA_E("ADC_MIC3_FILTER",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_adc_mic3_filter_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* PGA */
	SND_SOC_DAPM_PGA_E("HP_LR PGA",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_hplr_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* I2S2 SRC PGA */
	SND_SOC_DAPM_PGA_E("I2S2_RX PGA",
			SND_SOC_NOPM,
			0,
			0, /* not INVERT */
			NULL,
			0,
			da_separate_v2_i2s2_rx_pga_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* VOICE PGA */
	SND_SOC_DAPM_PGA_E("VOICE_DL PGA",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_voice_dl_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* VOICE PGA */
	SND_SOC_DAPM_PGA_E("VOICE_DR PGA",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_voice_dr_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

    /* SIDE TONE PGA */
	SND_SOC_DAPM_PGA_E("SIDE_TONE_EN PGA",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			NULL,
			0,
			da_separate_v2_side_tone_pga_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_PGA("AUDIO_UP_SRC5_EN PGA",
			CODEC_CLK_EN1_REG,
			AUDIO_L_UP_SRCUP_CLKEN_OFFSET,
			0/* not INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("AUDIO_UP_SRC6_EN PGA",
			CODEC_CLK_EN1_REG,
			AUDIO_R_UP_SRCUP_CLKEN_OFFSET,
			0/* not INVERT */,
			NULL,
			0),

	/* PGA VOICE_UPLINK SRC */
	SND_SOC_DAPM_PGA("VOICE_UP_SRC7_EN PGA",
			CODEC_CLK_EN1_REG,
			VOICE_L_UP_SRCDN_CLKEN_OFFSET,
			0/* not INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("VOICE_UP_SRC8_EN PGA",
			CODEC_CLK_EN1_REG,
			VOICE_R_UP_SRCDN_CLKEN_OFFSET,
			0/* not INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("ADC_MIC3_SRC9_EN PGA",
			CODEC_CLK_EN1_REG,
			MIC3_UP_SRCDN_CLKEN_OFFSET,
			1/* not INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("ANA_MAIN PGA",
			CODEC_ANA_RW2_REG,
			CODEC_MAINPGA_PD_OFFSET,
			1/* INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("ANA_AUX PGA",
			CODEC_ANA_RW2_REG,
			CODEC_AUXPGA_PD_OFFSET,
			1/* not INVERT */,
			NULL,
			0),

	SND_SOC_DAPM_PGA("ANA_MIC3 PGA",
			CODEC_ANA_RW2_REG,
			CODEC_MIC3PGA_PD_OFFSET,
			1/* not INVERT */,
			NULL,
			0),

	/* MIXER */
	SND_SOC_DAPM_MIXER("DAC_DL MIXER",
			CODEC_CLK_EN0_REG,
			DACL_MIXER4_CLKEN_OFFSET,
			0/* not INVERT */,
			da_separate_v2_dac_dl_mixer_en_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_dac_dl_mixer_en_mixer_dapm_controls)),

	SND_SOC_DAPM_MIXER("DAC_DR MIXER",
			CODEC_CLK_EN0_REG,
			DACR_MIXER4_CLKEN_OFFSET,
			0/* not INVERT */,
			da_separate_v2_dac_dr_mixer_en_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_dac_dr_mixer_en_mixer_dapm_controls)),


	SND_SOC_DAPM_MIXER("SIF_EN MIXER",
			CODEC_CLK_EN1_REG,
			SIF_MST_CLKEN_OFFSET,
			0/* not INVERT */,
			da_separate_v2_sif_en_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_sif_en_mixer_dapm_controls)),

	/* MIXER EAR */
	SND_SOC_DAPM_MIXER("ANA_EAR MIXER",
			CODEC_ANA_RW6_REG,
			CODEC_MIXOUT_EAR_PD_OFFSET,
			1/* INVERT */,
			da_separate_v2_ear_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_ear_mixer_dapm_controls)),

#ifdef CONFIG_HISI_HI6555V500_PMU
	SND_SOC_DAPM_MIXER("ANA_LINEOUT MIXER",
			CODEC_ANA_RW6_REG,
			LOMIXER_PD_OFFSET,
			1/* INVERT */,
			da_separate_line_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_line_mixer_dapm_controls)),
#endif

	/*S2 MIXER*/
	SND_SOC_DAPM_MIXER("S2_DOWNLINK_EN MIXER",
			CODEC_CLK_EN0_REG,
			BT_TX_MIXER2_CLKEN_OFFSET,
			0/* not INVERT */,
			da_separate_v2_s2_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_s2_mixer_dapm_controls)),

	/*SIL MUX*/
	SND_SOC_DAPM_MUX("I2S1_S1L MUX",
			SND_SOC_NOPM,
			0,
			0/*not INVERT*/,
			&da_separate_v2_i2s1_s1l_mux_dapm_controls),

	/*SIR MUX*/
	SND_SOC_DAPM_MUX("I2S1_S1R MUX",
			SND_SOC_NOPM,
			0,
			0/*not INVERT*/,
			&da_separate_v2_i2s1_s1r_mux_dapm_controls),

	/* MIXER */
	SND_SOC_DAPM_MIXER("ANA_ADC1 MIXER",
			CODEC_ANA_RW3_REG,
			CODEC_ADCL_PD_OFFSET,
			1/* INVERT */,
			da_separate_v2_ana_adc1_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_ana_adc1_mixer_dapm_controls)),

	/* MIXER */
	SND_SOC_DAPM_MIXER("ANA_ADC2 MIXER",
			CODEC_ANA_RW3_REG,
			CODEC_ADCR_PD_OFFSET,
			1/* INVERT */,
			da_separate_v2_ana_adc2_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_ana_adc2_mixer_dapm_controls)),

	/* MIXER */
	SND_SOC_DAPM_MIXER("ANA_ADC3 MIXER",
			CODEC_ANA_RW3_REG,
			CODEC_ADC3_PD_OFFSET,
			1/* INVERT */,
			da_separate_v2_ana_adc3_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_ana_adc3_mixer_dapm_controls)),

	SND_SOC_DAPM_MIXER_E("ANA_SIF_EN MIXER",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_ana_sif_en_mixer_dapm_controls,
			ARRAY_SIZE(da_separate_v2_ana_sif_en_mixer_dapm_controls),
			da_separate_v2_ana_sif_mixer_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_MUX("DAC_DL MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_dacl_mux_dapm_controls),

	SND_SOC_DAPM_MUX("DAC_DR MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_dacr_mux_dapm_controls),

	/* MUX  AUDIO_UP */
	SND_SOC_DAPM_MUX("AUDIO_UL MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_audio_ul_mux_dapm_controls),

	SND_SOC_DAPM_MUX("AUDIO_UR MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_audio_ur_mux_dapm_controls),

	SND_SOC_DAPM_MUX("SIDE_TONE MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_side_tone_mux_dapm_controls),

	/* MUX  VOICE_UP */
	SND_SOC_DAPM_MUX("VOICE_UL MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_voice_ul_mux_dapm_controls),

	SND_SOC_DAPM_MUX("VOICE_UR MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_voice_ur_mux_dapm_controls),

	/* MUX MIC3_UPLINK */
	SND_SOC_DAPM_MUX("MIC3_UP MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_mic3_up_mux_dapm_controls),

	/* MUX  ECHO */
	SND_SOC_DAPM_MUX("ECHO_L MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_echo_l_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ECHO_R MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_echo_r_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ECHO_SRC10 MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_src10_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ECHO_SRC11 MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_src11_mux_dapm_controls),

	SND_SOC_DAPM_MUX("AD_DA_LOOP MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_ad_da_loopback_mux_dapm_controls),

	SND_SOC_DAPM_MUX("DA_AD_LOOP MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_da_ad_loopback_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ADC_LOOP MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_adc_loopback_mux_dapm_controls),

	SND_SOC_DAPM_MUX("DAC_LOOPBACK MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_dac_loopback_mux_dapm_controls),

	/* MUX */
	SND_SOC_DAPM_MUX("ADC1 MUX",
			DAC_FILTER_CTRL_REG,
			SIF_MST_ADCL_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_adc1_mux_dapm_controls),

	/* MUX */
	SND_SOC_DAPM_MUX("ADC2 MUX",
			DAC_FILTER_CTRL_REG,
			SIF_MST_ADCR_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_adc2_mux_dapm_controls),

	/* MUX */
	SND_SOC_DAPM_MUX("ADC3 MUX",
			DAC_FILTER_CTRL_REG,
			SIF_MST_ADC_MIC3_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_adc3_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ANA_DAC_L MUX",
			CLK_EN_CFG_REG,
			DACL_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_ana_dacl_mux_dapm_controls),

	SND_SOC_DAPM_MUX("ANA_DAC_R MUX",
			CLK_EN_CFG_REG,
			DACR_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_ana_dacr_mux_dapm_controls),

	/* MUX MUX3 */
	SND_SOC_DAPM_MUX("ANA_MUX3 MUX",
			CLK_EN_CFG_REG,
			ADCL_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_ana_mux3_mux_dapm_controls),

	/* MUX MUX4 */
	SND_SOC_DAPM_MUX("ANA_MUX4 MUX",
			CLK_EN_CFG_REG,
			ADCR_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_ana_mux4_mux_dapm_controls),

	/* MUX MUX5 */
	SND_SOC_DAPM_MUX("ANA_MUX5 MUX",
			CLK_EN_CFG_REG,
			ADC_MIC3_EN_OFFSET,
			0/* not INVERT */,
			&da_separate_v2_ana_mux5_mux_dapm_controls),

	/* MUX MAIN_HP_MIC */
	SND_SOC_DAPM_MUX("MAIN_HP_MIC MUX",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			&da_separate_v2_main_hp_mic_mux_dapm_controls),

	/* SWITCH AUDIO_UPLINK_FIFO */
	SND_SOC_DAPM_SWITCH_E("AUDIO_UL_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_audio_ul_fifo_switch_dapm_controls,
			da_separate_v2_audio_ul_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_SWITCH_E("AUDIO_UR_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_audio_ur_fifo_switch_dapm_controls,
			da_separate_v2_audio_ur_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),


	/* SWITCH VOICE_UPLINK_FIFO */
	SND_SOC_DAPM_SWITCH_E("VOICE_UL_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_voice_ul_fifo_switch_dapm_controls,
			da_separate_v2_voice_ul_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_SWITCH_E("VOICE_UR_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_voice_ur_fifo_switch_dapm_controls,
			da_separate_v2_voice_ur_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_SWITCH_E("MIC3_UP_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_mic3_up_fifo_switch_dapm_controls,
			da_separate_v2_mic3_up_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* SWITCH ECHO_FIFO */
	SND_SOC_DAPM_SWITCH_E("ECHO_L_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_echo_l_fifo_switch_dapm_controls,
			da_separate_v2_echo_l_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_SWITCH_E("ECHO_R_FIFO SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_echo_r_fifo_switch_dapm_controls,
			da_separate_v2_echo_r_fifo_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),


	SND_SOC_DAPM_SWITCH_E("ANA_DAC_L SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_ana_dacl_switch_dapm_controls,
			da_separate_v2_ana_dacl_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	SND_SOC_DAPM_SWITCH_E("ANA_DAC_R SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_ana_dacr_switch_dapm_controls,
			da_separate_v2_ana_dacr_switch_event,
			(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* HEADSET */
	SND_SOC_DAPM_SWITCH("ANA_HEADSET SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_ana_headset_switch_dapm_controls),

	/* EAR2PGA */
	SND_SOC_DAPM_SWITCH("ANA_SPK SWITCH",
			CTRL_REG_CLASSD_ANA_SPK_SWITCH_REG,
			CTRL_REG_CLASSD_DRV_EN_OFFSET,
			0/* not INVERT */,
			da_separate_v2_ana_spk_switch_dapm_controls),

	SND_SOC_DAPM_SWITCH("ANA_RCV SWITCH",
			CODEC_ANA_RW6_REG,
			CODEC_EAR_VREF_PD_OFFSET,
			1/* INVERT */,
			da_separate_v2_ana_rcv_switch_dapm_controls),

	SND_SOC_DAPM_SWITCH("ANA_LOUT SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_ana_lout_switch_dapm_controls),

	/*I2S1_I2S_TX SWITCH*/
	SND_SOC_DAPM_SWITCH_E("I2S1_I2S_TX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s1_i2s_tx_switch_dapm_controls,
			da_separate_v2_i2s1_i2s_tx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*I2S1_TDM_TX SWITCH*/
	SND_SOC_DAPM_SWITCH_E("I2S1_TDM_TX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s1_tdm_tx_switch_dapm_controls,
			da_separate_v2_i2s1_tdm_tx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_SWITCH("I2S1_CLK SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s1_clk_switch_dapm_controls),

	SND_SOC_DAPM_SWITCH_E("I2S1_I2S_RX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s1_i2s_rx_switch_dapm_controls,
			da_separate_v2_i2s1_i2s_rx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_SWITCH_E("I2S1_TDM_RX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s1_tdm_rx_switch_dapm_controls,
			da_separate_v2_i2s1_tdm_rx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*S2_SRC_I2S2_TX SWITCH*/
	SND_SOC_DAPM_SWITCH_E("S2_SRC_I2S2_TX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_s2_src_i2s2_tx_switch_dapm_controls,
			da_separate_v2_s2_src_i2s2_tx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*S2_SRC_I2S2_TX SWITCH*/
	SND_SOC_DAPM_SWITCH_E("S2_SRC_RX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_s2_src_rx_switch_dapm_controls,
			da_separate_v2_s2_src_rx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*I2S2_CLK SWITCH*/
	SND_SOC_DAPM_SWITCH_E("I2S2_CLK SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s2_clk_switch_dapm_controls,
			da_separate_v2_i2s2_clk_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*3rdCodec_DLR_FIFO_I2S3_TX SWITCH*/
	SND_SOC_DAPM_SWITCH_E("THIRD_CODEC_I2S3_TX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_3rdCodec_dlr_fifo_i2s3_tx_switch_dapm_controls,
			da_separate_v2_3rdCodec_dlr_fifo_i2s3_tx_switch_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/*I2S3_CLK SWITCH*/
	SND_SOC_DAPM_SWITCH("I2S3_CLK SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s3_clk_switch_dapm_controls),

	/*I2S3_RX SWITCH*/
	SND_SOC_DAPM_SWITCH("I2S3_RX SWITCH",
			SND_SOC_NOPM,
			0,
			0/* not INVERT */,
			da_separate_v2_i2s3_rx_switch_dapm_controls),

	/* SUPPLY */
	SND_SOC_DAPM_SUPPLY("SMT_IBIAS SUPPLY",
			SND_SOC_NOPM,
			0,
			0,
			da_separate_v2_smt_ibias_supply_power_mode_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_SUPPLY("PLL",
			SND_SOC_NOPM,
			0,
			0,
			da_separate_v2_pll_supply_power_mode_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	SND_SOC_DAPM_SUPPLY("CLASSD SUPPLY",
			SND_SOC_NOPM,
			0,
			0,
			da_separate_v2_classd_supply_power_mode_event,
			(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),


	SND_SOC_DAPM_MIC("MICBIAS1 MIC", da_separate_v2_micbias1_mic_event),
	SND_SOC_DAPM_MIC("MICBIAS2 MIC", da_separate_v2_micbias2_mic_event),
	SND_SOC_DAPM_MIC("HPMICBIAS MIC", da_separate_v2_hpmicbias_mic_event),
	SND_SOC_DAPM_MIC("DMIC", dmic_event),

	/* OUT DRIVER */
	SND_SOC_DAPM_OUT_DRV_E("ANA_ADC1 DRV",
		CODEC_ANA_RW3_REG,
		CODEC_MUTE_ADCL_OFFSET,
		1/* INVERT */,
		NULL,
		0,
		da_separate_v2_adc_drv_power_mode_event,
		(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* OUT DRIVER */
	SND_SOC_DAPM_OUT_DRV_E("ANA_ADC2 DRV",
		CODEC_ANA_RW3_REG,
		CODEC_MUTE_ADCR_OFFSET,
		1/* INVERT */,
		NULL,
		0,
		da_separate_v2_adc_drv_power_mode_event,
		(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),

	/* OUT DRIVER */
	SND_SOC_DAPM_OUT_DRV_E("ANA_ADC3 DRV",
		CODEC_ANA_RW3_REG,
		CODEC_MUTE_ADC3_OFFSET,
		1/* INVERT */,
		NULL,
		0,
		da_separate_v2_adc_drv_power_mode_event,
		(SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD)),
};

/*****************************************************************************
  Route Map
 *****************************************************************************/
static const struct snd_soc_dapm_route da_separate_v2_route_map[] = {
	/* asp_codec audio_play */
	{"AUDIO_DR PGA",               				NULL,                  			"AUDIO_DLINK INPUT"},
	{"AUDIO_DL PGA",               				NULL,                  			"AUDIO_DLINK INPUT"},
	{"DAC_DL MIXER",            	  			"IN3_AUDIO_DL",  				"AUDIO_DL PGA"},
	{"DAC_DR MIXER",            	  			"IN3_AUDIO_DR",  				"AUDIO_DR PGA"},
	{"DAC_DL MIXER",							"IN4_SIDE_TONE_L",				"SIDE_TONE_EN PGA"},
	{"DAC_DR MIXER",							"IN4_SIDE_TONE_R",				"SIDE_TONE_EN PGA"},
	{"DAC_DL MUX",	            	  			"DACL_MIXER",  					"DAC_DL MIXER"},
	{"DAC_DR MUX",	            	  			"DACR_MIXER",  					"DAC_DR MIXER"},
	{"DACL_FILTER",     	  					NULL,  							"DAC_DL MUX"},
	{"DACR_FILTER",     	  					NULL,  							"DAC_DR MUX"},
	{"AD_DA_LOOP MUX",							"NORMAL",						"DACL_FILTER"},
	{"AD_DA_LOOP MUX",							"NORMAL",						"DACR_FILTER"},
	{"AD_DA_LOOP MUX",							"AD_DA_LOOP",					"DACL_FILTER"},
	{"AD_DA_LOOP MUX",							"AD_DA_LOOP",					"DACR_FILTER"},
	{"AD_DA_LOOP MUX",							"MIC3_DA_LOOP",					"DACL_FILTER"},
	{"AD_DA_LOOP MUX",							"MIC3_DA_LOOP",					"DACR_FILTER"},
	{"SIF_EN MIXER",							"SIF_DACL_EN",					"AD_DA_LOOP MUX"},
	{"SIF_EN MIXER",							"SIF_DACR_EN",					"AD_DA_LOOP MUX"},
	{"AUDIO_DLINK OUTPUT",                      NULL,                         	"SIF_EN MIXER"},

	/* asp_codec - echo */
	{"ECHO_SRC10 MUX",	            			"BYPASS",						"DAC_DL MIXER"},
	{"ECHO_SRC11 MUX",	            			"BYPASS",						"DAC_DR MIXER"},
	{"ECHO_SRC10 MUX",	            			"SRC10",						"DAC_DL MIXER"},
	{"ECHO_SRC11 MUX",	            			"SRC11",						"DAC_DR MIXER"},
	{"ECHO_L MUX",	            				"DACL_MIXER",					"ECHO_SRC10 MUX"},
	{"ECHO_R MUX",	            				"DACR_MIXER",					"ECHO_SRC11 MUX"},
	{"ECHO_L_FIFO SWITCH",	            		"ENABLE",						"ECHO_L MUX"},
	{"ECHO_R_FIFO SWITCH",	            		"ENABLE",						"ECHO_R MUX"},
	{"ECHO OUTPUT",	            				NULL,  							"ECHO_L_FIFO SWITCH"},
	{"ECHO OUTPUT",	            				NULL,  							"ECHO_R_FIFO SWITCH"},

	/* asp_codec - voice downlink */
	{"VOICE_DL PGA",               				NULL,                  			"VOICE_DLINK INPUT"},
	{"VOICE_DR PGA",               				NULL,                  			"VOICE_DLINK INPUT"},
	{"DAC_DL MIXER",            	  			"IN2_VOICE_DL",  				"VOICE_DL PGA"},
	{"DAC_DR MIXER",            	  			"IN2_VOICE_DR",  				"VOICE_DR PGA"},

	/* ana - head_phone audio_play */
	{"ANA_SIF_EN MIXER",						"ANA_SIFL_EN",                 	"SMT_ANA_DAC_SDATA INPUT"},
	{"ANA_SIF_EN MIXER",						"ANA_SIFR_EN",                  "SMT_ANA_DAC_SDATA INPUT"},
	{"ANA_DAC_L MUX",							"SIFL",                    		"ANA_SIF_EN MIXER"},
	{"ANA_DAC_R MUX",							"SIFR",                    		"ANA_SIF_EN MIXER"},
	{"ANA_DAC_L MUX",							"ADC1",							"ANA_MUX3 MUX"},
	{"ANA_DAC_L MUX",							"ADC2",							"ANA_MUX4 MUX"},
	{"ANA_DAC_L MUX",							"ADC3",							"ANA_MUX5 MUX"},
	{"ANA_DAC_R MUX",							"ADC1",							"ANA_MUX3 MUX"},
	{"ANA_DAC_R MUX",							"ADC2",							"ANA_MUX4 MUX"},
	{"ANA_DAC_R MUX",							"ADC3",							"ANA_MUX5 MUX"},
	{"ANA_DAC_L SWITCH",						"DACL",                    		"ANA_DAC_L MUX"},
	{"ANA_DAC_R SWITCH",						"DACR",                    		"ANA_DAC_R MUX"},
	{"ADC_LOOP MUX",							"NORMAL",						"ANA_DAC_L SWITCH"},
	{"ADC_LOOP MUX",							"NORMAL",						"ANA_DAC_R SWITCH"},
	{"ADC_LOOP MUX",							"DA_AD_LOOP",					"ANA_DAC_L SWITCH"},
	{"ADC_LOOP MUX",							"DA_AD_LOOP",					"ANA_DAC_R SWITCH"},
	{"ADC_LOOP MUX",							"DA_MIC3_LOOP",					"ANA_DAC_L SWITCH"},
	{"ADC_LOOP MUX",							"DA_MIC3_LOOP",					"ANA_DAC_R SWITCH"},
	{"HP_LR PGA",								NULL,                    		"ADC_LOOP MUX"},
	{"ANA_HEADSET SWITCH",						"ENABLE",                  		"HP_LR PGA"},
	{"SMT_HP_LR OUTPUT",						NULL,                    		"ANA_HEADSET SWITCH"},

	/* ana - ear audio_play */
	{"ANA_EAR MIXER",							"DACL",                    		"ADC_LOOP MUX"},
	{"ANA_EAR MIXER",							"DACR",                    		"ADC_LOOP MUX"},
#ifdef CONFIG_HISI_HI6555V500_PMU
	{"ANA_LINEOUT MIXER",						"DACL",                    		"ADC_LOOP MUX"},
	{"ANA_LINEOUT MIXER",						"DACR",                    		"ADC_LOOP MUX"},
	{"ANA_SPK SWITCH",							"ENABLE",                  		"ANA_LINEOUT MIXER"},
	{"ANA_RCV SWITCH",							"ENABLE",                  		"ANA_EAR MIXER"},
	{"ANA_LOUT SWITCH",							"ENABLE",              			"ANA_LINEOUT MIXER"},
#else
	{"ANA_SPK SWITCH",							"ENABLE",                  		"ANA_EAR MIXER"},
	{"ANA_RCV SWITCH",							"ENABLE",                  		"ANA_EAR MIXER"},
	{"ANA_LOUT SWITCH",							"ENABLE",              			"ANA_EAR MIXER"},
#endif
	{"ANA_EAR OUTPUT",							NULL,                  			"ANA_SPK SWITCH"},
	{"ANA_EAR OUTPUT",							NULL,                  			"ANA_RCV SWITCH"},
	{"ANA_EAR OUTPUT",							NULL,                  			"ANA_LOUT SWITCH"},

	/* asp_codec - capture */
	{"SIF_EN MIXER",                          	"SIF_ADCL_EN",                  "ADC1 INPUT"},
	{"SIF_EN MIXER",                          	"SIF_ADCR_EN",                  "ADC2 INPUT"},
	{"DA_AD_LOOP MUX",							"NORMAL",						"SIF_EN MIXER"},
	{"DA_AD_LOOP MUX",							"DAP_ADP_LOOP",					"SIF_EN MIXER"},
	{"DA_AD_LOOP MUX",							"DAS_ADS_LOOP",					"SIF_EN MIXER"},
	{"ADC1 MUX",								"ADC1",							"DA_AD_LOOP MUX"},
	{"ADC2 MUX",								"ADC2",							"DA_AD_LOOP MUX"},
	{"ADCL_FILTER",             				NULL,                  			"ADC1 MUX"},
	{"ADCR_FILTER",            		 			NULL,                  			"ADC2 MUX"},
	{"AUDIO_UL MUX",            				"ADCL",                  		"ADCL_FILTER"},
	{"AUDIO_UR MUX",             				"ADCR",                  		"ADCR_FILTER"},
	{"AUDIO_UL MUX",							"DACL_MIXER",					"DAC_DL MUX"},
	{"AUDIO_UR MUX",							"DACR_MIXER",					"DAC_DR MUX"},
	{"SIDE_TONE MUX",							"ADCL_I",						"ADCL_FILTER"},
	{"SIDE_TONE MUX",							"ADCR_I",						"ADCL_FILTER"},
	{"SIDE_TONE MUX",							"ADC_MIC3_I",					"ADCL_FILTER"},
	{"SIDE_TONE_EN PGA",						NULL,							"SIDE_TONE MUX"},
	{"I2S1_S1L MUX",							"ADCL",							"ADCL_FILTER"},
	{"I2S1_S1R MUX",							"ADCR",							"ADCR_FILTER"},
	{"AUDIO_UL_FIFO SWITCH",	            	"ENABLE",  						"AUDIO_UL MUX"},
	{"AUDIO_UR_FIFO SWITCH",	            	"ENABLE",  						"AUDIO_UR MUX"},
	{"AUDIO_UL OUTPUT",	      					NULL,  							"AUDIO_UL_FIFO SWITCH"},
	{"AUDIO_UR OUTPUT",	   				       	NULL,  							"AUDIO_UR_FIFO SWITCH"},

	/* asp_codec - mic3 */
	{"SIF_EN MIXER",                          	"SIF_ADC3_EN",                  "ADC3 INPUT"},
	{"ADC3 MUX",                           		"ADC3",                         "SIF_EN MIXER"},
	{"ADC_MIC3_FILTER",             			NULL,                  			"ADC3 MUX"},
	{"ADC_MIC3_SRC9_EN PGA",             		NULL,                 			"ADC_MIC3_FILTER"},
	{"MIC3_UP MUX",             				"ADC MIC3_SRC",                 "ADC_MIC3_SRC9_EN PGA"},
	{"MIC3_UP_FIFO SWITCH",	            		"ENABLE",  						"MIC3_UP MUX"},
	{"MIC3_UP OUTPUT",	   				       	NULL,  							"MIC3_UP_FIFO SWITCH"},

	/* asp_codec voice_uplink */
	{"VOICE_UP_SRC7_EN PGA",             		NULL,                  			"ADCL_FILTER"},
	{"VOICE_UP_SRC8_EN PGA",             		NULL,                  			"ADCR_FILTER"},
	{"VOICE_UL MUX",            	  			"ADCL_SRC",  					"VOICE_UP_SRC7_EN PGA"},
	{"VOICE_UR MUX",            	  			"ADCR_SRC",  					"VOICE_UP_SRC8_EN PGA"},
	{"VOICE_UL_FIFO SWITCH",	            	"ENABLE",  						"VOICE_UL MUX"},
	{"VOICE_UR_FIFO SWITCH",	            	"ENABLE",  						"VOICE_UR MUX"},
	{"VOICE_UL OUTPUT",	            			NULL,  							"VOICE_UL_FIFO SWITCH"},
	{"VOICE_UR OUTPUT",	            			NULL,  							"VOICE_UR_FIFO SWITCH"},

	/* ana - main\aux */
	{"MAINMIC INPUT",             				NULL,                  			"MICBIAS1 MIC"},
	{"AUXMIC INPUT",             				NULL,                  			"MICBIAS1 MIC"},
	{"MAIN_HP_MIC MUX",							"MAIN_MIC",						"MAINMIC INPUT"},
	{"ANA_MAIN PGA",             				NULL,                  			"MAIN_HP_MIC MUX"},
	{"ANA_AUX PGA",             				NULL,                  			"AUXMIC INPUT"},
	{"ANA_ADC1 MIXER",							"DACL",							"ANA_MAIN PGA"},
	{"ANA_ADC1 MIXER",							"DACR",							"ANA_MAIN PGA"},
	{"ANA_ADC1 MIXER",							"MAIN MIC",						"ANA_MAIN PGA"},
	{"ANA_ADC1 MIXER",							"AUX MIC",						"ANA_AUX PGA"},
	{"ANA_ADC2 MIXER",							"DACL",							"ANA_MAIN PGA"},
	{"ANA_ADC2 MIXER",							"DACR",							"ANA_MAIN PGA"},
	{"ANA_ADC2 MIXER",             				"MAIN MIC",                  	"ANA_MAIN PGA"},
	{"ANA_ADC2 MIXER",             				"AUX MIC",                  	"ANA_AUX PGA"},
	{"ANA_ADC1 DRV",							NULL,							"ANA_ADC1 MIXER"},
	{"ANA_ADC2 DRV",							NULL,							"ANA_ADC2 MIXER"},
	{"ANA_MUX3 MUX",							"ADC1",							"ANA_ADC1 DRV"},
	{"ANA_MUX4 MUX",							"ADC2",							"ANA_ADC2 DRV"},
    {"ANA_MUX3 MUX",							"ADC1",							"ADC_LOOP MUX"},
	{"ANA_MUX4 MUX",							"ADC2",							"ADC_LOOP MUX"},
	{"ANA_SIF_EN MIXER",						"ANA_MUX3_EN",					"ANA_MUX3 MUX"},
	{"ANA_SIF_EN MIXER",						"ANA_MUX4_EN",					"ANA_MUX4 MUX"},
	{"ANA_SIF OUTPUT",             				NULL,                  			"ANA_SIF_EN MIXER"},

	/* ana - mic3 */
	{"MIC3 INPUT",             					NULL,                  			"MICBIAS2 MIC"},
	{"ANA_MIC3 PGA",             				NULL,                  			"MIC3 INPUT"},
	{"ANA_ADC3 MIXER",             				"MIC3",                  		"ANA_MIC3 PGA"},
	{"ANA_ADC3 DRV",							NULL,							"ANA_ADC3 MIXER"},
	{"ANA_MUX5 MUX",             				"ADC3",                  		"ANA_ADC3 DRV"},
	{"ANA_SIF_EN MIXER",             			"ANA_MUX5_EN",                	"ANA_MUX5 MUX"},

	/* ana- headphone */
	{"HPMIC INPUT",             				NULL,                  			"HPMICBIAS MIC"},
	{"MAIN_HP_MIC MUX",             			"HP_MIC",                  		"HPMIC INPUT"},

	/* pll */
	{"DAC_DL MIXER",							NULL,                    		"PLL"},
	{"DAC_DR MIXER",							NULL,                    		"PLL"},

	{"AUDIO_UL_FIFO SWITCH",					NULL,                    		"PLL"},
	{"AUDIO_UR_FIFO SWITCH",					NULL,                    		"PLL"},
	{"MIC3_UP_FIFO SWITCH",						NULL,                    		"PLL"},
	{"I2S3_CLK SWITCH",							NULL,                    		"PLL"},
	{"ANA_SIF_EN MIXER",						NULL,                    		"SMT_IBIAS SUPPLY"},
	{"ANA_SPK SWITCH",							NULL,                    		"CLASSD SUPPLY"},

	/*I2s1_audio_voice_down_link i2s mode */
	{"I2S1_S1L MUX",							"DACL_MIXER",					"DAC_DL MUX"},
	{"I2S1_S1R MUX",							"DACR_MIXER",					"DAC_DR MUX"},
	{"I2S1_I2S_TX SWITCH",						"I2S_SWITCH_EN",				"I2S1_S1L MUX"},
	{"I2S1_I2S_TX SWITCH",						"I2S_SWITCH_EN",				"I2S1_S1R MUX"},
	{"I2S1_CLK SWITCH",							"I2S1_CLK_EN",					"I2S1_I2S_TX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S1_CLK SWITCH"},

	/*I2s1_audio_voice_down_link tdm mode */
	{"I2S1_TDM_TX SWITCH",						"TDM_SWITCH_EN",				"I2S1_S1L MUX"},
	{"I2S1_TDM_TX SWITCH",						"TDM_SWITCH_EN",				"I2S1_S1R MUX"},
	{"I2S1_CLK SWITCH",							"I2S1_CLK_EN",					"I2S1_TDM_TX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S1_CLK SWITCH"},

	/*I2S1 i2s smartpa feedback*/
	{"I2S1_I2S_RX SWITCH",						"I2S1_I2S_RX_EN",				"I2S1_2_3 INPUT"},
	{"I2S1_CLK SWITCH",							"I2S1_CLK_EN",					"I2S1_I2S_RX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S1_CLK SWITCH"},

	/*I2S1 tdm smartpa feedback*/
	{"I2S1_TDM_RX SWITCH",						"I2S1_TDM_RX_EN",				"I2S1_2_3 INPUT"},
	{"I2S1_CLK SWITCH",							"I2S1_CLK_EN",					"I2S1_TDM_RX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S1_CLK SWITCH"},

	/*3rdCodec i2s3 downlink*/
	{"THIRD_CODEC_I2S3_TX SWITCH",				"I2S3_SWITCH_EN",				"I2S1_2_3 INPUT"},
	{"I2S3_CLK SWITCH",							"I2S3_CLK_EN",					"THIRD_CODEC_I2S3_TX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S3_CLK SWITCH"},

	/*I2S2 DOWNLINK*/
	{"S2_DOWNLINK_EN MIXER",					"S2_DACL_MIXER",				"DAC_DL MUX"},
	{"S2_DOWNLINK_EN MIXER",					"S2_DACR_MIXER",				"DAC_DR MUX"},
	{"S2_SRC_I2S2_TX SWITCH",					"I2S2_SWITCH_EN",				"S2_DOWNLINK_EN MIXER"},
	{"I2S2_CLK SWITCH",							"I2S2_CLK_EN",					"S2_SRC_I2S2_TX SWITCH"},
	{"I2S1_2_3 OUTPUT",							NULL,							"I2S2_CLK SWITCH"},

	/*I2S2 UPLINK*/
	{"I2S2_CLK SWITCH",							"I2S2_CLK_EN",					"I2S1_2_3 INPUT"},
	{"I2S2_RX PGA",								NULL,							"I2S2_CLK SWITCH"},
	{"VOICE_UL MUX",							"S2_L",							"I2S2_RX PGA"},
	{"VOICE_UR MUX",							"S2_R",							"I2S2_RX PGA"},
	{"DAC_DL MUX",								"S2_L",							"I2S2_RX PGA"},
	{"DAC_DR MUX",								"S2_R",							"I2S2_RX PGA"},
	{"DAC_DL MIXER",				  			"IN1_S2_L",  					"I2S2_RX PGA"},
	{"DAC_DR MIXER",				  			"IN1_S2_R",  					"I2S2_RX PGA"},

	{"AUDIO_UP_SRC5_EN PGA",					NULL,							"I2S2_RX PGA"},
	{"AUDIO_UP_SRC6_EN PGA",					NULL,							"I2S2_RX PGA"},
	{"AUDIO_UL MUX",							"S2_L_SRC",						"AUDIO_UP_SRC5_EN PGA"},
	{"AUDIO_UR MUX",							"S2_R_SRC",						"AUDIO_UP_SRC6_EN PGA"},

	{"S2_SRC_RX SWITCH",						"I2S2_SRC_RX_EN",				"I2S2_RX PGA"},
	{"VOICE_UL MUX",							"S2_L",							"S2_SRC_RX SWITCH"},
	{"VOICE_UR MUX",							"S2_R",							"S2_SRC_RX SWITCH"},
	{"DAC_DL MUX",								"S2_L",							"S2_SRC_RX SWITCH"},
	{"DAC_DR MUX",								"S2_R",							"S2_SRC_RX SWITCH"},
	{"DAC_DL MIXER",							"IN1_S2_L",						"S2_SRC_RX SWITCH"},
	{"DAC_DR MIXER",							"IN1_S2_R",						"S2_SRC_RX SWITCH"},

	{"AUDIO_UP_SRC5_EN PGA",					NULL,							"S2_SRC_RX SWITCH"},
	{"AUDIO_UP_SRC6_EN PGA",					NULL,							"S2_SRC_RX SWITCH"},
	{"AUDIO_UL MUX",							"S2_L_SRC",						"AUDIO_UP_SRC5_EN PGA"},
	{"AUDIO_UR MUX",							"S2_R_SRC",						"AUDIO_UP_SRC6_EN PGA"},

	/*I2S3 UPLINK*/
	{"I2S3_RX SWITCH",							"I2S3_RX_EN",					"I2S1_2_3 INPUT"},
	{"I2S3_CLK SWITCH",							"I2S3_CLK_EN",					"I2S3_RX SWITCH"},
	{"ECHO_L MUX",								"S3_L",							"I2S3_CLK SWITCH"},
	{"ECHO_R MUX",								"S3_R",							"I2S3_CLK SWITCH"},

	/*dac adc loop*/
	{"DAC_LOOPBACK MUX",						"MIC3_SDIN",					"ANA_MUX3 MUX"},
	{"DAC_LOOPBACK MUX",						"MIC3_SDIN",					"ANA_MUX4 MUX"},
	{"DAC_LOOPBACK MUX",						"MIC3_SDIN",					"ANA_MUX5 MUX"},

	{"DAC_LOOPBACK MUX",						"DOUT_SDIN",					"ANA_MUX3 MUX"},
	{"DAC_LOOPBACK MUX",						"DOUT_SDIN",					"ANA_MUX4 MUX"},
	{"DAC_LOOPBACK MUX",						"DOUT_SDIN",					"ANA_MUX5 MUX"},

	{"ANA_SIF_EN MIXER",						NULL,							"DAC_LOOPBACK MUX"},
	{"ANA_SIF_EN MIXER",						NULL,							"DAC_LOOPBACK MUX"},

	/* dmic */
	{"DMIC",									NULL,							"DMIC INPUT"},
	{"ADC1 MUX",								"DMIC_L",						"DMIC"},
	{"ADC2 MUX",								"DMIC_R",						"DMIC"},

};

void audio_codec_mute_pga(bool mute)
{
	volatile unsigned int val = 0;
	struct da_separate_v2_priv *priv = NULL;

	IN_FUNCTION;

	if (!soc_codec) {
		AUDIO_LOGE("parameter is NULL\n");
		return;
	}

	priv = snd_soc_component_get_drvdata(soc_codec);
	if (!priv) {
		AUDIO_LOGE("priv is NULL\n");
		return;
	}

	if(CLASSD_SUPPLY_SCHARGER != priv->classd_supply_type) {
		AUDIO_LOGI("classd supply type is %d, need not mute/umute\n", priv->classd_supply_type);
		return;
	}

	if (mute) {
		/*mute audio_dlr_mixer*/
		val = da_separate_v2_reg_read(soc_codec, DACL_MIXER4_CTRL0_REG);
		if (INVALID_REG_VALUE != val) {
			priv->mute_dacl_reg_val = val;
			da_separate_v2_reg_update(DACL_MIXER4_CTRL0_REG, MUTE_DAC_VALUE_MASK, MUTE_DAC_VALUE);
		} else {
			AUDIO_LOGE("read dacl reg fail\n");
		}

		val = da_separate_v2_reg_read(soc_codec, DACR_MIXER4_CTRL0_REG);
		if (INVALID_REG_VALUE != val) {
			priv->mute_dacr_reg_val = val;
			da_separate_v2_reg_update(DACR_MIXER4_CTRL0_REG, MUTE_DAC_VALUE_MASK, MUTE_DAC_VALUE);
		} else {
			AUDIO_LOGE("read dacl reg fail\n");
		}

		AUDIO_LOGI("mute audio_dlr_mixer for flash driver\n");
	} else {
		/*unmute audio_dlr_mixer*/
		if (INVALID_REG_VALUE != priv->mute_dacl_reg_val) {
			da_separate_v2_reg_update(DACL_MIXER4_CTRL0_REG, MUTE_DAC_VALUE_MASK, priv->mute_dacl_reg_val);
			priv->mute_dacl_reg_val = INVALID_REG_VALUE;
		}

		if (INVALID_REG_VALUE != priv->mute_dacr_reg_val) {
			da_separate_v2_reg_update(DACR_MIXER4_CTRL0_REG, MUTE_DAC_VALUE_MASK, priv->mute_dacr_reg_val);
			priv->mute_dacr_reg_val = INVALID_REG_VALUE;
		}

		AUDIO_LOGI("unmute audio_dlr_mixer for flash driver\n");
	}

	return;
}
EXPORT_SYMBOL_GPL(audio_codec_mute_pga);

static void _da_separate_v2_reset_pmu_codec(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	/* smartstar codec reset */
	da_separate_v2_reg_write(codec, CTRL_REG_PMU_SOFT_RST_REG, 0x22);

#ifdef CONFIG_HISI_HI6555V500_PMU
	usleep_range(10000, 11000);
#else
	msleep(10);
#endif

	da_separate_v2_reg_write(codec, CTRL_REG_PMU_SOFT_RST_REG, 0x2F);

#ifdef CONFIG_HISI_HI6555V500_PMU
	snd_soc_component_write(codec, CTRL_REG_DIG_IO_DS_SEL1_CODEC_CFG_REG, 0x0);
#else
	da_separate_v2_reg_write(codec, CODEC_ANA_RW40_REG, 0x75);
#endif

	OUT_FUNCTION;
}

static void _da_separate_v2_reset_asp_codec(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	da_separate_v2_reg_write(codec, R_RST_CTRLEN, BIT(RST_EN_CODEC_N));
	da_separate_v2_reg_write(codec, R_RST_CTRLDIS, BIT(RST_DISEN_CODEC_N));
	da_separate_v2_reg_write(codec, R_GATE_EN, BIT(GT_CODEC_CLK));
	da_separate_v2_set_reg_bits(R_CODEC_DMA_SEL, 1 << CODEC_DMA_SEL);

	OUT_FUNCTION;
}

static void _da_separate_v2_asp_reg_init(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	_da_separate_v2_reset_asp_codec(codec);

	/* disable mixer */
	da_separate_v2_set_reg_bits(DACL_MIXER4_CTRL0_REG,
						(1 << DACL_MIXER4_IN1_MUTE_OFFSET)
					   |(1 << DACL_MIXER4_IN2_MUTE_OFFSET)
					   |(1 << DACL_MIXER4_IN3_MUTE_OFFSET)
					   |(1 << DACL_MIXER4_IN4_MUTE_OFFSET));

	da_separate_v2_set_reg_bits(DACR_MIXER4_CTRL0_REG,
						(1 << DACR_MIXER4_IN1_MUTE_OFFSET)
					   |(1 << DACR_MIXER4_IN2_MUTE_OFFSET)
					   |(1 << DACR_MIXER4_IN3_MUTE_OFFSET)
					   |(1 << DACR_MIXER4_IN4_MUTE_OFFSET));

	da_separate_v2_clr_reg_bits(CODEC_ANA_RW12_REG,
						(1 << CODEC_ADCL_MIXIN_DACL_OFFSET)
					   |(1 << CODEC_ADCL_MIXIN_DACR_OFFSET)
					   |(1 << CODEC_ADCL_MIXIN_MIC_PGA1_OFFSET)
					   |(1 << CODEC_ADCL_MIXIN_MIC_PGA2_OFFSET));

	da_separate_v2_clr_reg_bits(CODEC_ANA_RW14_REG,
						(1 << CODEC_ADCR_MIXIN_DACL_OFFSET)
					   |(1 << CODEC_ADCR_MIXIN_DACR_OFFSET)
					   |(1 << CODEC_ADCR_MIXIN_MIC_PGA1_OFFSET)
					   |(1 << CODEC_ADCR_MIXIN_MIC_PGA2_OFFSET));

	da_separate_v2_clr_reg_bits(CODEC_ANA_RW16_REG,
						(1 << CODEC_ADC3_MIXIN_DACL_OFFSET)
					   |(1 << CODEC_ADC3_MIXIN_DACR_OFFSET)
					   |(1 << CODEC_ADC3_MIXIN_MIC_PGA1_OFFSET)
					   |(1 << CODEC_ADC3_MIXIN_MIC_PGA3_OFFSET));

	da_separate_v2_clr_reg_bits(CODEC_ANA_RW20_REG,
						(1 << CODEC_ANA_RW_20_HP_L_DACL_OFFSET)
					   |(1 << CODEC_ANA_RW_20_HP_L_DACR_OFFSET)
					   |(1 << CODEC_ANA_RW_20_HP_R_DACL_OFFSET)
					   |(1 << CODEC_ANA_RW_20_HP_R_DACR_OFFSET));

	da_separate_v2_clr_reg_bits(CODEC_ANA_RW25_REG,
						(1 << CODEC_MIXOUT_EAR_DACL_OFFSET)
					   |(1 << CODEC_MIXOUT_EAR_DACR_OFFSET)
					   |(1 << CODEC_CR_MIXF_DACL_PGA_EN_OFFSET)
					   |(1 << CODEC_CR_MIXF_DACR_PGA_EN_OFFSET));

	da_separate_v2_set_reg_bits(CTRL_REG_CLASSD_MUTE_SEL_REG,
						(1 << CTRL_REG_CLASSD_MUTE_SEL_OFFSET));

	OUT_FUNCTION;
}

static void _da_separate_v2_hsmicbias_force_close(struct snd_soc_component *codec)
{
	unsigned int hs_irq_mask = 0;

	/* to avoid irq while MBHD_COMP power down, mask all COMP irq */
	hs_irq_mask = snd_soc_component_read32(codec, ANA_IRQM_REG0_REG);
	da_separate_v2_set_reg_bits(ANA_IRQM_REG0_REG, hs_irq_mask | IRQ_MSK_COMP);

	/* disable NORMAL key detect and identify */
	da_separate_v2_set_reg_bits(CODEC_ANA_RW8_REG, 0x1 << MBHD_BUFF_PD_OFFSET);
	da_separate_v2_set_reg_bits(CODEC_ANA_RW8_REG, 0x1 << MBHD_COMP_PD_OFFSET);

	/* hs micbias pd */
	da_separate_v2_set_reg_bits(CODEC_ANA_RW7_REG, 0x1 << HSMICB_PD_OFFSET);
	da_separate_v2_set_reg_bits(CODEC_ANA_RW7_REG, 0x1 << HSMICB_DSCHG_OFFSET);
#ifdef CONFIG_HISI_HI6555V500_PMU
	usleep_range(5000, 5500);
#else
	msleep(5);
#endif
	da_separate_v2_clr_reg_bits(CODEC_ANA_RW7_REG, 0x1 << HSMICB_DSCHG_OFFSET);
}

static int da_separate_v2_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	/* Inherit the previous version policy, not implemented */
	return 0;
}

static int da_separate_v2_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	/* Inherit the previous version policy, not implemented */
	return 0;
}

static void da_separate_v2_shutdown(struct platform_device *dev)
{
	struct da_separate_v2_priv *priv = NULL;

	AUDIO_LOGI("Begin\n");

	if (!soc_codec) {
		AUDIO_LOGE("parameter is NULL\n");
		return;
	}

	priv = snd_soc_component_get_drvdata(soc_codec);
	if (!priv) {
		AUDIO_LOGE("priv is NULL\n");
		return;
	}

	/* headset pop process */
	da_separate_v2_headphone_pop_off(soc_codec);

	/* classd pop process */
	da_separate_v2_classd_pop_off(priv);

	/* close hsmicbias */
	_da_separate_v2_hsmicbias_force_close(soc_codec);

	/* close ibias and Avref */
	da_separate_v2_power_codec(soc_codec, false);

	/* reset codec */
	da_separate_v2_reg_write(soc_codec, CTRL_REG_PMU_SOFT_RST_REG, 0x22);

	AUDIO_LOGI("End\n");
}

struct snd_soc_dai_ops da_separate_v2_dai_ops = {
	.startup    = da_separate_v2_startup,
	.hw_params  = da_separate_v2_hw_params,
};

struct snd_soc_dai_driver da_separate_v2_dai[] = {
	{
		.name = "da_separate-dai",
		.playback = {
			.stream_name = "Playback",
			.channels_min = DA_SEPARATE_V2_PB_MIN_CHANNELS,
			.channels_max = DA_SEPARATE_V2_PB_MAX_CHANNELS,
			.rates = DA_SEPARATE_V2_RATES,
			.formats = DA_SEPARATE_V2_FORMATS
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = DA_SEPARATE_V2_CP_MIN_CHANNELS,
			.channels_max = DA_SEPARATE_V2_CP_MAX_CHANNELS,
			.rates = DA_SEPARATE_V2_RATES,
			.formats = DA_SEPARATE_V2_FORMATS
		},
		.ops = &da_separate_v2_dai_ops,
	},
};

static void _set_platform_type(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = NULL;
	struct device_node *np = codec->dev->of_node;
	unsigned int val = 0;

	priv = snd_soc_component_get_drvdata(codec);
	if (!priv) {
		AUDIO_LOGE("priv is null\n");
		return;
	}

	if (!of_property_read_u32(np, "udp_io_config", &val)) {
		AUDIO_LOGI("udp_io_config is %d\n", val);
		if (val == 1) {
			priv->platform_type = UDP_PLATFORM;
		} else {
			priv->platform_type = FPGA_PLATFORM;
		}
	} else {
		AUDIO_LOGI("udp_io_config is not configed, set fpga default\n");
		priv->platform_type = FPGA_PLATFORM;
	}

	return;
}

static int _asp_resource_init(struct snd_soc_component *codec)
{
	int ret = 0;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	priv->asp_regulator = devm_regulator_get(codec->dev, "asp");
	if (IS_ERR(priv->asp_regulator)) {
		AUDIO_LOGE("get asp regulators err:%pK\n", priv->asp_regulator);
		ret = PTR_ERR(priv->asp_regulator);
		goto get_asp_regulator_err;
	}

	priv->asp_subsys_clk = devm_clk_get(codec->dev, "clk_asp_subsys");
	if (IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		AUDIO_LOGE("get clk_asp_subsys err:%pK\n", priv->asp_subsys_clk);
		ret = PTR_ERR(priv->asp_subsys_clk);
		goto get_asp_subsys_clk_err;
	}

	priv->asp_49m_clk = devm_clk_get(codec->dev, "clk_asp_codec");
	if (IS_ERR_OR_NULL(priv->asp_49m_clk)) {
		AUDIO_LOGE("get clk err:%pK\n", priv->asp_49m_clk);
		ret = PTR_ERR(priv->asp_49m_clk);
		goto get_asp_49m_clk_err;
	}

	AUDIO_LOGI("asp resource init ok\n");
	goto end;

get_asp_49m_clk_err:
	priv->asp_49m_clk = NULL;
get_asp_subsys_clk_err:
	priv->asp_subsys_clk = NULL;
get_asp_regulator_err:
	priv->asp_regulator = NULL;
end:
	OUT_FUNCTION;
	return ret;
}

static void _asp_resource_deinit(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);
	priv->asp_regulator = NULL;
	priv->asp_subsys_clk = NULL;
	priv->asp_49m_clk = NULL;

	AUDIO_LOGI("asp resource deinit ok\n");
}

static int _asp_regulator_enable(struct snd_soc_component *codec)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);
	WARN_ON(!priv->asp_regulator);
	WARN_ON(priv->regulator_pu_count < 0);
	WARN_ON(priv->regulator_pu_count > MAX_APS_CLK_COUNT);

	if (priv->regulator_pu_count > 0) {
		priv->regulator_pu_count++;
		AUDIO_LOGI("asp regulator is already enable, regulator_pu_count %d\n", priv->regulator_pu_count);
		return 0;
	}

	ret = regulator_enable(priv->asp_regulator);
	if (ret) {
		AUDIO_LOGE("couldn't enable asp_cfg_regu regulator, ret = %d\n", ret);
		return ret;/*lint !e527*/
	}

	priv->regulator_pu_count++;

	AUDIO_LOGI("asp regulator enable done, regulator_pu_count %d\n", priv->regulator_pu_count);

	OUT_FUNCTION;

	return ret;
}

static void _asp_regulator_disable(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);
	WARN_ON(priv->regulator_pu_count <= 0);

	if (priv->regulator_pu_count > 1) {
		priv->regulator_pu_count--;
		AUDIO_LOGI("asp regulator others in use, regulator_pu_count %d\n", priv->regulator_pu_count);
		return;
	}

	if (!IS_ERR(priv->asp_regulator)) {
		if (regulator_disable(priv->asp_regulator)) {
			AUDIO_LOGE("couldn't disable asp_regulator\n");
		}
		AUDIO_LOGI("asp_regulator disable ok\n");
	}

	priv->regulator_pu_count--;

	AUDIO_LOGI("asp regulator disable done, regulator_pu_count %d\n", priv->regulator_pu_count);

	OUT_FUNCTION;
}

static int _asp_clk_enable(struct snd_soc_component *codec)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);
	WARN_ON(!priv->asp_subsys_clk);
	WARN_ON(!priv->asp_49m_clk);
	WARN_ON(priv->clk_pu_count < 0);

	if (priv->clk_pu_count > 0) {
		priv->clk_pu_count++;
		AUDIO_LOGI("asp_subsys_clk is already enable, clk_pu_count %d\n", priv->clk_pu_count);
		return 0;
	}

	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret) {
		AUDIO_LOGE("asp_subsys_clk :clk prepare enable failed, ret = %d\n", ret);
		return ret;/*lint !e527*/
	}

	ret = clk_prepare_enable(priv->asp_49m_clk);
	if (ret) {
		AUDIO_LOGE("asp_49m_clk clk en fail, ret = %d\n", ret);
		clk_disable_unprepare(priv->asp_subsys_clk);
		return ret;/*lint !e527*/
	}

	priv->clk_pu_count++;
	AUDIO_LOGI("asp clk enable ok, clk_pu_count: %d\n", priv->clk_pu_count);

	OUT_FUNCTION;

	return ret;
}

static void _asp_clk_disable(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(priv->clk_pu_count <= 0);

	if (priv->clk_pu_count > 1) {
		priv->clk_pu_count--;
		AUDIO_LOGI("asp_subsys_clk others in use, clk_pu_count %d\n", priv->clk_pu_count);
		return;
	}

	if (!IS_ERR_OR_NULL(priv->asp_49m_clk)) {
		clk_disable_unprepare(priv->asp_49m_clk);
		AUDIO_LOGI("asp_49m_clk disable ok\n");
	}

	if (!IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		clk_disable_unprepare(priv->asp_subsys_clk);
		AUDIO_LOGI("asp_subsys_clk disable ok\n");
	}

	priv->clk_pu_count--;
	AUDIO_LOGI("asp clk disable ok, clk_pu_count: %d\n", priv->clk_pu_count);

	OUT_FUNCTION;
}

static int _asp_resource_enable(struct snd_soc_component *codec)
{
	int ret;

	IN_FUNCTION;

	ret = _asp_regulator_enable(codec);
	if (ret) {
		AUDIO_LOGE("couldn't enable asp regulator, ret = %d\n", ret);
		goto end;
	}

	ret = _asp_clk_enable(codec);
	if (ret) {
		AUDIO_LOGE("asp clk enable failed, ret = %d\n", ret);
		goto clk_enable_err;
	}

	AUDIO_LOGI("asp resource enable ok\n");

	goto end;

clk_enable_err:
	_asp_regulator_disable(codec);

end:
	OUT_FUNCTION;

	return ret;
}

static void _asp_resource_disable(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	_asp_regulator_disable(codec);
	_asp_clk_disable(codec);

	OUT_FUNCTION;
}

static int _pmu_resource_init(struct snd_soc_component *codec)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);
	struct device_node *np = codec->dev->of_node;

	IN_FUNCTION;

	if (of_property_read_bool(np, "not_support_ldo8")) {
		priv->is_not_support_ldo8 = true;
		AUDIO_LOGI("pmu resource not support ldo8, pmu resource init ok\n");
		return 0;
	}

	priv->codec_ldo8 = devm_regulator_get(codec->dev, "codec_ldo8");
	if (IS_ERR(priv->codec_ldo8)) {
		AUDIO_LOGE("couldn't get ldo8 regulator ret:%ld\n", PTR_ERR(priv->codec_ldo8));
		ret = PTR_ERR(priv->codec_ldo8);
		priv->codec_ldo8 = NULL;
		return ret;
	}

	ret = regulator_enable(priv->codec_ldo8);
	if (ret) {
		AUDIO_LOGE("couldn't enable ldo8 regulators %d\n", ret);
		return ret;/*lint !e527*/
	}

	AUDIO_LOGI("pmu resource init ok\n");

	OUT_FUNCTION;

	return 0;
}

static void _pmu_resource_deinit(struct snd_soc_component *codec)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	if (priv->is_not_support_ldo8) {
		AUDIO_LOGI("not support ldo8, pmu resource deinit ok\n");
		return;
	}

	if (!IS_ERR(priv->codec_ldo8)) {
		ret = regulator_disable(priv->codec_ldo8);
		if (ret) {
			AUDIO_LOGE("couldn't disable ldo8 %d\n", ret);
		}
	}

	AUDIO_LOGI("pmu resource deinit ok\n");
}

static int _pinctrl_init(struct snd_soc_component *codec)
{
	int ret;
	struct device *dev = codec->dev;
	struct da_separate_v2_priv *priv;
	priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	/* I2S1 & I2S2 & SIF & claasd bst_5v_en iomux config */
	priv->pctrl = pinctrl_get(dev);
	if (IS_ERR(priv->pctrl)) {
		priv->pctrl = NULL;
		AUDIO_LOGE("could not get pinctrl\n");
		return -EFAULT;
	}

	priv->pin_default = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(priv->pin_default)) {
		AUDIO_LOGE("could not get default state (%li)\n", PTR_ERR(priv->pin_default));
		priv->pin_default = NULL;
		goto pinctrl_operation_err;
	}

	priv->pin_idle = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(priv->pin_idle)) {
		AUDIO_LOGE("could not get idle state (%li)\n", PTR_ERR(priv->pin_idle));
		priv->pin_idle = NULL;
		goto pinctrl_operation_err;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("could not set pins to default state, ret=%d\n", ret);
		goto pinctrl_operation_err;
	}

	AUDIO_LOGI("pinctrl init ok!\n");

	OUT_FUNCTION;

	return 0;

pinctrl_operation_err:
	pinctrl_put(priv->pctrl);
	priv->pctrl = NULL;
	return -EFAULT;
}

static void _pinctrl_deinit(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	pinctrl_put(priv->pctrl);
	priv->pctrl = NULL;
	AUDIO_LOGI("pinctrl deinit ok\n");

	OUT_FUNCTION;
}

static void _classd_schg_init(struct da_separate_v2_priv *priv)
{
	struct snd_soc_component *codec = priv->codec;
	struct device *dev  = NULL;

	IN_FUNCTION;
	WARN_ON(!codec);
	dev = codec->dev;

	priv->regulator_schg_boost3 = devm_regulator_get(dev, "schg_boost3");
	if (IS_ERR(priv->regulator_schg_boost3)) {
		AUDIO_LOGE("get regulator_schg_boost3 failed. error:%ld\n", PTR_ERR(priv->regulator_schg_boost3));
		return;
	}

	AUDIO_LOGI("get regulator_schg_boost3 ok\n");
	OUT_FUNCTION;
}

static void _classd_schg_deinit(struct da_separate_v2_priv *priv)
{
	IN_FUNCTION;

	if (IS_ERR(priv->regulator_schg_boost3)) {
		AUDIO_LOGE("regulator_schg_boost3 is error:%ld\n", PTR_ERR(priv->regulator_schg_boost3));
		return;
	}

	devm_regulator_put(priv->regulator_schg_boost3);
	priv->regulator_schg_boost3 = NULL;
	AUDIO_LOGI("put regulator_schg_boost3 ok\n");
	OUT_FUNCTION;
}

static void _classd_gpio_init(struct da_separate_v2_priv *priv)
{
	int ret;
	unsigned int val = ARCH_NR_GPIOS;
	struct snd_soc_component *codec = priv->codec;
	struct device *dev  = NULL;
	struct device_node *np  = NULL;

	IN_FUNCTION;
	WARN_ON(!codec);
	WARN_ON(!codec->dev);
	WARN_ON(!codec->dev->of_node);
	dev = codec->dev;
	np = dev->of_node;

	priv->gpio_classd = ARCH_NR_GPIOS;
	ret = of_property_read_u32(np, "gpio_classd", &val);
	if (ret) {
		AUDIO_LOGE("get gpio_classd failed. error:%d\n", ret);
		return;
	}

	if (!gpio_is_valid(val)) {
		AUDIO_LOGE("classd gpio:%d is invalied\n", val);
		return;
	}

	if (gpio_request(val, "classd_gpio")) {
		AUDIO_LOGE("request classd_gpio:%d failed\n", val);
		return;
	}

	priv->gpio_classd = val;
	if (gpio_direction_output(priv->gpio_classd, GPIO_LEVEL_LOW)) {
		AUDIO_LOGE("classd_gpio:%d pull down failed\n", val);
		return;
	}

	AUDIO_LOGI("classd gpio init ok\n");
	OUT_FUNCTION;
}

static void _classd_gpio_deinit(struct da_separate_v2_priv *priv)
{
	IN_FUNCTION;

	if (!gpio_is_valid(priv->gpio_classd)) {
		AUDIO_LOGW("classd gpio is invalied\n");
		return;
	}

	gpio_free(priv->gpio_classd);
	priv->gpio_classd = ARCH_NR_GPIOS;

	AUDIO_LOGI("classd gpio deinit ok\n");
	OUT_FUNCTION;
	return;
}

static void _classd_supply_init(struct snd_soc_component *codec)
{
	enum da_separate_v2_classd_supply_type supply_type;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;
	WARN_ON(!priv);

	supply_type = _get_classd_supply_type(codec);
	switch (supply_type) {
	case CLASSD_SUPPLY_SCHARGER:
		_classd_schg_init(priv);
		break;
	case CLASSD_SUPPLY_GPIO:
		_classd_gpio_init(priv);
		break;
	default:
		AUDIO_LOGW("classd supply init fail. error type is %d!", supply_type);
		break;
	}

	priv->classd_supply_type = supply_type;
	OUT_FUNCTION;
}

static void _classd_supply_deinit(struct snd_soc_component *codec)
{
	int supply_type;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;
	WARN_ON(!priv);

	supply_type = priv->classd_supply_type;
	switch (supply_type) {
	case CLASSD_SUPPLY_SCHARGER:
		_classd_schg_deinit(priv);
		break;
	case CLASSD_SUPPLY_GPIO:
		_classd_gpio_deinit(priv);
		break;
	default:
		AUDIO_LOGW("classd supply deinit fail. error type is %d!", supply_type);
		break;
	}

	OUT_FUNCTION;
}

static void _pm_runtime_init(struct snd_soc_component *codec)
{
	struct device_node *np = codec->dev->of_node;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);
	if (of_property_read_bool(np, "pm_runtime_support")) {
		priv->pm_runtime_support = true;
	} else {
		priv->pm_runtime_support = false;
	}

	AUDIO_LOGI("pm_runtime_support:%d\n", priv->pm_runtime_support);

	if (priv->pm_runtime_support) {
		pm_runtime_use_autosuspend(codec->dev);
		pm_runtime_set_autosuspend_delay(codec->dev, 200); /* 200ms */
		pm_runtime_set_active(codec->dev);
		pm_runtime_enable(codec->dev);
	}

	OUT_FUNCTION;
}

static void _pm_runtime_deinit(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	WARN_ON(!priv);

	if (priv->pm_runtime_support) {
		pm_runtime_resume(codec->dev);
		pm_runtime_disable(codec->dev);
		pm_runtime_set_suspended(codec->dev);
	}

	OUT_FUNCTION;
}

static void _headset_support_init(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);
	struct device_node *np = codec->dev->of_node;

	priv->is_support_headset = true;
	if (of_property_read_bool(np, "is_support_headset")) {
		priv->is_support_headset = false;
		AUDIO_LOGI("headset is not supported\n");
	}
}

static int da_separate_v2_resource_init(struct snd_soc_component *codec)
{
	int ret;
	struct da_separate_v2_priv *priv = NULL;

	IN_FUNCTION;

	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);

	_set_platform_type(codec);

	_headset_support_init(codec);

	ret = _pmu_resource_init(codec);
	if (ret) {
		AUDIO_LOGE("pum resource init error, ret = %d\n", ret);
		goto end;
	}

	ret = _asp_resource_init(codec);
	if (ret) {
		AUDIO_LOGE("asp resource init error, ret = %d\n", ret);
		goto asp_resource_init_err;
	}

	ret = _asp_resource_enable(codec);
	if (ret) {
		AUDIO_LOGE("asp resource enable error, ret = %d\n", ret);
		goto asp_resource_enable_err;
	}

	ret = _pinctrl_init(codec);
	if (ret) {
		AUDIO_LOGE("pinctrl init error, ret=%d\n", ret);
		goto pinctrl_init_err;
	}

	_classd_supply_init(codec);
	_pm_runtime_init(codec);
	_da_separate_v2_reset_pmu_codec(codec);
	_da_separate_v2_asp_reg_init(codec);

	AUDIO_LOGI("resource init ok\n");

	goto end;

pinctrl_init_err:
	_asp_resource_disable(codec);

asp_resource_enable_err:
	_asp_resource_deinit(codec);

asp_resource_init_err:
	_pmu_resource_deinit(codec);

end:
	OUT_FUNCTION;

	return ret;
}

static void da_separate_v2_resource_deinit(struct snd_soc_component *codec)
{
	_pm_runtime_deinit(codec);
	_classd_supply_deinit(codec);
	_pinctrl_deinit(codec);
	_asp_resource_disable(codec);
	_asp_resource_deinit(codec);
	_pmu_resource_deinit(codec);

	AUDIO_LOGI("resource deinit ok\n");

	return;
}

/*lint -e429*/
static int da_separate_v2_priv_init(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = NULL;
	struct device *dev = codec->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		AUDIO_LOGE("priv devm_kzalloc failed\n");
		return -ENOMEM;
	}

	snd_soc_component_set_drvdata(codec, priv);

	soc_codec = codec;
	priv->codec = codec;

	/* virtual codec register init */
	priv->v_codec_reg[0] = 0;
	priv->v_codec_reg[1] = 0;

	priv->regulator_pu_count = 0;
	priv->hs_en_value = 0;
	priv->hs_en_gpio = ARCH_NR_GPIOS;

	priv->mute_dacl_reg_val = INVALID_REG_VALUE;
	priv->mute_dacr_reg_val = INVALID_REG_VALUE;

	priv->classd_scharger_voltage = CLASSD_DEFAULT_VOLTAGE;

	spin_lock_init(&priv->lock);
	mutex_init(&priv->ibias_mutex);

	AUDIO_LOGI("priv init ok\n");

	return 0;
}
/*lint +e429*/

static void da_separate_v2_priv_deinit(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(codec);

	IN_FUNCTION;

	if (priv) {
		mutex_destroy(&priv->ibias_mutex);
		snd_soc_component_set_drvdata(codec, NULL);
	}

	soc_codec = NULL;

	AUDIO_LOGI("priv deinit ok\n");

	OUT_FUNCTION;
}

static int da_separate_v2_hac_init(struct snd_soc_component *codec)
{
	unsigned int val;
	struct device_node *np = codec->dev->of_node;

	if (!of_property_read_u32(np, "hisilicon,hac_gpio", &val)) {
		hac_en_gpio = val;
		AUDIO_LOGI("hac gpio num is %d.\n",hac_en_gpio);
	} else {
		hac_en_gpio = ARCH_NR_GPIOS;
		AUDIO_LOGI("hac is not support.\n");
		return 0;
	}

	if (gpio_request(hac_en_gpio, "hac_en_gpio")) {
		hac_en_gpio = ARCH_NR_GPIOS;
		AUDIO_LOGE("hac_en_gpio request failed\n");
		return -EFAULT;
	}

	if (gpio_direction_output(hac_en_gpio, 0)) {
		gpio_free(hac_en_gpio);
		hac_en_gpio = ARCH_NR_GPIOS;
		AUDIO_LOGE("hac_en_gpio set output failed\n");
		return -EFAULT;
	}

	return 0;
}

static void da_separate_v2_hs_gpio_init(struct snd_soc_component *codec)
{
	unsigned int val;
	struct da_separate_v2_priv *priv = NULL;
	struct device_node *node = NULL;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	priv = snd_soc_component_get_drvdata(codec);
	if (priv == NULL) {
		AUDIO_LOGE("priv is null pointer\n");
		return;
	}

	node = codec->dev->of_node;
	if (node == NULL) {
		AUDIO_LOGE("node is null pointer\n");
		return;
	}

	if (!of_property_read_u32(node, "hisilicon,hs_en_gpio", &val)) {
		AUDIO_LOGI("headset enable gpio is %d\n", val);
	} else {
		AUDIO_LOGI("headset enable gpio is not support\n");
		return;
	}

	if (!gpio_is_valid(val)) {
		AUDIO_LOGE("headset enable gpio is invalid\n");
		return;
	}

	if (gpio_request(val, "hs_en_gpio") != 0) {
		AUDIO_LOGE("headset enable gpio request failed\n");
		return;
	}

	priv->hs_en_gpio = val;

	if (gpio_direction_output(priv->hs_en_gpio, 1) != 0) {
		gpio_free(priv->hs_en_gpio);
		priv->hs_en_gpio = ARCH_NR_GPIOS;
		AUDIO_LOGE("headset enable gpio set output failed\n");
		return;
	}
	priv->hs_en_value = 1;

	AUDIO_LOGI("headset enable gpio init success\n");
}

static int da_separate_v2_codec_probe(struct snd_soc_component *codec)
{
	struct snd_soc_dapm_context *dapm = NULL;
	struct da_separate_v2_priv *priv = NULL;
	struct da_separate_mbhc_ops ops;
	int ret;

	AUDIO_LOGI("Begin\n");

	if (!codec) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	dapm = snd_soc_component_get_dapm(codec);
	if (!dapm) {
		AUDIO_LOGE("dapm is null");
		return -EINVAL;
	}

	ret = da_separate_v2_priv_init(codec);
	if (ret) {
		AUDIO_LOGE("priv init err, ret: %d\n", ret);
		goto end;
	}

	ret = da_separate_v2_resource_init(codec);
	if (ret) {
		AUDIO_LOGE("resource init err, ret: %d\n", ret);
		goto resource_init_err;
	}

	priv = snd_soc_component_get_drvdata(codec);
	WARN_ON(!priv);
	if (priv->is_support_headset) {
		ops.set_ibias_hsmicbias = da_separate_v2_ibias_hsmicbias_enable;
		ops.enable_hsd = da_separate_v2_enable_hsd;
		ret = da_separate_mbhc_init(codec, &priv->mbhc, &ops);
		if (ret) {
			AUDIO_LOGE("mbhc init err, ret: %d\n", ret);
			goto mbhc_init_err;
		}
	}
	ret = snd_soc_dapm_sync(dapm);
	if (ret) {
		AUDIO_LOGE("dapm sync error, errornum = %d\n", ret);
		goto dapm_sync_err;
	}
#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
	ret = hicodec_debug_init(codec, &da_separate_v2_dump_info);
	if (ret) {
		AUDIO_LOGE("debug init error, errornum = %d\n", ret);
		ret = 0;
	}
#endif

	ret = da_separate_v2_hac_init(codec);
	if (ret) {
		AUDIO_LOGE("hac init error, errornum: %d\n", ret);
		goto hac_init_err;
	}

	da_separate_v2_hs_gpio_init(codec);

	goto end;

hac_init_err:
#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
	hicodec_debug_uninit(codec);
#endif

dapm_sync_err:
	if (priv->is_support_headset)
		da_separate_mbhc_deinit(priv->mbhc);

mbhc_init_err:
	da_separate_v2_resource_deinit(codec);

resource_init_err:
	da_separate_v2_priv_deinit(codec);

end:
	AUDIO_LOGI("End\n");

	return ret;
}

static void da_separate_v2_codec_remove(struct snd_soc_component *codec)
{
	struct da_separate_v2_priv *priv = NULL;

	IN_FUNCTION;

	WARN_ON(!codec);
	priv = snd_soc_component_get_drvdata(codec);

#ifdef CONFIG_SND_SOC_HICODEC_DEBUG
	hicodec_debug_uninit(codec);
#endif
	if (priv->is_support_headset)
		da_separate_mbhc_deinit(priv->mbhc);
	da_separate_v2_resource_deinit(codec);
	da_separate_v2_priv_deinit(codec);

	if (hac_en_gpio) {
		gpio_free(hac_en_gpio);
		hac_en_gpio = ARCH_NR_GPIOS;
	}

	if (gpio_is_valid(priv->hs_en_gpio)) {
		gpio_free(priv->hs_en_gpio);
		priv->hs_en_gpio = ARCH_NR_GPIOS;
	}

	OUT_FUNCTION;

	return;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
static struct snd_soc_component_driver soc_codec_dev_da_separate_v2 = {
	.name = DA_SEPARATE_V2_CODEC_NAME,
	.probe    = da_separate_v2_codec_probe,
	.remove  = da_separate_v2_codec_remove,
	.read      = da_separate_v2_reg_read,
	.write     = da_separate_v2_reg_write,
	.controls = da_separate_v2_snd_controls,
	.num_controls = ARRAY_SIZE(da_separate_v2_snd_controls),
	.dapm_widgets = da_separate_v2_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(da_separate_v2_dapm_widgets),
	.dapm_routes = da_separate_v2_route_map,
	.num_dapm_routes = ARRAY_SIZE(da_separate_v2_route_map),
	.idle_bias_off = true
};
#else
static struct snd_soc_codec_driver soc_codec_dev_da_separate_v2 = {
	.read      = da_separate_v2_reg_read_by_codec,
	.write     = da_separate_v2_reg_write_by_codec,
	.component_driver = {
		.name = DA_SEPARATE_V2_CODEC_NAME,
		.probe    = da_separate_v2_codec_probe,
		.remove  = da_separate_v2_codec_remove,
		.controls = da_separate_v2_snd_controls,
		.num_controls = ARRAY_SIZE(da_separate_v2_snd_controls),
		.dapm_widgets = da_separate_v2_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(da_separate_v2_dapm_widgets),
		.dapm_routes = da_separate_v2_route_map,
		.num_dapm_routes = ARRAY_SIZE(da_separate_v2_route_map),
	},
	.idle_bias_off = true
};
#endif

static int da_separate_v2_probe(struct platform_device *pdev)
{
	int ret;
	AUDIO_LOGI("Begin\n");
	WARN_ON(!pdev);

	ret = da_separate_v2_base_addr_map(pdev);
	if (ret) {
		AUDIO_LOGE("base addr map failed! err code 0x%x\n", ret);
		da_separate_v2_base_addr_unmap();
		return ret;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	ret = devm_snd_soc_register_component(&pdev->dev, &soc_codec_dev_da_separate_v2, da_separate_v2_dai, ARRAY_SIZE(da_separate_v2_dai));
#else
	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_da_separate_v2, da_separate_v2_dai, ARRAY_SIZE(da_separate_v2_dai));
#endif
	if (ret) {
		AUDIO_LOGE("register failed! err code 0x%x\n", ret);
		da_separate_v2_base_addr_unmap();
	}

	AUDIO_LOGI("End\n");
	return ret;
}

/* will not be called */
static int da_separate_v2_remove(struct platform_device *pdev)
{
	AUDIO_LOGI("Begin\n");
	WARN_ON(!pdev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
	snd_soc_unregister_codec(&pdev->dev);
#endif
	da_separate_v2_base_addr_unmap();
	AUDIO_LOGI("End\n");

	return 0;
}

static const struct of_device_id da_separate_v2_codec_match[] = {
	{ .compatible = "hisilicon,da_separate_v2-codec", },
	{},
};

#ifdef CONFIG_PM
int da_separate_v2_runtime_suspend(struct device *dev)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	AUDIO_LOGI("begin\n");

	if (!priv) {
		AUDIO_LOGE("get drvdata failed\n");
		return -EINVAL;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
	if (ret) {
		AUDIO_LOGE("could not set pins to idle state, ret = %d\n", ret);
		return ret;
	}

	_asp_clk_disable(soc_codec);

	AUDIO_LOGI("end\n");
	return 0;
}

int da_separate_v2_runtime_resume(struct device *dev)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	AUDIO_LOGI("begin\n");

	if (!priv) {
		AUDIO_LOGE("get drvdata failed\n");
		return -EINVAL;
	}

	ret = _asp_clk_enable(soc_codec);
	if (ret) {
		AUDIO_LOGE("could not enable asp resource, ret = %d\n", ret);
		return ret;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("could not set pins to default state, ret = %d\n", ret);
		return ret;
	}

	AUDIO_LOGI("end\n");
	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static int da_separate_v2_suspend(struct device *dev)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	AUDIO_LOGI("begin\n");

	if (!dev) {
		AUDIO_LOGE("device is null\n");
		return -EINVAL;
	}

	if (!priv) {
		AUDIO_LOGE("get drvdata failed\n");
		return -EINVAL;
	}

	if (priv->pm_runtime_support) {
		ret = pm_runtime_get_sync(dev);
		AUDIO_LOGD("pm suspend ret:%d\n", ret);
		if (ret < 0) {
			AUDIO_LOGE("pm suspend error, ret:%d\n", ret);
			return ret;
		}
	}

	AUDIO_LOGD("suspend usage_count:0x%x status:0x%x disable_depth:%d asp_subsys_clk:%d, asp_49m_clk:%d\n",
			atomic_read(&(dev->power.usage_count)),
			dev->power.runtime_status,
			dev->power.disable_depth,
			clk_get_enable_count(priv->asp_subsys_clk),
			clk_get_enable_count(priv->asp_49m_clk));


	if (!priv->have_dapm) {
		/* set pin to low power mode */
		ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
		if (ret) {
			AUDIO_LOGE("could not set pins to idle state, ret=%d\n", ret);
			return ret;
		}

		_asp_resource_disable(soc_codec);
		priv->asp_pd = true;

		AUDIO_LOGI("suspend without dapm\n");
	}

	AUDIO_LOGI("end\n");
	return 0;
}

static int da_separate_v2_resume(struct device *dev)
{
	int ret;
	struct da_separate_v2_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	AUDIO_LOGI("resume begin\n");

	if (!dev) {
		AUDIO_LOGE("device is null\n");
		return -EINVAL;
	}

	if (!priv) {
		AUDIO_LOGE("get drvdata failed\n");
		return -EINVAL;
	}

	if (priv->asp_pd) {
		ret = _asp_resource_enable(soc_codec);
		if (ret) {
			AUDIO_LOGE("asp resource enable error, ret: %d\n", ret);
			return ret;
		}

		priv->asp_pd = false;

		_da_separate_v2_asp_reg_init(soc_codec);

		ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
		if (ret) {
			AUDIO_LOGE("could not set pins to default state, ret=%d\n", ret);
			return ret;
		}
		AUDIO_LOGI("resume from asp pd\n");
	}

	if (priv->pm_runtime_support) {
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}

	AUDIO_LOGD("resume usage_count:0x%x status:0x%x disable_depth:%d asp_subsys_clk:%d, asp_49m_clk:%d\n",
			atomic_read(&(dev->power.usage_count)),
			dev->power.runtime_status,
			dev->power.disable_depth,
			clk_get_enable_count(priv->asp_subsys_clk),
			clk_get_enable_count(priv->asp_49m_clk));

	AUDIO_LOGI("resume end\n");
	return 0;
}
#endif

struct snd_soc_component *da_separate_v2_get_codec(void)
{
	return soc_codec;
}

const struct dev_pm_ops da_separate_v2_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(da_separate_v2_suspend, da_separate_v2_resume)
	SET_RUNTIME_PM_OPS(da_separate_v2_runtime_suspend, da_separate_v2_runtime_resume, NULL)
};

static struct platform_driver da_separate_v2_driver = {
	.driver = {
		.name  = "da_separate_v2-codec",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(da_separate_v2_codec_match),
		.pm = &da_separate_v2_pm_ops,
	},
	.probe  = da_separate_v2_probe,
	.remove = da_separate_v2_remove,
	.shutdown = da_separate_v2_shutdown,
};

static int __init da_separate_v2_codec_init(void)
{
	AUDIO_LOGI("Begin\n");
	return platform_driver_register(&da_separate_v2_driver);
}
module_init(da_separate_v2_codec_init);

static void __exit da_separate_v2_codec_exit(void)
{
	AUDIO_LOGI("Begin\n");
	platform_driver_unregister(&da_separate_v2_driver);
}
module_exit(da_separate_v2_codec_exit);

MODULE_DESCRIPTION("ASoC da_separate_v2 driver");
MODULE_LICENSE("GPL");

