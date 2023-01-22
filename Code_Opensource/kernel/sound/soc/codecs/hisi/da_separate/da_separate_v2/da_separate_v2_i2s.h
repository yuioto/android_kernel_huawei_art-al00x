/*
 * da_separate_v2 i2s pinctrl driver.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __DA_SEPARATE_V2_I2S_H__
#define __DA_SEPARATE_V2_I2S_H__

enum da_separate_v2_pinctrl_state {
	STATE_DEFAULT = 0,
	STATE_IDLE,
};

struct da_separate_v2_i2s2_priv {
	struct pinctrl *pctrl;
	struct pinctrl_state *pin_default;
	struct pinctrl_state *pin_idle;
};

struct da_separate_v2_i2s_priv {
	struct da_separate_v2_i2s2_priv *i2s2_priv;
};

int da_separate_v2_i2s2_set_pinctrl_default(void);

int da_separate_v2_i2s2_set_pinctrl_idle(void);

#endif /* __DA_SEPARATE_V2_I2S_H__ */
