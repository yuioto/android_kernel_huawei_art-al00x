

/* 1 ͷ�ļ����� */
#include "mac_regdomain.h"
#include "mac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_REGDOMAIN_HOST_C

/* 2 ȫ�ֱ������� */
/* ��������Ϣȫ�ֱ��� */
mac_regdomain_info_stru g_st_mac_regdomain;
oal_uint8 g_channel_14_enable;

#ifdef _PRE_WLAN_FEATURE_5G
/* �ŵ����б�����ʼ���������Ϊ��Ч */
OAL_STATIC mac_channel_info_stru g_ast_channel_list_5G[MAC_CHANNEL_FREQ_5_BUTT] = {
    {36,  MAC_INVALID_RC}, {40,  MAC_INVALID_RC}, {44,  MAC_INVALID_RC},
    {48,  MAC_INVALID_RC}, {52,  MAC_INVALID_RC}, {56,  MAC_INVALID_RC},
    {60,  MAC_INVALID_RC}, {64,  MAC_INVALID_RC}, {100, MAC_INVALID_RC},
    {104, MAC_INVALID_RC}, {108, MAC_INVALID_RC}, {112, MAC_INVALID_RC},
    {116, MAC_INVALID_RC}, {120, MAC_INVALID_RC}, {124, MAC_INVALID_RC},
    {128, MAC_INVALID_RC}, {132, MAC_INVALID_RC}, {136, MAC_INVALID_RC},
    {140, MAC_INVALID_RC}, {144, MAC_INVALID_RC}, {149, MAC_INVALID_RC},
    {153, MAC_INVALID_RC}, {157, MAC_INVALID_RC}, {161, MAC_INVALID_RC},
    {165, MAC_INVALID_RC}, {184, MAC_INVALID_RC}, {188, MAC_INVALID_RC},
    {192, MAC_INVALID_RC}, {196, MAC_INVALID_RC},
};
#endif /* _PRE_WLAN_FEATURE_5G */

OAL_STATIC mac_channel_info_stru g_ast_channel_list_2G[MAC_CHANNEL_FREQ_2_BUTT] = {
    {1,  MAC_INVALID_RC}, {2,  MAC_INVALID_RC}, {3,  MAC_INVALID_RC},
    {4,  MAC_INVALID_RC}, {5,  MAC_INVALID_RC}, {6,  MAC_INVALID_RC},
    {7,  MAC_INVALID_RC}, {8,  MAC_INVALID_RC}, {9,  MAC_INVALID_RC},
    {10, MAC_INVALID_RC}, {11, MAC_INVALID_RC}, {12, MAC_INVALID_RC},
    {13, MAC_INVALID_RC}, {14, MAC_INVALID_RC},
};


/* 3 ����ʵ�� */

oal_void  mac_get_regdomain_info(mac_regdomain_info_stru **ppst_rd_info)
{
    if (ppst_rd_info == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_get_regdomain_info::ppst_rd_info null.}");
        return;
    }

    *ppst_rd_info = &g_st_mac_regdomain;

    return;
}


oal_void  mac_init_regdomain(oal_void)
{
    oal_int8 ac_default_country[] = "99";
    mac_regclass_info_stru *pst_regclass = OAL_PTR_NULL;
    int32_t ret;

    ret = memcpy_s(g_st_mac_regdomain.ac_country, WLAN_COUNTRY_STR_LEN, ac_default_country,
        OAL_SIZEOF(ac_default_country));
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_init_regdomain::memcopy_s return value %d", ret);
        return;
    }
    /* ��ʼĬ�ϵĹ��������Ϊ2 */
    g_st_mac_regdomain.uc_regclass_num = 2;
    g_channel_14_enable = 0;

    /* ��ʼ��������1 */
    pst_regclass = &(g_st_mac_regdomain.ast_regclass[0]);

    pst_regclass->en_start_freq     = MAC_RC_START_FREQ_2;
    pst_regclass->en_ch_spacing     = MAC_CH_SPACING_5MHZ;
    pst_regclass->uc_behaviour_bmap = 0;
    pst_regclass->uc_coverage_class = 0;
    pst_regclass->uc_max_reg_tx_pwr = MAC_RC_DEFAULT_MAX_TX_PWR;
    pst_regclass->uc_max_tx_pwr     = MAC_RC_DEFAULT_MAX_TX_PWR;

    pst_regclass->ul_channel_bmap = MAC_GET_CH_BIT(MAC_CHANNEL1) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL2) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL3) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL4) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL5) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL6) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL7) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL8) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL9) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL10) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL11) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL12) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL13);
#ifdef _PRE_WLAN_FEATURE_5G
    /* ��ʼ��������2 */
    pst_regclass = &(g_st_mac_regdomain.ast_regclass[1]);

    pst_regclass->en_start_freq     = MAC_RC_START_FREQ_5;
    pst_regclass->en_ch_spacing     = MAC_CH_SPACING_20MHZ;
    pst_regclass->uc_behaviour_bmap = 0;
    pst_regclass->uc_coverage_class = 0;
    pst_regclass->uc_max_reg_tx_pwr = MAC_RC_DEFAULT_MAX_TX_PWR;
    pst_regclass->uc_max_tx_pwr     = MAC_RC_DEFAULT_MAX_TX_PWR;

    pst_regclass->ul_channel_bmap = MAC_GET_CH_BIT(MAC_CHANNEL36) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL40) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL44) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL48) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL52) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL56) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL60) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL64) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL100) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL104) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL108) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL112) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL116) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL120) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL124) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL128) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL132) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL136) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL140) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL144) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL149) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL153) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL157) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL161) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL165) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL184) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL188) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL192) |
                                    MAC_GET_CH_BIT(MAC_CHANNEL196);
#endif /* _PRE_WLAN_FEATURE_5G */
}

#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  mac_set_country_ie_2g(mac_regdomain_info_stru *pst_rd_info,
                                  oal_uint8               *puc_buffer,
                                  oal_uint8               *puc_len)
{
    oal_uint8                uc_rc_idx;
    oal_uint8                uc_lsb_bit_position;
    mac_regclass_info_stru  *pst_reg_class = OAL_PTR_NULL;
    oal_uint32               ul_ret;

    oal_uint8                uc_len = 0;

    for (uc_rc_idx = 0; uc_rc_idx < pst_rd_info->uc_regclass_num; uc_rc_idx++) {
        /* ��ȡ Regulatory Class */
        pst_reg_class = &(pst_rd_info->ast_regclass[uc_rc_idx]);

        /* ���Ƶ�β�ƥ�� */
        if (pst_reg_class->en_start_freq != MAC_RC_START_FREQ_2) {
            continue;
        }

        /* �쳣��飬�ŵ�λͼΪ0��ʾ�˹�����û���ŵ����ڣ������� */
        if (pst_reg_class->ul_channel_bmap == 0) {
            continue;
        }

        /* ��ȡ�ŵ�λͼ�����һλ, ����0����bit0��1 */
        uc_lsb_bit_position = oal_bit_find_first_bit_four_byte(pst_reg_class->ul_channel_bmap);

        /* ��ȡ�ŵ��ţ�����Channel_MapΪ1100�����Ӧ������ֵΪ2��3����������ֵ�ҵ��ŵ��� */
        ul_ret = mac_get_channel_num_from_idx(MAC_RC_START_FREQ_2, uc_lsb_bit_position, &puc_buffer[uc_len++]);
        if (ul_ret != OAL_SUCC) {
            MAC_ERR_LOG1(0, "err code", ul_ret);
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_set_country_ie_2g::mac_get_channel_num_from_idx failed[%d].}", ul_ret);

            return ul_ret;
        }

        /* ��ȡ�ŵ��� */
        puc_buffer[uc_len++] = (oal_uint8)oal_bit_get_num_four_byte(pst_reg_class->ul_channel_bmap);

        /* ��ȡ����� */
        puc_buffer[uc_len++] = pst_reg_class->uc_max_reg_tx_pwr;
    }

    *puc_len = uc_len;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_5G

oal_uint32  mac_set_country_ie_5g(mac_regdomain_info_stru *pst_rd_info,
                                  oal_uint8               *puc_buffer,
                                  oal_uint8               *puc_len)
{
    mac_regclass_info_stru      *pst_reg_class = OAL_PTR_NULL;
    mac_country_reg_field_stru  *pst_reg_field = OAL_PTR_NULL;
    oal_uint8                    uc_chan_idx;
    oal_uint8                    uc_chan_num = 0;
    oal_uint8                    uc_len = 0;

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
        pst_reg_class = mac_get_channel_idx_rc_info(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (pst_reg_class == OAL_PTR_NULL) {
            continue;
        }

        mac_get_channel_num_from_idx(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);

        pst_reg_field = (mac_country_reg_field_stru *)puc_buffer;

        pst_reg_field->uc_first_channel = uc_chan_num;
        pst_reg_field->uc_channel_num   = 1;
        pst_reg_field->uc_max_tx_pwr    = pst_reg_class->uc_max_reg_tx_pwr;

        puc_buffer += MAC_COUNTRY_REG_FIELD_LEN;

        uc_len += MAC_COUNTRY_REG_FIELD_LEN;
    }

    *puc_len = uc_len;

    return OAL_SUCC;
}
#endif /* _PRE_WLAN_FEATURE_5G */
#endif


oal_void  mac_init_channel_list(oal_void)
{
    oal_uint8                uc_ch_idx;
    oal_uint8                uc_rc_num;
    oal_uint8                uc_freq;
    oal_uint8                uc_rc_idx;
    mac_regdomain_info_stru *pst_rd_info;
    mac_regclass_info_stru  *pst_rc_info = OAL_PTR_NULL;

    pst_rd_info = &g_st_mac_regdomain;

    /* �ȳ�ʼ�������ŵ��Ĺ�����Ϊ��Ч */
    for (uc_ch_idx = 0; uc_ch_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_ch_idx++) {
        g_ast_channel_list_2G[uc_ch_idx].uc_reg_class = MAC_INVALID_RC;
    }

#ifdef _PRE_WLAN_FEATURE_5G
    for (uc_ch_idx = 0; uc_ch_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_ch_idx++) {
        g_ast_channel_list_5G[uc_ch_idx].uc_reg_class = MAC_INVALID_RC;
    }
#endif /* _PRE_WLAN_FEATURE_5G */

    /* Ȼ����ݹ���������ŵ��Ĺ�������Ϣ */
    uc_rc_num = pst_rd_info->uc_regclass_num;

    /* ����2GƵ�����ŵ��Ĺ�������Ϣ */
    uc_freq = MAC_RC_START_FREQ_2;

    for (uc_rc_idx = 0; uc_rc_idx < uc_rc_num; uc_rc_idx++) {
        pst_rc_info = &(pst_rd_info->ast_regclass[uc_rc_idx]);

        for (uc_ch_idx = 0; uc_ch_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_ch_idx++) {
            if (mac_is_ch_supp_in_regclass(pst_rc_info, uc_freq, uc_ch_idx) == OAL_TRUE) {
                g_ast_channel_list_2G[uc_ch_idx].uc_reg_class = uc_rc_idx;
            }
        }
    }

#ifdef _PRE_WLAN_FEATURE_5G
    /* ����5GƵ�����ŵ��Ĺ�������Ϣ */
    uc_freq = MAC_RC_START_FREQ_5;

    for (uc_rc_idx = 0; uc_rc_idx < uc_rc_num; uc_rc_idx++) {
        pst_rc_info = &(pst_rd_info->ast_regclass[uc_rc_idx]);

        for (uc_ch_idx = 0; uc_ch_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_ch_idx++) {
            if (mac_is_ch_supp_in_regclass(pst_rc_info, uc_freq, uc_ch_idx) == OAL_TRUE) {
                g_ast_channel_list_5G[uc_ch_idx].uc_reg_class = uc_rc_idx;
            }
        }
    }
#endif /* _PRE_WLAN_FEATURE_5G */
}


oal_uint32  mac_get_channel_num_from_idx(oal_uint8 uc_band,
                                         oal_uint8 uc_idx,
                                         oal_uint8 *puc_channel_num)
{
    switch (uc_band) {
        case MAC_RC_START_FREQ_2:
            if (uc_idx >= MAC_CHANNEL_FREQ_2_BUTT) {
                return OAL_ERR_CODE_ARRAY_OVERFLOW;
            }

            *puc_channel_num = g_ast_channel_list_2G[uc_idx].uc_chan_number;
            break;

#ifdef _PRE_WLAN_FEATURE_5G
        case MAC_RC_START_FREQ_5:
            if (uc_idx >= MAC_CHANNEL_FREQ_5_BUTT) {
                return OAL_ERR_CODE_ARRAY_OVERFLOW;
            }

            *puc_channel_num = g_ast_channel_list_5G[uc_idx].uc_chan_number;
            break;
#endif /* _PRE_WLAN_FEATURE_5G */

        default:
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    return OAL_SUCC;
}


oal_uint32 mac_get_channel_idx_from_num(oal_uint8 uc_band, oal_uint8 uc_channel_num, oal_uint8 *puc_channel_idx)
{
    mac_channel_info_stru       *pst_channel = OAL_PTR_NULL;
    oal_uint8                    uc_total_channel_num  = 0;
    oal_uint8                    uc_idx;

    /* ����Ƶ�λ�ȡ�ŵ���Ϣ */
    switch (uc_band) {
        case MAC_RC_START_FREQ_2:
            pst_channel = g_ast_channel_list_2G;
            uc_total_channel_num = (oal_uint8)MAC_CHANNEL_FREQ_2_BUTT;
            break;

#ifdef _PRE_WLAN_FEATURE_5G
        case MAC_RC_START_FREQ_5:
            pst_channel = g_ast_channel_list_5G;
            uc_total_channel_num = (oal_uint8)MAC_CHANNEL_FREQ_5_BUTT;
            break;
#endif /* _PRE_WLAN_FEATURE_5G */

        default:
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* ����ŵ������� */
    for (uc_idx = 0; uc_idx < uc_total_channel_num; uc_idx++) {
        if (pst_channel[uc_idx].uc_chan_number == uc_channel_num) {
            *puc_channel_idx = uc_idx;
            return OAL_SUCC;
        }
    }

    return OAL_ERR_CODE_INVALID_CONFIG;
}


oal_uint32 mac_is_channel_idx_valid(oal_uint8 uc_band, oal_uint8 uc_ch_idx)
{
    mac_channel_info_stru  *pst_ch_info = OAL_PTR_NULL;

    switch (uc_band) {
        case MAC_RC_START_FREQ_2:
            if (uc_ch_idx >= MAC_CHANNEL_FREQ_2_BUTT) {
                return OAL_ERR_CODE_ARRAY_OVERFLOW;
            }
            pst_ch_info   = &(g_ast_channel_list_2G[uc_ch_idx]);
            break;

#ifdef _PRE_WLAN_FEATURE_5G
        case MAC_RC_START_FREQ_5:
            if (uc_ch_idx >= MAC_CHANNEL_FREQ_5_BUTT) {
                return OAL_ERR_CODE_ARRAY_OVERFLOW;
            }
            pst_ch_info   = &(g_ast_channel_list_5G[uc_ch_idx]);
            break;
#endif /* _PRE_WLAN_FEATURE_5G */

        default:
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_CHIP_TEST
    if (uc_ch_idx == MAC_CHANNEL14) {
        return OAL_SUCC;
    }
#endif

    if (pst_ch_info->uc_reg_class != MAC_INVALID_RC) {
        return OAL_SUCC;
    }

    return OAL_ERR_CODE_INVALID_CONFIG;
}

uint32_t mac_get_valid_channel_idx(uint8_t band, uint8_t vap_id, uint8_t channel_num, uint8_t *channel_idx)
{
    uint32_t ret;
    if (channel_idx == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    ret = mac_get_channel_idx_from_num(band, channel_num, channel_idx);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG2(vap_id, OAM_SF_ANY,
            "{mac_get_valid_channel_idx::get channel idx failed(band[%d],channel[%d])!}", band, channel_num);
        return ret;
    }
    ret = mac_is_channel_idx_valid(band, *channel_idx);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG3(vap_id, OAM_SF_ANY,
            "{mac_get_valid_channel_idx::channel_idx_valid failed(band[%d],channel[%d],idx[%d])!}",
            band, channel_num, *channel_idx);
    }
    return ret;
}


oal_uint32  mac_is_channel_num_valid(oal_uint8 uc_band, oal_uint8 uc_ch_num)
{
    oal_uint8  uc_ch_idx;
    oal_uint32 ul_ret;

    ul_ret = mac_get_channel_idx_from_num(uc_band, uc_ch_num, &uc_ch_idx);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    ul_ret = mac_is_channel_idx_valid(uc_band, uc_ch_idx);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    return OAL_SUCC;
}


mac_regclass_info_stru* mac_get_channel_idx_rc_info(oal_uint8 uc_band, oal_uint8 uc_ch_idx)
{
    oal_uint8               uc_max_ch_idx;
    mac_channel_info_stru  *pst_ch_info = OAL_PTR_NULL;

    switch (uc_band) {
        case MAC_RC_START_FREQ_2:
            uc_max_ch_idx = MAC_CHANNEL_FREQ_2_BUTT;
            pst_ch_info   = &(g_ast_channel_list_2G[uc_ch_idx]);
            break;
#ifdef _PRE_WLAN_FEATURE_5G
        case MAC_RC_START_FREQ_5:
            uc_max_ch_idx = MAC_CHANNEL_FREQ_5_BUTT;
            pst_ch_info   = &(g_ast_channel_list_5G[uc_ch_idx]);
            break;
#endif /* _PRE_WLAN_FEATURE_5G */

        default:
            return OAL_PTR_NULL;
    }

    if (uc_ch_idx >= uc_max_ch_idx) {
        return OAL_PTR_NULL;
    }

    if (pst_ch_info->uc_reg_class == MAC_INVALID_RC) {
        return OAL_PTR_NULL;
    }

    return &(g_st_mac_regdomain.ast_regclass[pst_ch_info->uc_reg_class]);
}


mac_regclass_info_stru*  mac_get_channel_num_rc_info(oal_uint8 uc_band, oal_uint8 uc_ch_num)
{
    oal_uint8   uc_channel_idx;

    if (mac_get_channel_idx_from_num(uc_band, uc_ch_num, &uc_channel_idx) != OAL_SUCC) {
        OAM_WARNING_LOG2(0, OAM_SF_ANY,
            "{mac_get_channel_num_rc_info::mac_get_channel_idx_from_num failed.band:%d, ch_num:%d", uc_band, uc_ch_num);
        return OAL_PTR_NULL;
    }

    return mac_get_channel_idx_rc_info(uc_band, uc_channel_idx);
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint32  mac_regdomain_set_country(oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_country_stru    *pst_country_param;
    mac_regdomain_info_stru *pst_mac_regdom;
    oal_uint8                uc_rc_num;
    oal_uint32               ul_size;
    int32_t ret;

    pst_country_param = (mac_cfg_country_stru *)puc_param;

    pst_mac_regdom = (mac_regdomain_info_stru *)pst_country_param->p_mac_regdom;

    /* ��ȡ������ĸ��� */
    uc_rc_num = pst_mac_regdom->uc_regclass_num;

    /* ������������ */
    ul_size = (oal_uint32)(OAL_SIZEOF(mac_regclass_info_stru) * uc_rc_num + MAC_RD_INFO_LEN);

    /* ���¹�������Ϣ */
    ret = memcpy_s((uint8_t *)&g_st_mac_regdomain, sizeof(g_st_mac_regdomain), (uint8_t *)pst_mac_regdom,
        ul_size);
    if (ret != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_regdomain_set_country::memcopy_s return value %d", ret);
        return OAL_FAIL;
    }
    /* �����ŵ��Ĺ�������Ϣ */
    mac_init_channel_list();

    return OAL_SUCC;
}


oal_int8*  mac_regdomain_get_country(oal_void)
{
    return g_st_mac_regdomain.ac_country;
}


oal_int32 mac_regdomain_enable_channel_14(oal_void)
{
    g_st_mac_regdomain.ast_regclass[0].ul_channel_bmap |= MAC_GET_CH_BIT(MAC_CHANNEL14);
    g_channel_14_enable = 1;
    mac_init_channel_list();
    return OAL_SUCC;
}


oal_int32 mac_regdomain_disable_channel_14(oal_void)
{
    g_st_mac_regdomain.ast_regclass[0].ul_channel_bmap &= ~(MAC_GET_CH_BIT(MAC_CHANNEL14));
    g_channel_14_enable = 0;
    mac_init_channel_list();
    return OAL_SUCC;
}
#endif // #if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_uint32  mac_regdomain_set_max_power(oal_uint8 uc_pwr, oal_bool_enum_uint8 en_exceed_reg)
{
    oal_uint8 uc_rc_idx;
    oal_uint8 uc_reg_pwr;

    for (uc_rc_idx = 0; uc_rc_idx < g_st_mac_regdomain.uc_regclass_num; uc_rc_idx++) {
        uc_reg_pwr = g_st_mac_regdomain.ast_regclass[uc_rc_idx].uc_max_reg_tx_pwr;

        if (uc_pwr <= uc_reg_pwr || en_exceed_reg == OAL_TRUE) {
            g_st_mac_regdomain.ast_regclass[uc_rc_idx].uc_max_tx_pwr = uc_pwr;
        } else {
            OAM_WARNING_LOG3(0, OAM_SF_TPC, "uc_pwr[%d] exceed reg_tx_pwr[%d], rc_idx[%d]",
                             uc_pwr, uc_reg_pwr, uc_rc_idx);
            g_st_mac_regdomain.ast_regclass[uc_rc_idx].uc_max_tx_pwr =
                g_st_mac_regdomain.ast_regclass[uc_rc_idx].uc_max_reg_tx_pwr;
        }
    }

    return OAL_SUCC;
}

/*lint -e19*/
oal_module_symbol(mac_get_channel_idx_from_num);
oal_module_symbol(mac_get_channel_num_from_idx);
oal_module_symbol(mac_is_channel_num_valid);
oal_module_symbol(mac_is_channel_idx_valid);
oal_module_symbol(mac_regdomain_set_country);
oal_module_symbol(mac_regdomain_get_country);
oal_module_symbol(mac_get_regdomain_info);
oal_module_symbol(mac_get_channel_idx_rc_info);
oal_module_symbol(mac_get_channel_num_rc_info);
oal_module_symbol(mac_regdomain_set_max_power);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
