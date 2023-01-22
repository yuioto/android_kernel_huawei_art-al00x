/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include "hisi_dpe_utils.h"

DEFINE_SEMAPHORE(hisi_fb_dss_inner_clk_sem);

static int dss_inner_clk_refcount = 0;
static unsigned  int g_comform_value = 0;
static unsigned  int g_acm_State = 0;
static unsigned  int g_gmp_State = 0;
static unsigned int g_led_rg_csc_value[9];
static unsigned int g_is_led_rg_csc_set;
unsigned int g_led_rg_para1 = 7;
unsigned int g_led_rg_para2 = 30983;

#define OFFSET_FRACTIONAL_BITS	(11)
#define gmp_cnt_cofe (729)
#define xcc_cnt_cofe (12)

#define CSC_VALUE_MIN_LEN 9


/*lint -e647*/
static int get_lcd_frame_rate(struct dpu_panel_info *pinfo)
{
	return pinfo->pxl_clk_rate/(pinfo->xres + pinfo->pxl_clk_rate_div *
		(pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch + pinfo->ldi.h_pulse_width))/(pinfo->yres +
		pinfo->ldi.v_back_porch + pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width);
}
/*lint +e647*/

bool is_vote_needed_for_low_temp(bool is_lowtemp, int volt_to_set)
{
	DPU_FB_DEBUG("is_lowtemp=%d, volt=%d\n", is_lowtemp, volt_to_set);
	if (is_lowtemp) {
		return false;
	}
	return true;
}

struct dss_vote_cmd * get_dss_vote_cmd(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	int frame_rate = 0;
	uint64_t default_dss_pri_clk_rate;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is null.\n");
		return pdss_vote_cmd;
	}

	pinfo = &(dpufd->panel_info);
	pdss_vote_cmd = &(dpufd->dss_vote_cmd);

	frame_rate = get_lcd_frame_rate(pinfo);

	default_dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE;

	/* FIXME: TBD  */
	if (g_fpga_flag == 1) {
		if (pdss_vote_cmd->dss_pclk_dss_rate == 0) {
			pdss_vote_cmd->dss_pri_clk_rate = 40 * 1000000UL;
			pdss_vote_cmd->dss_pclk_dss_rate = 20 * 1000000UL;
			pdss_vote_cmd->dss_pclk_pctrl_rate = 20 * 1000000UL;
		}
	} else {
		if (pdss_vote_cmd->dss_pclk_dss_rate == 0) {
			if ((pinfo->xres * pinfo->yres) >= (RES_4K_PHONE)) {
				pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
				pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L3;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				dpufd->core_clk_upt_support = 0;
			} else if ((pinfo->xres * pinfo->yres) >= (RES_1440P)) {
				if (frame_rate >= 110) {
					pdss_vote_cmd->dss_pri_clk_rate = DEFAULT_DSS_CORE_CLK_RATE_L3;
					pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L3;
					pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
					pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
					dpufd->core_clk_upt_support = 0;
				} else {
					pdss_vote_cmd->dss_pri_clk_rate = default_dss_pri_clk_rate;
					pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
					pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
					pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
					dpufd->core_clk_upt_support = 1;
				}
			} else if ((pinfo->xres * pinfo->yres) >= (RES_1080P)) {
				pdss_vote_cmd->dss_pri_clk_rate = default_dss_pri_clk_rate;
				pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				dpufd->core_clk_upt_support = 1;
			} else {
				pdss_vote_cmd->dss_pri_clk_rate = default_dss_pri_clk_rate;
				pdss_vote_cmd->dss_mmbuf_rate = DEFAULT_DSS_MMBUF_CLK_RATE_L1;
				pdss_vote_cmd->dss_pclk_dss_rate = DEFAULT_PCLK_DSS_RATE;
				pdss_vote_cmd->dss_pclk_pctrl_rate = DEFAULT_PCLK_PCTRL_RATE;
				dpufd->core_clk_upt_support = 1;
			}
		}
	}

	return pdss_vote_cmd;
}

int set_dss_vote_cmd(struct dpu_fb_data_type *dpufd, dss_vote_cmd_t dss_vote_cmd)
{
	int ret = 0;
	uint64_t tempClkRate = 240000000UL;

	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return -1;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_DEBUG("fb%d is not support!\n", dpufd->index);
		return -1;
	}

	if ((dss_vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_08V_RATE)
		&& (dss_vote_cmd.dss_pri_clk_rate != DEFAULT_DSS_CORE_CLK_07V_RATE)) {
		DPU_FB_ERR("no support set dss_pri_clk_rate(%llu)!\n", dss_vote_cmd.dss_pri_clk_rate);
		return -1;
	}

	if (dss_vote_cmd.dss_pri_clk_rate == dpufd->dss_vote_cmd.dss_pri_clk_rate) {
		return ret;
	}

	if ((dss_vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_07V_RATE)
		&& (dpufd->dss_vote_cmd.dss_pri_clk_rate == DEFAULT_DSS_CORE_CLK_08V_RATE)) {
		ret = clk_set_rate(dpufd->dss_pri_clk, tempClkRate);
		if (ret < 0) {
			DPU_FB_ERR("set dss_pri_clk_rate(240) failed, error = %d! \n", ret);
			return -1;
		} else {
			DPU_FB_INFO("set dss_pri_clk_rate(240) succ! \n");
		}
	}

	ret = clk_set_rate(dpufd->dss_pri_clk, dss_vote_cmd.dss_pri_clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("set dss_pri_clk_rate(%llu) failed, error=%d! \n", dss_vote_cmd.dss_pri_clk_rate, ret);
		return -1;
	} else {
		DPU_FB_INFO("set dss_pri_clk_rate(%llu) succ! \n", dss_vote_cmd.dss_pri_clk_rate);
	}

	dpufd->dss_vote_cmd.dss_pri_clk_rate = dss_vote_cmd.dss_pri_clk_rate;

	return ret;
}

#define PERI_VOLTAGE_LEVEL0_070V		(0) // 0.7v
#define PERI_VOLTAGE_LEVEL1_080V		(2) // 0.8v
int dpe_get_voltage_value(uint32_t dss_voltage_level)
{
	switch (dss_voltage_level) {
	case PERI_VOLTAGE_LEVEL0:
		return PERI_VOLTAGE_LEVEL0_070V; // 0.7v
	case PERI_VOLTAGE_LEVEL1:
		return PERI_VOLTAGE_LEVEL1_080V; // 0.8v
	default:
		DPU_FB_ERR("not support dss_voltage_level is %d \n", dss_voltage_level);
		return -1;
	}
}

int dpe_get_voltage_level(int votage_value)
{
	switch (votage_value) {
	case PERI_VOLTAGE_LEVEL0_070V:
		return PERI_VOLTAGE_LEVEL0;
	case PERI_VOLTAGE_LEVEL1_080V:
		return PERI_VOLTAGE_LEVEL1;
	default:
		DPU_FB_ERR("not support votage_value is %d \n", votage_value);
		return PERI_VOLTAGE_LEVEL0;
	}
}

/*lint -e712 -e838*/
int dpe_set_clk_rate(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	struct dss_vote_cmd *pdss_vote_cmd = NULL;
	uint64_t dss_pri_clk_rate;
	int ret = 0;

	if (pdev == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return -1;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return -1;
	}

	pinfo = &(dpufd->panel_info);
	pdss_vote_cmd = get_dss_vote_cmd(dpufd);
	if (pdss_vote_cmd == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return -1;
	}

	ret = clk_set_rate(dpufd->dss_mmbuf_clk, pdss_vote_cmd->dss_mmbuf_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_mmbuf clk_set_rate(%llu) failed, error=%d!\n",
			dpufd->index, pdss_vote_cmd->dss_mmbuf_rate, ret);
		return -EINVAL;
	}

	dss_pri_clk_rate = pdss_vote_cmd->dss_pri_clk_rate;

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		if (dpufd_list[PRIMARY_PANEL_IDX]) {
			if (dpufd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate > dss_pri_clk_rate) {
				dss_pri_clk_rate = dpufd_list[PRIMARY_PANEL_IDX]->dss_vote_cmd.dss_pri_clk_rate;
			}
		}
	}

	ret = clk_set_rate(dpufd->dss_pri_clk, dss_pri_clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_pri_clk clk_set_rate(%llu) failed, error=%d!\n",
			dpufd->index, dss_pri_clk_rate, ret);
		return -EINVAL;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ret = clk_set_rate(dpufd->dss_pxl0_clk, pinfo->pxl_clk_rate);
		if (ret < 0) {
			DPU_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed, error=%d!\n",
				dpufd->index, pinfo->pxl_clk_rate, ret);
			if (g_fpga_flag == 0) {
				return -EINVAL;
			}
		}
	} else if ((dpufd->index == EXTERNAL_PANEL_IDX) && !dpufd->panel_info.fake_external) {
		DPU_FB_ERR("not support.\n");
	} else {
		;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		DPU_FB_INFO("dss_mmbuf_clk:[%llu]->[%llu].\n",
			pdss_vote_cmd->dss_mmbuf_rate, (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));
		DPU_FB_INFO("dss_pri_clk:[%llu]->[%llu].\n",
			pdss_vote_cmd->dss_pri_clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pri_clk));
		DPU_FB_INFO("dss_pxl0_clk:[%llu]->[%llu].\n",
			pinfo->pxl_clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pxl0_clk));
	}

	return ret;
}
/*lint +e712 +e838*/

int dpe_set_pixel_clk_rate_on_pll0(struct dpu_fb_data_type *dpufd)
{
	return 0;
}

int dpe_set_common_clk_rate_on_pll0(struct dpu_fb_data_type *dpufd)
{
	int ret;
	uint64_t clk_rate;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL Pointer!\n");
		return -EINVAL;
	}

	if (g_fpga_flag == 1) {
		return 0;
	}

	clk_rate = DEFAULT_DSS_MMBUF_CLK_RATE_POWER_OFF;
	ret = clk_set_rate(dpufd->dss_mmbuf_clk, clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_mmbuf clk_set_rate(%llu) failed, error=%d!\n", dpufd->index, clk_rate, ret);
		return -EINVAL;
	}
	DPU_FB_INFO("dss_mmbuf_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_mmbuf_clk));

	clk_rate = DEFAULT_DSS_CORE_CLK_RATE_POWER_OFF;
	ret = clk_set_rate(dpufd->dss_pri_clk, clk_rate);
	if (ret < 0) {
		DPU_FB_ERR("fb%d dss_pri_clk clk_set_rate(%llu) failed, error=%d!\n", dpufd->index, clk_rate, ret);
		return -EINVAL;
	}
	DPU_FB_INFO("dss_pri_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pri_clk));

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		clk_rate = DEFAULT_DSS_PXL0_CLK_RATE_POWER_OFF;
		ret = clk_set_rate(dpufd->dss_pxl0_clk, clk_rate);
		if (ret < 0) {
			DPU_FB_ERR("fb%d dss_pxl0_clk clk_set_rate(%llu) failed, error=%d!\n", dpufd->index, clk_rate, ret);
			return -EINVAL;
		}
		DPU_FB_INFO("dss_pxl0_clk:[%llu]->[%llu].\n", clk_rate, (uint64_t)clk_get_rate(dpufd->dss_pxl0_clk));

	} else if ((dpufd->index == EXTERNAL_PANEL_IDX) && !dpufd->panel_info.fake_external) {
		DPU_FB_ERR("not support.\n");
	} else {
		;
	}
	return ret;
}
/*lint +e712 +e838*/
/*lint -e838 */
void dss_inner_clk_common_enable(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;
	int prev_refcount = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	dss_base = dpufd->dss_base;

	down(&hisi_fb_dss_inner_clk_sem);

	prev_refcount = dss_inner_clk_refcount++;
	if (!prev_refcount && !fastboot_enable) {
		//core/axi/mmbuf
		outp32(dss_base + DSS_CMDLIST_OFFSET + CMD_MEM_CTRL, 0x00000008);  //cmd mem

		if (g_fpga_flag == 1)
			outp32(dss_base + DSS_RCH_VG1_ARSR_OFFSET + ARSR2P_LB_MEM_CTRL, 0x00000000);//rch_v1 ,arsr2p mem
		else
			outp32(dss_base + DSS_RCH_VG1_ARSR_OFFSET + ARSR2P_LB_MEM_CTRL, 0x00000008);//rch_v1 ,arsr2p mem

		outp32(dss_base + DSS_RCH_VG1_SCL_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088);//rch_v1 ,scf mem
		outp32(dss_base + DSS_RCH_VG1_SCL_OFFSET + SCF_LB_MEM_CTRL, 0x00000008);//rch_v1 ,scf mem
		outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + VPP_MEM_CTRL, 0x00000008);//rch_v1 ,vpp mem
		outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_v1 ,dma_buf mem
		outp32(dss_base + DSS_RCH_VG1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888);//rch_v1 ,afbcd mem

		outp32(dss_base + DSS_RCH_G1_SCL_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088);//rch_g1 ,scf mem
		outp32(dss_base + DSS_RCH_G1_SCL_OFFSET + SCF_LB_MEM_CTRL, 0x0000008);//rch_g1 ,scf mem
		outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_g1 ,dma_buf mem
		outp32(dss_base + DSS_RCH_G1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888);//rch_g1 ,afbcd mem

		outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_d0 ,dma_buf mem
		outp32(dss_base + DSS_RCH_D0_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888);//rch_d0 ,afbcd mem
		outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_d1 ,dma_buf mem
		outp32(dss_base + DSS_RCH_D1_DMA_OFFSET + AFBCD_MEM_CTRL, 0x00008888);//rch_d1 ,afbcd mem
		outp32(dss_base + DSS_RCH_D2_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_d2 ,dma_buf mem
		outp32(dss_base + DSS_RCH_D3_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//rch_d3 ,dma_buf mem

		outp32(dss_base + DSS_WCH0_DMA_OFFSET + DMA_BUF_MEM_CTRL, 0x00000008);//wch0 DMA/AFBCE mem
		outp32(dss_base + DSS_WCH0_DMA_OFFSET + AFBCE_MEM_CTRL, 0x00000888);//wch0 DMA/AFBCE mem
		outp32(dss_base + DSS_WCH0_DMA_OFFSET + ROT_MEM_CTRL, 0x00000008);//wch0 rot mem
	}

	DPU_FB_DEBUG("fb%d, dss_inner_clk_refcount=%d\n",
		dpufd->index, dss_inner_clk_refcount);

	up(&hisi_fb_dss_inner_clk_sem);
}

void dss_inner_clk_common_disable(struct dpu_fb_data_type *dpufd)
{
	int new_refcount = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	down(&hisi_fb_dss_inner_clk_sem);
	new_refcount = --dss_inner_clk_refcount;
	if (new_refcount < 0) {
		DPU_FB_ERR("dss new_refcount err");
	}

	if (!new_refcount) {
		;
	}

	DPU_FB_DEBUG("fb%d, dss_inner_clk_refcount=%d\n",
		dpufd->index, dss_inner_clk_refcount);
	up(&hisi_fb_dss_inner_clk_sem);
}

void dss_inner_clk_pdp_enable(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *dss_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	dss_base = dpufd->dss_base;

	if (fastboot_enable)
		return ;

	outp32(dss_base + DSS_LDI0_OFFSET + LDI_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_POST_SCF_OFFSET + SCF_COEF_MEM_CTRL, 0x00000088);
	outp32(dss_base + DSS_POST_SCF_OFFSET + SCF_LB_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DBUF0_OFFSET + DBUF_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DPP_DITHER_OFFSET + DITHER_MEM_CTRL, 0x00000008);
}

void dss_inner_clk_pdp_disable(struct dpu_fb_data_type *dpufd)
{
}

void dss_inner_clk_sdp_enable(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	dss_base = dpufd->dss_base;

	outp32(dss_base + DSS_LDI1_OFFSET + LDI_MEM_CTRL, 0x00000008);
	outp32(dss_base + DSS_DBUF1_OFFSET + DBUF_MEM_CTRL, 0x00000008);
}

void dss_inner_clk_sdp_disable(struct dpu_fb_data_type *dpufd)
{
}

void init_dpp(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dpp_base = NULL;
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dpp_base = dpufd->dss_base + DSS_DPP_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	outp32(dpp_base + DPP_IMG_SIZE_BEF_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));
	outp32(dpp_base + DPP_IMG_SIZE_AFT_SR, (DSS_HEIGHT(pinfo->yres) << 16) | DSS_WIDTH(pinfo->xres));

}
/*lint +e838 */

void init_ifbc(struct dpu_fb_data_type *dpufd)
{
	return;
}


/*lint -e438 -e550 -e838 -e573 -e647*/
void init_post_scf(struct dpu_fb_data_type *dpufd)
{
	char __iomem *scf_lut_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	scf_lut_base = dpufd->dss_base + DSS_POST_SCF_LUT_OFFSET;

	if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_POST_SCF)) {
		return;
	}

	hisi_dss_post_scl_load_filter_coef(dpufd, false, scf_lut_base, SCL_COEF_RGB_IDX);

	return;
}

void init_dbuf(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dbuf_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	int sram_valid_num = 0;
	int sram_max_mem_depth = 0;
	int sram_min_support_depth = 0;

	uint32_t thd_rqos_in = 0;
	uint32_t thd_rqos_out = 0;
	uint32_t thd_wqos_in = 0;
	uint32_t thd_wqos_out = 0;
	uint32_t thd_cg_in = 0;
	uint32_t thd_cg_out = 0;
	uint32_t thd_wr_wait = 0;
	uint32_t thd_cg_hold = 0;
	uint32_t thd_flux_req_befdfs_in = 0;
	uint32_t thd_flux_req_befdfs_out = 0;
	uint32_t thd_flux_req_aftdfs_in = 0;
	uint32_t thd_flux_req_aftdfs_out = 0;
	uint32_t thd_dfs_ok = 0;
	uint32_t dfs_ok_mask = 0;
	uint32_t thd_flux_req_sw_en = 1;

	int dfs_time = 0;
	int dfs_time_min = 0;
	int depth = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dbuf_base = dpufd->dss_base + DSS_DBUF0_OFFSET;
		if (!HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_DBUF)) {
			return;
		}

		if (pinfo->xres * pinfo->yres >= RES_4K_PHONE) {
			dfs_time_min = DFS_TIME_MIN_4K;
		} else {
			dfs_time_min = DFS_TIME_MIN;
		}

		dfs_time = DFS_TIME;
		depth = DBUF0_DEPTH;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		dbuf_base = dpufd->dss_base + DSS_DBUF1_OFFSET;

		dfs_time = DFS_TIME;
		dfs_time_min = DFS_TIME_MIN;
		depth = DBUF1_DEPTH;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	/*
	** int K = 0;
	** int Tp = 1000000  / pinfo->pxl_clk_rate;
	** K = (pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch + pinfo->xres +
	**	pinfo->ldi.h_front_porch) / pinfo->xres;
	** thd_cg_out = dfs_time / (Tp * K * 6);
	*/
	if (pinfo->pxl_clk_rate_div <= 0)
		pinfo->pxl_clk_rate_div = 1;
	/*lint -e712 */
	thd_cg_out = (dfs_time * pinfo->pxl_clk_rate * pinfo->xres) /
		(((pinfo->ldi.h_pulse_width + pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch) * pinfo->pxl_clk_rate_div
		+ pinfo->xres) * 6 * 1000000UL);
	/*lint +e712 */
	sram_valid_num = thd_cg_out / depth;
	thd_cg_in = (sram_valid_num + 1) * depth - 1;

	sram_max_mem_depth = (sram_valid_num + 1) * depth;

	thd_rqos_in = thd_cg_out * 85 / 100;
	thd_rqos_out = thd_cg_out;
	thd_flux_req_befdfs_in = GET_FLUX_REQ_IN(sram_max_mem_depth);
	thd_flux_req_befdfs_out = GET_FLUX_REQ_OUT(sram_max_mem_depth);

	sram_min_support_depth = dfs_time_min * pinfo->xres * pinfo->pxl_clk_rate_div / (1000000 / 60 / (pinfo->yres +
		pinfo->ldi.v_back_porch + pinfo->ldi.v_front_porch + pinfo->ldi.v_pulse_width) * (DBUF_WIDTH_BIT / 3 / BITS_PER_BYTE));

	//thd_flux_req_aftdfs_in   =[(sram_valid_num+1)*depth - 50*HSIZE/((1000000/60/(VSIZE+VFP+VBP+VSW))*6)]/3
	thd_flux_req_aftdfs_in = (sram_max_mem_depth - sram_min_support_depth) / 3;
	//thd_flux_req_aftdfs_out  =  2*[(sram_valid_num+1)* depth - 50*HSIZE/((1000000/60/(VSIZE+VFP+VBP+VSW))*6)]/3
	thd_flux_req_aftdfs_out = 2 * (sram_max_mem_depth - sram_min_support_depth) / 3;

	thd_dfs_ok = thd_flux_req_befdfs_in;

	DPU_FB_DEBUG("sram_valid_num=%d,\n"
		"thd_rqos_in=0x%x\n"
		"thd_rqos_out=0x%x\n"
		"thd_cg_in=0x%x\n"
		"thd_cg_out=0x%x\n"
		"thd_flux_req_befdfs_in=0x%x\n"
		"thd_flux_req_befdfs_out=0x%x\n"
		"thd_flux_req_aftdfs_in=0x%x\n"
		"thd_flux_req_aftdfs_out=0x%x\n"
		"thd_dfs_ok=0x%x\n",
		sram_valid_num,
		thd_rqos_in,
		thd_rqos_out,
		thd_cg_in,
		thd_cg_out,
		thd_flux_req_befdfs_in,
		thd_flux_req_befdfs_out,
		thd_flux_req_aftdfs_in,
		thd_flux_req_aftdfs_out,
		thd_dfs_ok);

    if (g_fpga_flag == 1) {
        thd_flux_req_befdfs_out = 0xacf;
        thd_flux_req_befdfs_in = 0x734;
        thd_flux_req_aftdfs_out = 0x4dc;
        thd_flux_req_aftdfs_in = 0x26e;
        thd_dfs_ok = 0x960;
        dfs_ok_mask = 0;
        thd_rqos_out = 0x9c0;
        thd_rqos_in = 0x898;
        thd_cg_out = 0x9c0;
        thd_cg_in = 0x1780;
    } else {
	sram_valid_num = 1;
	thd_rqos_out = 0x9c0;
	thd_rqos_in = 0x898;
	thd_cg_out = 0x9c0;
	thd_cg_in = 0x1780;
	thd_dfs_ok = 0x960;
	thd_flux_req_befdfs_out = 0xacf;
	thd_flux_req_befdfs_in = 0x734;
	thd_flux_req_aftdfs_out = 0x4dc;
	thd_flux_req_aftdfs_in = 0x26e;
	}
	outp32(dbuf_base + DBUF_FRM_SIZE, pinfo->xres * pinfo->yres);
	outp32(dbuf_base + DBUF_FRM_HSIZE, DSS_WIDTH(pinfo->xres));
	outp32(dbuf_base + DBUF_SRAM_VALID_NUM, sram_valid_num);// 1

	outp32(dbuf_base + DBUF_THD_RQOS, (thd_rqos_out<< 16) | thd_rqos_in); // 9c0 898
	outp32(dbuf_base + DBUF_THD_WQOS, (thd_wqos_out << 16) | thd_wqos_in);// 0,0
	outp32(dbuf_base + DBUF_THD_CG, (thd_cg_out << 16) | thd_cg_in); //9c0  1780
	outp32(dbuf_base + DBUF_THD_OTHER, (thd_cg_hold << 16) | thd_wr_wait); // 0 0
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_BEF, (thd_flux_req_befdfs_out << 16) | thd_flux_req_befdfs_in); //acf 734
	outp32(dbuf_base + DBUF_THD_FLUX_REQ_AFT, (thd_flux_req_aftdfs_out << 16) | thd_flux_req_aftdfs_in); //4dc 26e
	outp32(dbuf_base + DBUF_THD_DFS_OK, thd_dfs_ok); //960
	outp32(dbuf_base + DBUF_FLUX_REQ_CTRL, (dfs_ok_mask << 1) | thd_flux_req_sw_en);//0 1

	outp32(dbuf_base + DBUF_THD_RQOS_IDLE, 0);//0

	outp32(dbuf_base + DBUF_DFS_LP_CTRL, 0x1);

	DPU_FB_DEBUG("-.!\n");
}
/*lint +e438 +e550 +e838 +e573 +e647*/
/*lint -e838 -e568 -e685*/
static void init_ldi_pxl_div(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *ldi_base = NULL;
	uint32_t ifbc_type = 0;
	uint32_t mipi_idx = 0;
	uint32_t pxl0_div2_gt_en = 0;
	uint32_t pxl0_div4_gt_en = 0;
	uint32_t pxl0_divxcfg = 0;
	uint32_t pxl0_dsi_gt_en = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == EXTERNAL_PANEL_IDX)
            return;

	ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;

	ifbc_type = pinfo->ifbc_type;
	if (ifbc_type  >= IFBC_TYPE_MAX) {
		DPU_FB_ERR("ifbc_type is invalid");
		return;
	}

	mipi_idx = is_dual_mipi_panel(dpufd) ? 1 : 0;

	pxl0_div2_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div2_gt_en;
	pxl0_div4_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_div4_gt_en;
	pxl0_divxcfg = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_divxcfg;
	pxl0_dsi_gt_en = g_mipi_ifbc_division[mipi_idx][ifbc_type].pxl0_dsi_gt_en;

	set_reg(ldi_base + LDI_PXL0_DIV2_GT_EN, pxl0_div2_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DIV4_GT_EN, pxl0_div4_gt_en, 1, 0);
	set_reg(ldi_base + LDI_PXL0_GT_EN, 0x1, 1, 0);
	set_reg(ldi_base + LDI_PXL0_DSI_GT_EN, pxl0_dsi_gt_en, 2, 0);
	set_reg(ldi_base + LDI_PXL0_DIVXCFG, pxl0_divxcfg, 3, 0);
}
/*lint +e838 +e568 +e685*/
/*lint -e838 */
void init_ldi(struct dpu_fb_data_type *dpufd, bool fastboot_enable)
{
	char __iomem *ldi_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	dss_rect_t rect = {0,0,0,0};
	uint32_t te_source = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
		if (g_fpga_flag == 1) {
			set_reg(dpufd->dss_base + GLB_TP_SEL, 0x2, 2, 0);
		}
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = pinfo->xres;
	rect.h = pinfo->yres;
	mipi_ifbc_get_rect(dpufd, &rect);

	init_ldi_pxl_div(dpufd);

	if (is_dual_mipi_panel(dpufd)) {
		if (is_mipi_video_panel(dpufd)) {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, (pinfo->ldi.h_back_porch + DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, 0);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		} else {
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL0, pinfo->ldi.h_back_porch << 16);
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
			outp32(ldi_base + LDI_DPI1_HRZ_CTRL2, DSS_WIDTH(rect.w));
		}

		outp32(ldi_base + LDI_OVERLAP_SIZE,
			pinfo->ldi.dpi0_overlap_size | (pinfo->ldi.dpi1_overlap_size << 16));

		/* dual_mode_en */
		set_reg(ldi_base + LDI_CTRL, 1, 1, 5);

		/* split mode */
		set_reg(ldi_base + LDI_CTRL, 0, 1, 16);

		//dual lcd: 0x1, dual mipi: 0x0
		set_reg(dpufd->dss_base + DSS_LDI0_OFFSET + LDI_DSI1_CLK_SEL, 0x0, 1, 0);
	}
	if (is_mipi_video_panel(dpufd)) {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
				pinfo->ldi.h_front_porch | ((pinfo->ldi.h_back_porch + DSS_WIDTH(pinfo->ldi.h_pulse_width)) << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, 0);
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	} else {
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL0,
				pinfo->ldi.h_front_porch | (pinfo->ldi.h_back_porch << 16));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL1, DSS_WIDTH(pinfo->ldi.h_pulse_width));
		outp32(ldi_base + LDI_DPI0_HRZ_CTRL2, DSS_WIDTH(rect.w));
	}
	outp32(ldi_base + LDI_VRT_CTRL0,
		pinfo->ldi.v_front_porch | (pinfo->ldi.v_back_porch << 16));
	outp32(ldi_base + LDI_VRT_CTRL1, DSS_HEIGHT(pinfo->ldi.v_pulse_width));
	outp32(ldi_base + LDI_VRT_CTRL2, DSS_HEIGHT(rect.h));

	outp32(ldi_base + LDI_PLR_CTRL,
		pinfo->ldi.vsync_plr | (pinfo->ldi.hsync_plr << 1) |
		(pinfo->ldi.pixelclk_plr << 2) | (pinfo->ldi.data_en_plr << 3));

	//sensorhub int msk
	//outp32(ldi_base + LDI_SH_MASK_INT, 0x0);

	// bpp
	set_reg(ldi_base + LDI_CTRL, pinfo->bpp, 2, 3);
	// bgr
	set_reg(ldi_base + LDI_CTRL, pinfo->bgr_fmt, 1, 13);

	// for ddr pmqos
	outp32(ldi_base + LDI_VINACT_MSK_LEN,
		pinfo->ldi.v_front_porch);

	//cmd event sel
	outp32(ldi_base + LDI_CMD_EVENT_SEL, 0x1);

	//outp32(ldi_base + LDI_FRM_VALID_DBG, 0x1);

	// for 1Hz LCD and mipi command LCD
	if (is_mipi_cmd_panel(dpufd)) {
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x1, 1, 0);

		//DSI_TE_CTRL
		// te_source = 0, select te_pin
		// te_source = 1, select te_triger
		te_source = 0;

		// dsi_te_hard_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 0);
		// dsi_te0_pin_p , dsi_te1_pin_p
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 2, 1);
		// dsi_te_hard_sel
		set_reg(ldi_base + LDI_DSI_TE_CTRL, te_source, 1, 3);
		// select TE0 PIN
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x01, 2, 6);
		// dsi_te_mask_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 1, 8);
		// dsi_te_mask_dis
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 9);
		// dsi_te_mask_und
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x0, 4, 13);
		// dsi_te_pin_en
		set_reg(ldi_base + LDI_DSI_TE_CTRL, 0x1, 1, 17);

		//TBD:(dsi_te_hs_num+vactive)*htotal/clk_pxl0_div+0.00004<1/60+vs_te_time+(vactive*hotal) /clk_ddic_rd
		set_reg(ldi_base + LDI_DSI_TE_HS_NUM, 0x0, 32, 0);
		set_reg(ldi_base + LDI_DSI_TE_HS_WD, 0x24024, 32, 0);

		// dsi_te0_vs_wd = lcd_te_width / T_pxl_clk, experience lcd_te_width = 2us
		if (pinfo->pxl_clk_rate_div == 0) {
			DPU_FB_ERR("pxl_clk_rate_div is NULL, not support !\n");
			pinfo->pxl_clk_rate_div = 1;
		}
		/*lint -e712 */
		set_reg(ldi_base + LDI_DSI_TE_VS_WD,
			(0x3FC << 12) | (2 * pinfo->pxl_clk_rate / pinfo->pxl_clk_rate_div / 1000000), 32, 0);
		/*lint +e712 */
		//set_reg(ldi_base + LDI_DSI_TE_VS_WD, 0x3FC0FF, 32, 0);
		//set_reg(ldi_base + LDI_DSI_TE_VS_WD, 0x3FC01F, 32, 0);
	} else {
		set_reg(ldi_base + LDI_DSI_CMD_MOD_CTRL, 0x1, 1, 1);
	}
	//ldi_data_gate(dpufd, true);

	// normal
	set_reg(ldi_base + LDI_WORK_MODE, 0x1, 1, 0);


	if (is_mipi_cmd_panel(dpufd)) {
		set_reg(ldi_base + LDI_FRM_MSK,
			(dpufd->frame_update_flag == 1) ? 0x0 : 0x1, 1, 0);
	}

	if (dpufd->index == EXTERNAL_PANEL_IDX && (is_mipi_panel(dpufd))) {
		set_reg(ldi_base + LDI_DP_DSI_SEL, 0x1, 1, 0);
	}

	// ldi disable
	if (!fastboot_enable)
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);

	DPU_FB_DEBUG("-.!\n");
}
/*lint +e838 */
void deinit_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	set_reg(ldi_base + LDI_CTRL, 0, 1, 0);
}

void enable_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	/* ldi enable */
	set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
}

void disable_ldi(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	/* ldi disable */
	set_reg(ldi_base + LDI_CTRL, 0x0, 1, 0);
}

void ldi_frame_update(struct dpu_fb_data_type *dpufd, bool update)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;

		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK, (update ? 0x0 : 0x1), 1, 0);
			if (update)
				set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
	}
}

void single_frame_update(struct dpu_fb_data_type *dpufd)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}

	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;

		if (is_mipi_cmd_panel(dpufd)) {
			set_reg(ldi_base + LDI_FRM_MSK_UP, 0x1, 1, 0);
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		} else {
			set_reg(ldi_base + LDI_CTRL, 0x1, 1, 0);
		}
	} else {
		;
	}
}
/*lint -e838 */
void dpe_interrupt_clear(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = 0;
	uint32_t clear = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_PDP_INTS, clear);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INTS, clear);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INTS, clear);

		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INTS, clear);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INTS, clear);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		clear = ~0;
		outp32(dss_base + GLB_CPU_OFF_INTS, clear);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}

void dpe_interrupt_unmask(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = 0;
	uint32_t unmask = 0;
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	pinfo = &(dpufd->panel_info);
	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		unmask = ~0;
		//unmask &= ~(BIT_DPP_INTS | BIT_ITF0_INTS | BIT_DSS_GLB_INTS | BIT_MMU_IRPT_NS);
		unmask &= ~(BIT_DPP_INTS | BIT_ITF0_INTS | BIT_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_PDP_INT_MSK, unmask);

		unmask = ~0;
		if (is_mipi_cmd_panel(dpufd)) {
			unmask &= ~(BIT_LCD_TE0_PIN | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
		} else {
			unmask &= ~(BIT_VSYNC | BIT_VACTIVE0_START | BIT_VACTIVE0_END | BIT_FRM_END);
		}
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, unmask);

		unmask = ~0;
		//unmask &= ~(BIT_CE_END_IND | BIT_BACKLIGHT_INTP);
		if ((pinfo->acm_ce_support == 1) && (HISI_DSS_SUPPORT_DPP_MODULE_BIT(DPP_MODULE_ACE) != 0))
			unmask &= ~(BIT_CE_END_IND);

		if (pinfo->hiace_support == 1)
			unmask &= ~(BIT_HIACE_IND);

		outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, unmask);

	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		unmask = ~0;
		unmask &= ~(BIT_OFF_WCH0_INTS | BIT_OFF_WCH1_INTS | BIT_OFF_WCH0_WCH1_FRM_END_INT | BIT_OFF_MMU_IRPT_NS);
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, unmask);
		unmask = ~0;
		unmask &= ~(BIT_OFF_CAM_WCH2_FRMEND_INTS);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, unmask);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}

void dpe_interrupt_mask(struct dpu_fb_data_type *dpufd)
{
	char __iomem *dss_base = 0;
	uint32_t mask = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	dss_base = dpufd->dss_base;

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_PDP_INT_MSK, mask);
		outp32(dss_base + DSS_LDI0_OFFSET + LDI_CPU_ITF_INT_MSK, mask);
		outp32(dss_base + DSS_DPP_OFFSET + DPP_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_DSS_GLB_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_MCTL_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_WCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH0_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH1_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH4_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH5_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH6_INT_MSK, mask);
		outp32(dss_base + DSS_DBG_OFFSET + DBG_RCH7_INT_MSK, mask);
	} else if (dpufd->index == AUXILIARY_PANEL_IDX) {
		mask = ~0;
		outp32(dss_base + GLB_CPU_OFF_INT_MSK, mask);
		outp32(dss_base + GLB_CPU_OFF_CAM_INT_MSK, mask);
	} else {
		DPU_FB_ERR("fb%d, not support this device!\n", dpufd->index);
	}

}
/*lint +e838 */
void ldi_data_gate(struct dpu_fb_data_type *dpufd, bool enble)
{
	char __iomem *ldi_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (!is_mipi_cmd_panel(dpufd)) {
		dpufd->ldi_data_gate_en = (enble ? 1 : 0);
		return ;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI0_OFFSET;
	} else if (dpufd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = dpufd->dss_base + DSS_LDI1_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	if (g_ldi_data_gate_en == 1) {
		dpufd->ldi_data_gate_en = (enble ? 1 : 0);
		set_reg(ldi_base + LDI_CTRL, (enble ? 0x1 : 0x0), 1, 2);
	} else {
		dpufd->ldi_data_gate_en = 0;
		set_reg(ldi_base + LDI_CTRL, 0x0, 1, 2);
	}

	DPU_FB_DEBUG("ldi_data_gate_en=%d!\n", dpufd->ldi_data_gate_en);
}

/* dpp csc not surport */
void init_dpp_csc(struct dpu_fb_data_type *dpufd)
{
	return;
}

/*lint -e679 -e838*/
void acm_set_lut(char __iomem *address, uint32_t table[], uint32_t size)
{
	dpu_check_and_no_retval(!address, ERR, "address is NULL\n");

	uint32_t data = 0;
	uint32_t index = 0;
	uint32_t i = 0;

	size /= 4;

	for (i = 0; i < size; i++) {
		index = i << 2;
		data = table[index] + (table[index + 1] << 8) + (table[index + 2] << 16) + (table[index + 3] << 24);
		outp32(address + (i << 2), data);
	}
}

void acm_set_lut_hue(char __iomem *address, uint32_t table[], uint32_t size)
{
	dpu_check_and_no_retval(!address, ERR, "address is NULL\n");

	uint32_t data;
	uint32_t index;
	uint32_t i;

	size /= 2;

	for (i = 0; i < size; i++) {
		index = i << 1;
		data = table[index] + (table[index + 1] << 16);
		outp32(address + (i << 2), data);
	}
}//lint !e550 !e715
static void acm_set_lut_lh(char __iomem *address, uint32_t table[], uint32_t size)
{
	uint32_t data;
	uint32_t index;
	uint32_t i;

	size /= 2;

	for (i = 0; i < size; i++) {
		index = i << 1;
		data = (table[index] & 0x1FFF) | ((table[index + 1] & 0x1FFF) << 16);
		outp32(address + (i << 2), data);
	}
}//lint !e550 !e715
/*lint +e679 +838*/
static void acm_set_lut_table(char __iomem *acm_lut_base, struct dpu_panel_info *pinfo, uint32_t index)
{
	if (pinfo->acm_lut_hue_table && pinfo->acm_lut_hue_table_len > 0) {
		acm_set_lut_hue(acm_lut_base + ACM_U_H_COEF, pinfo->acm_lut_hue_table, pinfo->acm_lut_hue_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_hue_table is NULL or hue_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_sata_table && pinfo->acm_lut_sata_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATA_COEF, pinfo->acm_lut_sata_table, pinfo->acm_lut_sata_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_sata_table is NULL or sata_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr0_table && pinfo->acm_lut_satr0_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR0_COEF, pinfo->acm_lut_satr0_table, pinfo->acm_lut_satr0_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr0_table is NULL or satr0_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr1_table && pinfo->acm_lut_satr1_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR1_COEF, pinfo->acm_lut_satr1_table, pinfo->acm_lut_satr1_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr1_table is NULL or satr1_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr2_table && pinfo->acm_lut_satr2_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR2_COEF, pinfo->acm_lut_satr2_table, pinfo->acm_lut_satr2_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr2_table is NULL or satr2_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr3_table && pinfo->acm_lut_satr3_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR3_COEF, pinfo->acm_lut_satr3_table, pinfo->acm_lut_satr3_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr3_table is NULL or satr3_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr4_table && pinfo->acm_lut_satr4_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR4_COEF, pinfo->acm_lut_satr4_table, pinfo->acm_lut_satr4_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr4_table is NULL or satr4_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr5_table && pinfo->acm_lut_satr5_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR5_COEF, pinfo->acm_lut_satr5_table, pinfo->acm_lut_satr5_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr5_table is NULL or satr5_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr6_table && pinfo->acm_lut_satr6_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR6_COEF, pinfo->acm_lut_satr6_table, pinfo->acm_lut_satr6_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr6_table is NULL or satr6_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_satr7_table && pinfo->acm_lut_satr7_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_SATR7_COEF, pinfo->acm_lut_satr7_table, pinfo->acm_lut_satr7_table_len);
	} else {
		DPU_FB_ERR("fb%d, acm_lut_satr7_table is NULL or satr7_table_len less than 0!\n", index);
		return;
	}

	return;
}
static void acm_set_lut_LTx_table(char __iomem *acm_lut_base, struct dpu_panel_info *pinfo, uint32_t index)
{
	if (pinfo->acm_lut_satr_face_table && pinfo->acm_lut_satr_face_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_SATR_FACE_COEF, pinfo->acm_lut_satr_face_table, pinfo->acm_lut_satr_face_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_satr_face_table is NULL or acm_lut_satr_face_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lta_table && pinfo->acm_lut_lta_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTA_COEF, pinfo->acm_lut_lta_table, pinfo->acm_lut_lta_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lta_table is NULL or acm_lut_lta_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr0_table && pinfo->acm_lut_ltr0_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR0_COEF, pinfo->acm_lut_ltr0_table, pinfo->acm_lut_ltr0_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr0_table is NULL or acm_lut_ltr0_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr1_table && pinfo->acm_lut_ltr1_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR1_COEF, pinfo->acm_lut_ltr1_table, pinfo->acm_lut_ltr1_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr1_table is NULL or acm_lut_ltr1_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr2_table && pinfo->acm_lut_ltr2_table_len> 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR2_COEF, pinfo->acm_lut_ltr2_table, pinfo->acm_lut_ltr2_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr2_table is NULL or acm_lut_ltr2_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr3_table && pinfo->acm_lut_ltr3_table_len> 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR3_COEF, pinfo->acm_lut_ltr3_table, pinfo->acm_lut_ltr3_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr3_table is NULL or acm_lut_ltr3_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr4_table && pinfo->acm_lut_ltr4_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR4_COEF, pinfo->acm_lut_ltr4_table, pinfo->acm_lut_ltr4_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr4_table is NULL or acm_lut_ltr4_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr5_table && pinfo->acm_lut_ltr5_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR5_COEF, pinfo->acm_lut_ltr5_table, pinfo->acm_lut_ltr5_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr5_table is NULL or acm_lut_ltr5_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr6_table && pinfo->acm_lut_ltr6_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR6_COEF, pinfo->acm_lut_ltr6_table, pinfo->acm_lut_ltr6_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr6_table is NULL or acm_lut_ltr6_table_len less than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ltr7_table && pinfo->acm_lut_ltr7_table_len > 0) {
		acm_set_lut(acm_lut_base + ACM_U_ACM_LTR7_COEF, pinfo->acm_lut_ltr7_table, pinfo->acm_lut_ltr7_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ltr4_table is NULL or acm_lut_ltr4_table_len less than 0!\n", index);
		return;
	}

	return;
}
static void acm_set_lut_LHx_table(char __iomem *acm_lut_base, struct dpu_panel_info *pinfo, uint32_t index)
{
	if (pinfo->acm_lut_lh0_table && pinfo->acm_lut_lh0_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH0_COFF, pinfo->acm_lut_lh0_table, pinfo->acm_lut_lh0_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh0_table is NULL or acm_lut_lh0_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh1_table && pinfo->acm_lut_lh1_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH1_COFF, pinfo->acm_lut_lh1_table, pinfo->acm_lut_lh1_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh1_table is NULL or acm_lut_lh1_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh2_table && pinfo->acm_lut_lh2_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH2_COFF, pinfo->acm_lut_lh2_table, pinfo->acm_lut_lh2_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh2_table is NULL or acm_lut_lh2_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh3_table && pinfo->acm_lut_lh3_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH3_COFF, pinfo->acm_lut_lh3_table, pinfo->acm_lut_lh3_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh3_table is NULL or acm_lut_lh3_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh4_table && pinfo->acm_lut_lh4_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH4_COFF, pinfo->acm_lut_lh4_table, pinfo->acm_lut_lh4_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh4_table is NULL or acm_lut_lh4_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh5_table && pinfo->acm_lut_lh5_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH5_COFF, pinfo->acm_lut_lh5_table, pinfo->acm_lut_lh5_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh5_table is NULL or acm_lut_lh5_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh6_table && pinfo->acm_lut_lh6_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH6_COFF, pinfo->acm_lut_lh6_table, pinfo->acm_lut_lh6_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh6_table is NULL or acm_lut_lh6_table_lenless than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_lh7_table && pinfo->acm_lut_lh7_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_LH7_COFF, pinfo->acm_lut_lh7_table, pinfo->acm_lut_lh7_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_lh7_table is NULL or acm_lut_lh7_table_lenless than 0!\n", index);
		return;
	}

	return;
}
static void acm_set_lut_CHx_table(char __iomem *acm_lut_base, struct dpu_panel_info *pinfo, uint32_t index)
{
	if (pinfo->acm_lut_ch0_table && pinfo->acm_lut_ch0_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH0_COFF, pinfo->acm_lut_ch0_table, pinfo->acm_lut_ch0_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch0_table is NULL or acm_lut_ch0_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch1_table && pinfo->acm_lut_ch1_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH1_COFF, pinfo->acm_lut_ch1_table, pinfo->acm_lut_ch1_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch1_table is NULL or acm_lut_ch1_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch2_table && pinfo->acm_lut_ch2_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH2_COFF, pinfo->acm_lut_ch2_table, pinfo->acm_lut_ch2_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch2_table is NULL or acm_lut_ch2_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch3_table && pinfo->acm_lut_ch3_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH3_COFF, pinfo->acm_lut_ch3_table, pinfo->acm_lut_ch3_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch3_table is NULL or acm_lut_ch3_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch4_table && pinfo->acm_lut_ch4_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH4_COFF, pinfo->acm_lut_ch4_table, pinfo->acm_lut_ch4_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch4_table is NULL or acm_lut_ch4_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch5_table && pinfo->acm_lut_ch5_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH5_COFF, pinfo->acm_lut_ch5_table, pinfo->acm_lut_ch5_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch5_table is NULL or acm_lut_ch5_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch6_table && pinfo->acm_lut_ch6_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH6_COFF, pinfo->acm_lut_ch6_table, pinfo->acm_lut_ch6_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch6_table is NULL or acm_lut_ch6_table_len than 0!\n", index);
		return;
	}

	if (pinfo->acm_lut_ch7_table && pinfo->acm_lut_ch7_table_len > 0) {
		acm_set_lut_lh(acm_lut_base + ACM_U_ACM_CH7_COFF, pinfo->acm_lut_ch7_table, pinfo->acm_lut_ch7_table_len);
	} else {
		DPU_FB_INFO("fb%d, acm_lut_ch7_table is NULL or acm_lut_ch7_table_len than 0!\n", index);
		return;
	}
	return;
}
/*lint -e838 */
void init_acm(struct dpu_fb_data_type *dpufd)
{
	char __iomem *acm_base = NULL;
	char __iomem *acm_lut_base = NULL;
	struct dpu_panel_info *pinfo = NULL;
	uint32_t lut_sel = 0;

	if (dpufd == NULL)	{
		DPU_FB_DEBUG("init_acm dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	acm_base = dpufd->dss_base + DSS_DPP_ACM_OFFSET;

	if (pinfo->acm_support != 1) {
		outp32(acm_base + ACM_MEM_CTRL, 0x4);
		DPU_FB_DEBUG("fb%d, not support acm!\n", dpufd->index);
		return;
	}

	acm_lut_base = dpufd->dss_base + DSS_DPP_ACM_LUT_OFFSET;

	set_reg(acm_base + ACM_SATA_OFFSET, 0x20, 6, 0);
	//Rec.709(wide):
	set_reg(acm_base + ACM_CSC_IDC0, 0x600, 11, 0);
	set_reg(acm_base + ACM_CSC_IDC1, 0x600, 11, 0);
	set_reg(acm_base + ACM_CSC_IDC2, 0x0, 11, 0);

	set_reg(acm_base + ACM_CSC_P00, 0x4000, 17, 0);
	set_reg(acm_base + ACM_CSC_P01, 0x0, 17, 0);
	set_reg(acm_base + ACM_CSC_P02, 0x64CA, 17, 0);
	set_reg(acm_base + ACM_CSC_P10, 0x4000, 17, 0);
	set_reg(acm_base + ACM_CSC_P11, 0x1F403, 17, 0);
	set_reg(acm_base + ACM_CSC_P12, 0x1E20A, 17, 0);
	set_reg(acm_base + ACM_CSC_P20, 0x4000, 17, 0);
	set_reg(acm_base + ACM_CSC_P21, 0x76C2, 17, 0);
	set_reg(acm_base + ACM_CSC_P22, 0x0, 17, 0);

	set_reg(acm_base + ACM_HUE_RLH01, (((pinfo->r1_lh & 0x3ff) << 16) | (pinfo->r0_lh & 0x3ff)), 26, 0);
	set_reg(acm_base + ACM_HUE_RLH23, (((pinfo->r3_lh & 0x3ff) << 16) | (pinfo->r2_lh & 0x3ff)), 26, 0);
	set_reg(acm_base + ACM_HUE_RLH45, (((pinfo->r5_lh & 0x3ff) << 16) | (pinfo->r4_lh & 0x3ff)), 26, 0);
	set_reg(acm_base + ACM_HUE_RLH67, (((pinfo->r6_hh & 0x3ff) << 16)| (pinfo->r6_lh & 0x3ff)), 26, 0);//needconfirm
	set_reg(acm_base + ACM_HUE_PARAM01, ((0x200 << 16) | (0x200)), 32, 0);
	set_reg(acm_base + ACM_HUE_PARAM23, ((0x1FC << 16) | 0x200), 32, 0);
	set_reg(acm_base + ACM_HUE_PARAM45, ((0x204 << 16) | (0x200)), 32, 0);
	set_reg(acm_base + ACM_HUE_PARAM67, ((0x200 << 16) | (0x200)), 32, 0);
	set_reg(acm_base + ACM_HUE_SMOOTH0, 0x0040003F, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH1, 0x00C000BF, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH2, 0x0140013F, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH3, 0x01C001BF, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH4, 0x02410240, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH5, 0x02C102C0, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH6, 0x0340033F, 26, 0); //needconfirm
	set_reg(acm_base + ACM_HUE_SMOOTH7, 0x03C003BF, 26, 0); //needconfirm
	set_reg(acm_base + ACM_COLOR_CHOOSE, 1, 1, 0); //needconfirm
	//ACM RGB2YUV
	set_reg(acm_base + ACM_RGB2YUV_IDC0, 0x00000200, 11, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_IDC1, 0x00000200, 11, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_IDC2, 0x00000000, 11, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P00, 0x00000D9B, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P01, 0x00002DC6, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P02, 0x0000049F, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P10, 0x0001F8AB, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P11, 0x0001E755, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P12, 0x00002000, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P20, 0x00002000, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P21, 0x0001E2EF, 17, 0);//needconfirm
	set_reg(acm_base + ACM_RGB2YUV_P22, 0x0001FD11, 17, 0);//needconfirm
	//ACM FACE
	set_reg(acm_base + ACM_FACE_CRTL, 0x01180118, 32, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_STARTXY, 0x004600DC, 29, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_LEN01, 0x00100010, 29, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_LEN23, 0x00100010, 29, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM0, 0x00010000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM1, 0x00010000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM2, 0x00010000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM3, 0x00010000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM4, 0x00001000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM5, 0x00001000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM6, 0x00001000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SMOOTH_PARAM7, 0x00001000, 20, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_AREA_SEL, 0x00000002, 3, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SAT_LH, 0x02AE0000, 26, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SAT_SMOOTH_LH, 0x02D60000, 26, 0);//needconfirm
	set_reg(acm_base + ACM_FACE_SAT_SMO_PARAM_LH, 0x06660001, 16, 0);//needconfirm
	//ACM L CONTRAST
	set_reg(acm_base + ACM_L_CONT_EN, 0x00000000, 1, 0);//needconfirm
	set_reg(acm_base + ACM_LC_PARAM01, 0x020401FC, 16, 0);//needconfirm
	set_reg(acm_base + ACM_LC_PARAM23, 0x02000200, 16, 0);//needconfirm
	set_reg(acm_base + ACM_LC_PARAM45, 0x020801F8, 16, 0);//needconfirm
	set_reg(acm_base + ACM_LC_PARAM67, 0x020401FC, 16, 0);//needconfirm
	//ACM L ADJ
	set_reg(acm_base + ACM_L_ADJ_CTRL, 0, 9, 0);//needconfirm
	//ACM CAPTURE
	set_reg(acm_base + ACM_CAPTURE_CTRL, 0, 32, 0);//needconfirm
	set_reg(acm_base + ACM_CAPTURE_IN, 0, 30, 0);//needconfirm
	set_reg(acm_base + ACM_CAPTURE_OUT, 0, 30, 0);//needconfirm
	//ACM INK
	set_reg(acm_base + ACM_INK_CTRL, 0, 32, 0);//needconfirm
	set_reg(acm_base + ACM_INK_OUT, 0, 30, 0);//needconfirm

	acm_set_lut_table(acm_lut_base, pinfo, dpufd->index);
	acm_set_lut_LTx_table(acm_lut_base, pinfo, dpufd->index);
	acm_set_lut_LHx_table(acm_lut_base, pinfo, dpufd->index);
	acm_set_lut_CHx_table(acm_lut_base, pinfo, dpufd->index);

	lut_sel = inp32(acm_base + ACM_LUT_SEL);
	set_reg(acm_base + ACM_LUT_SEL, (~lut_sel) & 0x7F80, 16, 0);
	set_reg(acm_base + ACM_EN, 0x0, 1, 0);

	g_acm_State = 1;
	/*acm reg dimming init*/
	hisi_effect_color_dimming_acm_reg_init(dpufd);
}
/*lint +e838 */
//lint -e838 -e550 -e438
static void degamma_set_lut(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t i = 0;
	uint32_t index = 0;
	char __iomem *degamma_lut_base = NULL;//lint !e838

	if (dpufd == NULL)	{
		DPU_FB_ERR("init_degmma_xcc_gmp dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	degamma_lut_base = dpufd->dss_base + DSS_DPP_DEGAMMA_LUT_OFFSET;

	if (!dpufb_use_dynamic_degamma(dpufd, degamma_lut_base)) {
		if (pinfo->igm_lut_table_len > 0
			&& pinfo->igm_lut_table_R
			&& pinfo->igm_lut_table_G
			&& pinfo->igm_lut_table_B) {
			for (i = 0; i < pinfo->igm_lut_table_len / 2; i++) {
				index = i << 1;
				outp32(degamma_lut_base + (U_DEGAMA_R_COEF +  i * 4), pinfo->igm_lut_table_R[index] | pinfo->igm_lut_table_R[index+1] << 16);
				outp32(degamma_lut_base + (U_DEGAMA_G_COEF +  i * 4), pinfo->igm_lut_table_G[index] | pinfo->igm_lut_table_G[index+1] << 16);
				outp32(degamma_lut_base + (U_DEGAMA_B_COEF +  i * 4), pinfo->igm_lut_table_B[index] | pinfo->igm_lut_table_B[index+1] << 16);
			}
			outp32(degamma_lut_base + U_DEGAMA_R_LAST_COEF, pinfo->igm_lut_table_R[pinfo->igm_lut_table_len - 1]);
			outp32(degamma_lut_base + U_DEGAMA_G_LAST_COEF, pinfo->igm_lut_table_G[pinfo->igm_lut_table_len - 1]);
			outp32(degamma_lut_base + U_DEGAMA_B_LAST_COEF, pinfo->igm_lut_table_B[pinfo->igm_lut_table_len - 1]);
		}
	}//lint !e438 !e550
}
static void gamma_set_lut(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t i = 0;
	uint32_t index = 0;
	char __iomem *gamma_lut_base = NULL;

	if (dpufd == NULL)	{
		DPU_FB_ERR("init_degmma_xcc_gmp dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);
	gamma_lut_base = dpufd->dss_base + DSS_DPP_GAMA_LUT_OFFSET;

	if (!dpufb_use_dynamic_gamma(dpufd, gamma_lut_base)) {
		if (pinfo->gamma_lut_table_len > 0
			&& pinfo->gamma_lut_table_R
			&& pinfo->gamma_lut_table_G
			&& pinfo->gamma_lut_table_B) {
			for (i = 0; i < pinfo->gamma_lut_table_len / 2; i++) {
				index = i << 1;
				//GAMA LUT
				outp32(gamma_lut_base + (U_GAMA_R_COEF + i * 4), pinfo->gamma_lut_table_R[index] | pinfo->gamma_lut_table_R[index+1] << 16 );
				outp32(gamma_lut_base + (U_GAMA_G_COEF + i * 4), pinfo->gamma_lut_table_G[index] | pinfo->gamma_lut_table_G[index+1] << 16 );
				outp32(gamma_lut_base + (U_GAMA_B_COEF + i * 4), pinfo->gamma_lut_table_B[index] | pinfo->gamma_lut_table_B[index+1] << 16 );
			}
			outp32(gamma_lut_base + U_GAMA_R_LAST_COEF, pinfo->gamma_lut_table_R[pinfo->gamma_lut_table_len - 1]);
			outp32(gamma_lut_base + U_GAMA_G_LAST_COEF, pinfo->gamma_lut_table_G[pinfo->gamma_lut_table_len - 1]);
			outp32(gamma_lut_base + U_GAMA_B_LAST_COEF, pinfo->gamma_lut_table_B[pinfo->gamma_lut_table_len - 1]);
		}
	}
}
/*lint -e695*/
inline void xcc_set_coef(char __iomem *base_addr, struct dpu_panel_info *pinfo,
    uint32_t rectify_R, uint32_t rectify_G, uint32_t rectify_B)
{
	dpu_check_and_no_retval((!base_addr || !pinfo), ERR, "base_addr or pinfo is NULL\n");

	outp32(base_addr + LCP_XCC_COEF_00, pinfo->xcc_table[0]);
	outp32(base_addr + LCP_XCC_COEF_01, pinfo->xcc_table[1]
	    * g_led_rg_csc_value[0] / 32768 * rectify_R / 32768);
	outp32(base_addr + LCP_XCC_COEF_02, pinfo->xcc_table[2]);
	outp32(base_addr + LCP_XCC_COEF_03, pinfo->xcc_table[3]);
	outp32(base_addr + LCP_XCC_COEF_10, pinfo->xcc_table[4]);
	outp32(base_addr + LCP_XCC_COEF_11, pinfo->xcc_table[5]);
	outp32(base_addr + LCP_XCC_COEF_12, pinfo->xcc_table[6]
	    * g_led_rg_csc_value[4] / 32768 * rectify_G / 32768);
	outp32(base_addr + LCP_XCC_COEF_13, pinfo->xcc_table[7]);
	outp32(base_addr + LCP_XCC_COEF_20, pinfo->xcc_table[8]);
	outp32(base_addr + LCP_XCC_COEF_21, pinfo->xcc_table[9]);
	outp32(base_addr + LCP_XCC_COEF_22, pinfo->xcc_table[10]);
	outp32(base_addr + LCP_XCC_COEF_23, pinfo->xcc_table[11]
	    * g_led_rg_csc_value[8] / 32768 * discount_coefficient(g_comform_value) / CHANGE_MAX
	    * rectify_B / 32768);
}
/*lint +e695*/
void init_igm_gmp_xcc_gm(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *xcc_base = NULL;
	char __iomem *gmp_base = NULL;
	char __iomem *degamma_base = NULL;
	char __iomem *gmp_lut_base = NULL;
	char __iomem *gamma_base = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t i = 0;
	uint32_t gama_lut_sel = 0;
	uint32_t degama_lut_sel = 0;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;
	uint32_t gmp_lut_sel;

	if (dpufd == NULL)	{
		DPU_FB_ERR("init_degmma_xcc_gmp dpufd is NULL!\n");
		return;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		lcp_base = dpufd->dss_base + DSS_DPP_LCP_OFFSET;
		xcc_base = dpufd->dss_base + DSS_DPP_XCC_OFFSET;
		gmp_base = dpufd->dss_base + DSS_DPP_GMP_OFFSET;
		degamma_base = dpufd->dss_base + DSS_DPP_DEGAMMA_OFFSET;
		gmp_lut_base = dpufd->dss_base + DSS_DPP_GMP_LUT_OFFSET;
		gamma_base = dpufd->dss_base + DSS_DPP_GAMA_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!\n", dpufd->index);
		return;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}
	//Degamma
	if (pinfo->gamma_support == 1) {
		//disable degamma
		set_reg(degamma_base + DEGAMA_EN, 0x0, 1, 0);

		degamma_set_lut(dpufd);
			degama_lut_sel = (uint32_t)inp32(degamma_base + DEGAMA_LUT_SEL);
			set_reg(degamma_base + DEGAMA_LUT_SEL, (~(degama_lut_sel & 0x1)) & 0x1, 1, 0);

		//enable degamma
		set_reg(degamma_base + DEGAMA_EN, 0x1, 1, 0);
	} else {
		//degama memory shutdown
		outp32(degamma_base + DEGAMA_MEM_CTRL, 0x4);
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len == xcc_cnt_cofe && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
			//enable xcc
			set_reg(lcp_base + LCP_XCC_BYPASS_EN, 0x0, 1, 0);
		}
	}

	//GMP
	if (pinfo->gmp_support == 1) {
		//disable gmp
		set_reg(gmp_base + GMP_EN, 0x0, 1, 0);

		//gmp lut
		if (pinfo->gmp_lut_table_len == gmp_cnt_cofe
			&& pinfo->gmp_lut_table_low32bit
			&& pinfo->gmp_lut_table_high4bit) {
			for (i = 0; i < gmp_cnt_cofe; i++) {
				outp32(gmp_lut_base + i * 2 * 4, pinfo->gmp_lut_table_low32bit[i]);
				outp32(gmp_lut_base + i * 2 * 4 + 4, pinfo->gmp_lut_table_high4bit[i]);
			}
			gmp_lut_sel = (uint32_t)inp32(gmp_base + GMP_LUT_SEL);
			set_reg(gmp_base + GMP_LUT_SEL, (~(gmp_lut_sel & 0x1)) & 0x1, 1, 0);
			//enable gmp
			set_reg(gmp_base + GMP_EN, 0x1, 1, 0);

			g_gmp_State = 1;
		}
	} else {
		//gmp memory shutdown
		outp32(gmp_base + GMP_MEM_CTRL, 0x4);
	}

	//GAMMA
	if (pinfo->gamma_support == 1) {
		//disable gamma
		set_reg(gamma_base + GAMA_EN, 0x0, 1, 0);
		//set gama lut
		gamma_set_lut(dpufd);

		gama_lut_sel = (uint32_t)inp32(gamma_base + GAMA_LUT_SEL);
		set_reg(gamma_base + GAMA_LUT_SEL, (~(gama_lut_sel & 0x1)) & 0x1, 1, 0);

		//enable gamma
		set_reg(gamma_base + GAMA_EN, 0x1, 1, 0);
	} else {
		//gama memory shutdown
		outp32(gamma_base + GAMA_MEM_CTRL, 0x4);
	}
}

void init_dither(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *dither_base = NULL;

	if (dpufd == NULL)	{
		DPU_FB_ERR("dpufd is NULL!\n");
		return;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->dither_support != 1) {
		return;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dither_base = dpufd->dss_base + DSS_DPP_DITHER_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return ;
	}

	set_reg(dither_base + DITHER_CTL, 0x1A86, 28, 0);
	set_reg(dither_base + DITHER_PARA, 0x1, 3, 0);
	set_reg(dither_base + DITHER_MATRIX_PART1, 0x5D7F91B3, 32, 0);
	set_reg(dither_base + DITHER_MATRIX_PART0, 0x6E4CA280, 32, 0);
	set_reg(dither_base + DITHER_ERRDIFF_WEIGHT, 0x1232134, 28, 0);
	set_reg(dither_base + DITHER_FRC_01_PART0, 0x0, 32, 0);
	set_reg(dither_base + DITHER_FRC_01_PART1, 0xFFFF0000, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART0, 0, 32, 0);
	set_reg(dither_base + DITHER_FRC_10_PART1, 0xFFFFFFFF, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART0, 0xFFFF0000, 32, 0);
	set_reg(dither_base + DITHER_FRC_11_PART1, 0xFFFFFFFF, 32, 0);
}
//lint +e838 +e550 +e438
/*lint -e838 */
void dpe_store_ct_csc_value(struct dpu_fb_data_type *dpufd, unsigned int csc_value[], unsigned int len)
{
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return;
	}

	if (len < CSC_VALUE_MIN_LEN) {
		DPU_FB_ERR("csc_value len is too short\n");
		return;
	}

	pinfo = &(dpufd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return;
	}

	pinfo->xcc_table[1] = csc_value[0];
	pinfo->xcc_table[2] = csc_value[1];
	pinfo->xcc_table[3] = csc_value[2];
	pinfo->xcc_table[5] = csc_value[3];
	pinfo->xcc_table[6] = csc_value[4];
	pinfo->xcc_table[7] = csc_value[5];
	pinfo->xcc_table[9] = csc_value[6];
	pinfo->xcc_table[10] = csc_value[7];
	pinfo->xcc_table[11] = csc_value[8];

	return;
}

void dpe_update_g_comform_discount(unsigned int value)
{
	g_comform_value = value;
	DPU_FB_INFO(" g_comform_value = %d" , g_comform_value);
}

int dpe_set_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		lcp_base = dpufd->dss_base + DSS_DPP_LCP_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
			dpufd->color_temperature_flag = 2;
		}
	}

	return 0;
}

ssize_t dpe_show_ct_csc_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	struct dpu_panel_info *pinfo = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11]);
}
/*lint +e838 */
int dpe_set_xcc_csc_value(struct dpu_fb_data_type *dpufd)
{
	return 0;
}
/*lint -e550, -e838*/
int dpe_set_comform_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	uint32_t color_temp_rectify_R = 32768, color_temp_rectify_G = 32768, color_temp_rectify_B = 32768;
	char __iomem *lcp_base = NULL;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	lcp_base = dpufd->dss_base + DSS_DPP_LCP_OFFSET;

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R <= 32768 && pinfo->color_temp_rectify_R) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B <= 32768 && pinfo->color_temp_rectify_B) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (pinfo->xcc_support == 1) {
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			xcc_set_coef(lcp_base, pinfo, color_temp_rectify_R, color_temp_rectify_G, color_temp_rectify_B);
		}
	}

	return 0;
}
/*lint +e550, +e838*/
/*lint -e838*/
ssize_t dpe_show_comform_ct_csc_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	struct dpu_panel_info *pinfo = NULL;
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (pinfo->xcc_support == 0 || pinfo->xcc_table == NULL) {
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,g_comform_value = %d\n",
		pinfo->xcc_table[1], pinfo->xcc_table[2], pinfo->xcc_table[3],
		pinfo->xcc_table[5], pinfo->xcc_table[6], pinfo->xcc_table[7],
		pinfo->xcc_table[9], pinfo->xcc_table[10], pinfo->xcc_table[11],
		g_comform_value);
}
/*lint +e838*/
void dpe_init_led_rg_ct_csc_value(void)
{
	g_led_rg_csc_value[0] = 32768;
	g_led_rg_csc_value[1] = 0;
	g_led_rg_csc_value[2] = 0;
	g_led_rg_csc_value[3] = 0;
	g_led_rg_csc_value[4] = 32768;
	g_led_rg_csc_value[5] = 0;
	g_led_rg_csc_value[6] = 0;
	g_led_rg_csc_value[7] = 0;
	g_led_rg_csc_value[8] = 32768;
	g_is_led_rg_csc_set = 0;

	return;
}

void dpe_store_led_rg_ct_csc_value(unsigned int csc_value[], unsigned int len)
{
	if (len < CSC_VALUE_MIN_LEN) {
		DPU_FB_ERR("csc_value len is too short\n");
		return;
	}

	g_led_rg_csc_value [0] = csc_value[0];
	g_led_rg_csc_value [1] = csc_value[1];
	g_led_rg_csc_value [2] = csc_value[2];
	g_led_rg_csc_value [3] = csc_value[3];
	g_led_rg_csc_value [4] = csc_value[4];
	g_led_rg_csc_value [5] = csc_value[5];
	g_led_rg_csc_value [6] = csc_value[6];
	g_led_rg_csc_value [7] = csc_value[7];
	g_led_rg_csc_value [8] = csc_value[8];
	g_is_led_rg_csc_set = 1;

	return;
}
/*lint -e838*/
int dpe_set_led_rg_ct_csc_value(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *lcp_base = NULL;
	uint32_t color_temp_rectify_R = 32768;
	uint32_t color_temp_rectify_G = 32768;
	uint32_t color_temp_rectify_B = 32768;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}
	pinfo = &(dpufd->panel_info);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		lcp_base = dpufd->dss_base + DSS_DPP_LCP_OFFSET;
	} else {
		DPU_FB_ERR("fb%d, not support!", dpufd->index);
		return -1;
	}

	if (pinfo->color_temp_rectify_R && pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_R <= 32768) {
		color_temp_rectify_R = pinfo->color_temp_rectify_R;
	}

	if (pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_G && pinfo->color_temp_rectify_G <= 32768) {
		color_temp_rectify_G = pinfo->color_temp_rectify_G;
	}

	if (pinfo->color_temp_rectify_B && pinfo->color_temp_rectify_support && pinfo->color_temp_rectify_B <= 32768) {
		color_temp_rectify_B = pinfo->color_temp_rectify_B;
	}

	//XCC
	if (g_is_led_rg_csc_set == 1 && pinfo->xcc_support == 1) {
		DPU_FB_DEBUG("real set color temperature: g_is_led_rg_csc_set = %d, R = 0x%x, G = 0x%x, B = 0x%x .\n",
				g_is_led_rg_csc_set, g_led_rg_csc_value[0], g_led_rg_csc_value[4], g_led_rg_csc_value[8]);
		// XCC matrix
		if (pinfo->xcc_table_len > 0 && pinfo->xcc_table) {
			outp32(lcp_base + LCP_XCC_COEF_00, pinfo->xcc_table[0]);
			outp32(lcp_base + LCP_XCC_COEF_01, pinfo->xcc_table[1]
				* g_led_rg_csc_value[0] / 32768 * color_temp_rectify_R / 32768);
			outp32(lcp_base + LCP_XCC_COEF_02, pinfo->xcc_table[2]);
			outp32(lcp_base + LCP_XCC_COEF_03, pinfo->xcc_table[3]);
			outp32(lcp_base + LCP_XCC_COEF_10, pinfo->xcc_table[4]);
			outp32(lcp_base + LCP_XCC_COEF_11, pinfo->xcc_table[5]);
			outp32(lcp_base + LCP_XCC_COEF_12, pinfo->xcc_table[6]
				* g_led_rg_csc_value[4] / 32768 * color_temp_rectify_G / 32768);
			outp32(lcp_base + LCP_XCC_COEF_13, pinfo->xcc_table[7]);
			outp32(lcp_base + LCP_XCC_COEF_20, pinfo->xcc_table[8]);
			outp32(lcp_base + LCP_XCC_COEF_21, pinfo->xcc_table[9]);
			outp32(lcp_base + LCP_XCC_COEF_22, pinfo->xcc_table[10]);
			outp32(lcp_base + LCP_XCC_COEF_23, pinfo->xcc_table[11]
				* g_led_rg_csc_value[8] / 32768 * discount_coefficient(g_comform_value) / CHANGE_MAX
				* color_temp_rectify_B / 32768);
		}
	}

	return 0;
}
/*lint +e838*/
ssize_t dpe_show_led_rg_ct_csc_value(char *buf)
{
	if (buf == NULL) {
		DPU_FB_ERR("buf, NUll pointer warning\n");
		return 0;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		g_led_rg_para1, g_led_rg_para2,
		g_led_rg_csc_value [0], g_led_rg_csc_value [1], g_led_rg_csc_value [2],
		g_led_rg_csc_value [3], g_led_rg_csc_value [4], g_led_rg_csc_value [5],
		g_led_rg_csc_value [6], g_led_rg_csc_value [7], g_led_rg_csc_value [8]);
}

ssize_t dpe_show_cinema_value(struct dpu_fb_data_type *dpufd, char *buf)
{
	dpu_check_and_return((!dpufd || !buf), 0, ERR, "dpufd or buf is NULL\n");

	return snprintf(buf, PAGE_SIZE, "gamma type is = %d\n", dpufd->panel_info.gamma_type);
}

int dpe_set_cinema(struct dpu_fb_data_type *dpufd, unsigned int value)
{
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd, NUll pointer warning.\n");
		return -1;
	}

	if(dpufd->panel_info.gamma_type == value) {
		DPU_FB_DEBUG("fb%d, cinema mode is already in %d!\n", dpufd->index, value);
		return 0;
	}

	dpufd->panel_info.gamma_type = value;
	return 0;
}

void dpe_update_g_acm_state(unsigned int value)
{
	return;
}

void dpe_set_acm_state(struct dpu_fb_data_type *dpufd)
{
	return;
}
/*lint -e838*/
ssize_t dpe_show_acm_state(char *buf)
{
	ssize_t ret = 0;

	if (buf == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_acm_State = %d\n", g_acm_State);

	return ret;
}

void dpe_update_g_gmp_state(unsigned int value)
{
	return;
}

void dpe_set_gmp_state(struct dpu_fb_data_type *dpufd)
{
	return;
}

ssize_t dpe_show_gmp_state(char *buf)
{
	ssize_t ret = 0;

	if (buf == NULL) {
		DPU_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "g_gmp_State = %d\n", g_gmp_State);

	return ret;
}

void deinit_dbuf(struct dpu_fb_data_type *dpufd)
{
	void_unused(dpufd);
}

/*lint +e838*/
