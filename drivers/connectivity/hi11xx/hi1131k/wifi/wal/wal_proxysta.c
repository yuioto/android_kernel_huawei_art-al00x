

#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wal_linux_bridge.h"
#include "mac_device.h"
#include "hmac_vap.h"
#include "mac_vap.h"
#include "wal_main.h"
#include "wal_proxysta.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_PROXYSTA_C

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

OAL_STATIC oal_uint32  wal_proxysta_handle_pre_route(oal_uint32 ul_hooknum,
                                                     oal_netbuf_stru *pst_netbuf,
                                                     OAL_CONST oal_net_device_stru *pst_in,
                                                     OAL_CONST oal_net_device_stru *pst_out,
                                                     oal_int32 (*p_okfn_func)(oal_netbuf_stru *));

OAL_STATIC oal_uint32  wal_proxysta_handle_forward(oal_uint32 ul_hooknum,
                                                   oal_netbuf_stru *pst_netbuf,
                                                   OAL_CONST oal_net_device_stru *pst_in,
                                                   OAL_CONST oal_net_device_stru *pst_out,
                                                   oal_int32 (*p_okfn_func)(oal_netbuf_stru *));

OAL_STATIC oal_uint32  wal_proxysta_handle_post_route(oal_uint32 ul_hooknum,
                                                      oal_netbuf_stru *pst_netbuf,
                                                      OAL_CONST oal_net_device_stru *pst_in,
                                                      OAL_CONST oal_net_device_stru *pst_out,
                                                      oal_int32 (*p_okfn_func)(oal_netbuf_stru *));

OAL_STATIC oal_uint32  wal_proxysta_handle_local_in(oal_uint32 ul_hooknum,
                                                    oal_netbuf_stru *pst_netbuf,
                                                    OAL_CONST oal_net_device_stru *pst_in,
                                                    OAL_CONST oal_net_device_stru *pst_out,
                                                    oal_int32 (*p_okfn_func)(oal_netbuf_stru *));

/* port types */
#define WAL_PROXYSTA_PTYPE_ETH      0x1000  /* 4096 Port type ethernet */
#define WAL_PROXYSTA_PTYPE_VAP      0x1001  /* 4097 Port type VAP */
#define WAL_PROXYSTA_PTYPE_PETH     0x1003  /* 4099 Port type proxy ethernet */
#define WAL_PROXYSTA_PTYPE_PVAP     0x1004  /* 4100 type proxy VAP */
/* liuming proxysta begin */
#define WAL_PROXYSTA_PTYPE_MPVAP	0x1005  /* 4101 Port type main proxy VAP */

/* liuming proxysta end */

/* Port flags */
#define WAL_PROXYSTA_PTYPE_FLAG_ISO         1 << 0    /* Port isolation enabled/disabled */

#define WAL_PROXYSTA_PFLAG_SHIFT            16
#define WAL_PROXYSTA_PTYPE_MASK             ((1 << WAL_PROXYSTA_PFLAG_SHIFT) - 1)
#define WAL_PROXYSTA_PTYPE(_type)           (_type & WAL_PROXYSTA_PTYPE_MASK)
#define WAL_PROXYSTA_PFLAG(_type)           (_type >> WAL_PROXYSTA_PFLAG_SHIFT)

#define WAL_PROXYSTA_PFLAG_IS_ISO(_flag)    (_flag & WAL_PROXYSTA_PTYPE_FLAG_ISO)
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
OAL_STATIC oal_nf_hook_ops_stru proxysta_brhooks[] __read_mostly = {
    {
        .hook     = wal_proxysta_handle_pre_route,
        .owner    = OAL_THIS_MODULE,
        .pf       = OAL_PF_BRIDGE,
        .hooknum  = OAL_NF_BR_PRE_ROUTING,
        .priority = NF_BR_PRI_NAT_DST_BRIDGED
    },
    {
        .hook     = wal_proxysta_handle_forward,
        .owner    = OAL_THIS_MODULE,
        .pf       = OAL_PF_BRIDGE,
        .hooknum  = OAL_NF_BR_FORWARD,
        .priority = NF_BR_PRI_FILTER_BRIDGED
    },
    {
        .hook     = wal_proxysta_handle_post_route,
        .owner    = OAL_THIS_MODULE,
        .pf       = OAL_PF_BRIDGE,
        .hooknum  = OAL_NF_BR_POST_ROUTING,
        .priority = NF_BR_PRI_NAT_SRC
    },
    {
        .hook     = wal_proxysta_handle_local_in,
        .owner    = OAL_THIS_MODULE,
        .pf       = OAL_PF_BRIDGE,
        .hooknum  = OAL_NF_BR_LOCAL_IN,
        .priority = NF_BR_PRI_FILTER_BRIDGED
    },
};
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
OAL_STATIC oal_nf_hook_ops_stru proxysta_brhooks[] = {
    {
        wal_proxysta_handle_pre_route,
        OAL_THIS_MODULE,
        OAL_PF_BRIDGE,
        {0},
        OAL_NF_BR_PRE_ROUTING,
        NF_BR_PRI_NAT_DST_BRIDGED
    },
    {
        wal_proxysta_handle_forward,
        OAL_THIS_MODULE,
        OAL_PF_BRIDGE,
        {0},
        OAL_NF_BR_FORWARD,
        NF_BR_PRI_FILTER_BRIDGED
    },
    {
        wal_proxysta_handle_post_route,
        OAL_THIS_MODULE,
        OAL_PF_BRIDGE,
        {0},
        OAL_NF_BR_POST_ROUTING,
        NF_BR_PRI_NAT_SRC
    },
    {
        wal_proxysta_handle_local_in,
        OAL_THIS_MODULE,
        OAL_PF_BRIDGE,
        {0},
        OAL_NF_BR_LOCAL_IN,
        NF_BR_PRI_FILTER_BRIDGED
    },
};
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
OAL_STATIC oal_int32 wal_proxysta_brroute_hook(oal_netbuf_stru *pst_netbuf)
{
    if (pst_netbuf->mark == OAL_PROXYSTA_MARK_ROUTE) {
        OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_brroute_hook::Allowing local delivery.}");
        return OAL_TRUE; /* route it */
    }

    return OAL_FALSE; /* bridge it */
}
#endif

OAL_STATIC oal_void wal_proxysta_dev_xmit(oal_netbuf_stru *pst_netbuf)
{
    if (pst_netbuf->len > pst_netbuf->dev->mtu) {
        oal_netbuf_free(pst_netbuf);
    } else {
        oal_netbuf_push(pst_netbuf, ETHER_HDR_LEN);
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
        wal_vap_start_xmit(pst_netbuf, pst_netbuf->dev);
#else
        wal_bridge_vap_xmit(pst_netbuf, pst_netbuf->dev);
#endif
    }
}


OAL_STATIC oal_uint32  wal_proxysta_find_net_dev(mac_device_proxysta_stru *pst_dev_psta,
                                                 oal_uint8                *puc_mac_addr,
                                                 oal_net_device_stru     **ppst_net_device)
{
    oal_uint8              uc_hash_value;
    mac_vap_stru          *pst_mav_vap = OAL_PTR_NULL;
    hmac_vap_stru         *pst_hmac_vap = OAL_PTR_NULL;
    oal_dlist_head_stru   *pst_entry = OAL_PTR_NULL;

    uc_hash_value = MAC_PROXYSTA_CALCULATE_HASH_VALUE(puc_mac_addr);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_dev_psta->ast_proxysta_hash[uc_hash_value]))
    {
        pst_mav_vap = OAL_DLIST_GET_ENTRY(pst_entry, mac_vap_stru, st_entry);
        if (oal_compare_mac_addr(pst_mav_vap->st_vap_proxysta.auc_oma, puc_mac_addr) == 0) {
            pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mav_vap->uc_vap_id);
            if (pst_hmac_vap == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(pst_mav_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_find_net_dev::pst_hmac_vap is null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            if (pst_hmac_vap->pst_net_device == OAL_PTR_NULL) {
                return OAL_FAIL;
            }

            *ppst_net_device = pst_hmac_vap->pst_net_device;

            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


OAL_STATIC oal_uint32  wal_proxysta_find_mainsta_net_dev(mac_device_proxysta_stru *pst_dev_psta,
                                                         oal_uint8                *puc_mac_addr,
                                                         oal_net_device_stru     **ppst_net_device)
{
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;

    /* ????????????mac??????main sta??mac????????????????mainsta???????? */
    if (oal_compare_mac_addr(pst_dev_psta->auc_mac_addr, puc_mac_addr) == 0) {
        /* ????main sta??hmac vap */
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(1);
        if (pst_hmac_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_find_net_dev::pst_hmac_vap is null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (pst_hmac_vap->pst_net_device == OAL_PTR_NULL) {
            return OAL_FAIL;
        }

        *ppst_net_device = pst_hmac_vap->pst_net_device;

        return OAL_SUCC;
    }

    return OAL_FAIL;
}


OAL_STATIC oal_uint32 wal_proxysta_get_psta_dev(OAL_CONST oal_net_device_stru *pst_net_device,
    mac_device_proxysta_stru **ppst_dev_psta)
{
    mac_device_stru   *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru      *pst_mac_vap = OAL_PTR_NULL;

    pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_device);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    *ppst_dev_psta = &(pst_mac_device->st_device_proxysta);

    return OAL_SUCC;
}


oal_uint32 wal_proxysta_add_vap(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru              *pst_event = OAL_PTR_NULL;
    mac_cfg_set_oma_param_stru  *pst_mac_cfg_set_oma = OAL_PTR_NULL;
    oal_uint8                    auc_proxysta_oma[WLAN_MAC_ADDR_LEN];
    mac_device_stru             *pst_mac_device = OAL_PTR_NULL;
    oal_int32                    l_ret;
    oal_uint32                   uc_hash_value;
    mac_vap_stru                *pst_mac_vap = OAL_PTR_NULL;

    if (OAL_UNLIKELY(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_add_vap::pst_event_mem is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mac_cfg_set_oma = (mac_cfg_set_oma_param_stru *)pst_event->auc_event_data;

    /* ????proxysta??oma???? */
    if (memcpy_s(auc_proxysta_oma, WLAN_MAC_ADDR_LEN, pst_mac_cfg_set_oma, WLAN_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_proxysta_add_vap::memcpy_s failed!");
        return OAL_FAIL;
    }

    /* ????mac_device_stru */
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_PROXYSTA,
            "{wal_proxysta_add_vap::get mac_device ptr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ????mac_vap_stru */
    pst_mac_vap = mac_res_get_mac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (pst_mac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_PROXYSTA,
            "{wal_proxysta_add_vap::get mac_vap ptr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ????????????Proxy STA?? ???????????????? */
    if (pst_mac_device->st_device_proxysta.uc_proxysta_cnt == 0) {
        l_ret = oal_nf_register_hooks(proxysta_brhooks, OAL_SIZEOF(proxysta_brhooks) / OAL_SIZEOF(proxysta_brhooks[0]));
        if (l_ret < 0) {
            OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_add_vap::Failed to register br hooks.}");
            return (oal_uint32)l_ret;
        }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
        /* register hook to intercept pkt to support local delivery */
        OAL_RCU_ASSIGN_POINTER(oal_br_should_route_hook, wal_proxysta_brroute_hook);
#endif
    }

    /* ????Proxy STA?? hash???? */
    uc_hash_value = MAC_PROXYSTA_CALCULATE_HASH_VALUE(auc_proxysta_oma);
    oal_dlist_add_head(&(pst_mac_vap->st_entry),
        &(pst_mac_device->st_device_proxysta.ast_proxysta_hash[uc_hash_value]));
    pst_mac_device->st_device_proxysta.uc_proxysta_cnt++;

    return OAL_SUCC;
}


oal_uint32 wal_proxysta_remove_vap(mac_vap_stru *pst_mac_vap)
{
    oal_uint32                   uc_hash_value;
    oal_uint8                    auc_proxysta_oma[WLAN_MAC_ADDR_LEN];
    oal_dlist_head_stru         *pst_hash_head = OAL_PTR_NULL;
    oal_dlist_head_stru         *pst_entry = OAL_PTR_NULL;
    mac_device_stru             *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru                *pst_vap = OAL_PTR_NULL;

    /* ????mac_device_stru */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
            "{wal_proxysta_remove_vap::get mac_device ptr is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* ??????????????proxysta oma???? */
    if (memcpy_s(auc_proxysta_oma, WLAN_MAC_ADDR_LEN, pst_mac_vap->st_vap_proxysta.auc_oma, WLAN_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_proxysta_remove_vap::memcpy_s failed!");
        return OAL_FAIL;
    }

    uc_hash_value = MAC_PROXYSTA_CALCULATE_HASH_VALUE(auc_proxysta_oma);
    pst_hash_head = &(pst_mac_device->st_device_proxysta.ast_proxysta_hash[uc_hash_value]);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, pst_hash_head)
    {
        pst_vap = OAL_DLIST_GET_ENTRY(pst_entry, mac_vap_stru, st_entry);
        if (oal_compare_mac_addr(pst_vap->st_vap_proxysta.auc_oma, auc_proxysta_oma) == 0) {
            oal_dlist_delete_entry(pst_entry);
            pst_mac_device->st_device_proxysta.uc_proxysta_cnt--;

            if (pst_mac_device->st_device_proxysta.uc_proxysta_cnt == 0) {
                oal_nf_unregister_hooks(proxysta_brhooks,
                    OAL_SIZEOF(proxysta_brhooks) / OAL_SIZEOF(proxysta_brhooks[0]));
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA, "{wal_proxysta_remove_vap::Remov hooks.}");

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
#else
                /* reset the br route hook */
                OAL_RCU_ASSIGN_POINTER(oal_br_should_route_hook, NULL);
#endif
            }
            return OAL_SUCC;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_proxysta_handle_pre_route(oal_uint32 ul_hooknum,
                                                     oal_netbuf_stru *pst_netbuf,
                                                     OAL_CONST oal_net_device_stru *pst_in,
                                                     OAL_CONST oal_net_device_stru *pst_out,
                                                     oal_int32 (*p_okfn_func)(oal_netbuf_stru *))
{
    oal_bool_enum_uint8 en_ismcast;
    mac_device_proxysta_stru *pst_dev_psta = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_device = OAL_PTR_NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_uint32 ul_type = ~0UL;
#else
    oal_uint32 ul_type = oal_br_get_port_type(pst_in->br_port);
#endif

    oal_uint32 ul_ptype = WAL_PROXYSTA_PTYPE(ul_type);
    oal_netbuf_stru *pst_netbuf_clone = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;

    OAM_INFO_LOG1(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_pre_route:ul_ptype:%d}", ul_ptype);

    en_ismcast = ETHER_IS_MULTICAST(oal_eth_hdr(pst_netbuf)->h_dest);

    ul_ret = wal_proxysta_get_psta_dev(pst_in, &pst_dev_psta);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret)) {
        OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_pre_route::get proxysta dev failed.}");
        return OAL_NF_DROP;
    }

    switch (ul_ptype) {
        case WAL_PROXYSTA_PTYPE_ETH:

        /* AP0??????????????????????????:
           1.AP0????STA1??????DHCP??????????????????OAL_NF_ACCEPT;
           2.AP0????STA1??????ARP??????????????????OAL_NF_ACCEPT;
           3.AP0????STA1??????ARP??????????????STA1??????STA1'??????????????????Root????????OAL_NF_ACCEPT;
           4.AP0????STA1??????ICMP ping??????OAL_NF_ACCEPT;
           5.AP0????STA1??????????????EAPOL????????OAL_NF_ACCEPT. */
        case WAL_PROXYSTA_PTYPE_VAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                "{wal_proxysta_handle_pre_route::received packet, start into AP0 process.}");

            pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_in);
            if (OAL_PTR_NULL == pst_mac_vap) {
                OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_pre_route::get mac_vap failed.}");
                return OAL_NF_DROP;
            }

            /* ??????????AP0????????????????????????AP0?????????????????? */
            if (0 == oal_compare_mac_addr(oal_eth_hdr(pst_netbuf)->h_dest, mac_mib_get_StationID(pst_mac_vap))) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::sk_buff dest address is AP0.}");
                return OAL_NF_ACCEPT;
            }

            /* AP0????STA1??????DHCP????ARP??????????????????OAL_NF_ACCEPT */
            if (en_ismcast) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::AP0 received multicast process.}");
                return OAL_NF_ACCEPT;
            }

            /* AP0????STA1??????sta0????????local in ???????????????????????????????? ???????? */
            if (0 == oal_compare_mac_addr(oal_eth_hdr(pst_netbuf)->h_dest, pst_dev_psta->auc_mac_addr)) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::sk_buff dest address is STA0.}");
                return OAL_NF_ACCEPT;
            }

            /* ??????AP0??????STA1??????Root AP?????????????? */
            /* AP0????STA1????????????????????????????????STA1'?????????????? */
            ul_ret = wal_proxysta_find_net_dev(pst_dev_psta, oal_eth_hdr(pst_netbuf)->h_source, &pst_net_device);
            if (OAL_SUCC != ul_ret) {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::wal_proxysta_find_net_dev don't find proxysta.}");
                return OAL_NF_DROP;
            }

            /* Clone????skb????skb2????STA1??????STA1'???????? */
            pst_netbuf_clone = oal_netbuf_clone(pst_netbuf);
            if (OAL_PTR_NULL == pst_netbuf_clone) {
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::clone sk_buff failed.}");
                return OAL_NF_DROP;
            }

            pst_netbuf_clone->dev = pst_net_device;

            /* ??skb??????????????????????????MAC????????????????forward?????? */
            pst_netbuf->mark = OAL_PROXYSTA_MARK_DROPPED;

            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                "{wal_proxysta_handle_pre_route::start send cloned sk_buff by proxysta.}");

            /* ????????STA1??????STA1'??clone????????????Root AP */
            wal_proxysta_dev_xmit(pst_netbuf_clone);

            return OAL_NF_ACCEPT;

        case WAL_PROXYSTA_PTYPE_PETH:

        /* proxysta??????????????????????????:(????????????????main sta??????root ap??????????????????????????????)
           1.proxysta????root_ap??????ARP??????????OAL_NF_ACCEPT;
           2.proxysta????root_ap??????ICMP ping??????OAL_NF_ACCEPT. */
        case WAL_PROXYSTA_PTYPE_PVAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                "{wal_proxysta_handle_pre_route::received packet, start into proxysta process.}");

            /* proxysta?????????????????????????? */
            if (OAL_PROXYSTA_MARK_DROPPED == pst_netbuf->mark) {
                OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::received packet which src address is equal with own address, "
                    "dropped.}");

                /* sk_buff mark???????? */
                pst_netbuf->mark = 0;
                return OAL_NF_DROP;
            }
            break;

        /* main sta??????????????????????????:
           1.main sta????root_ap??????DHCP??????????????????OAL_NF_ACCEPT;
           2.main sta????root_ap??????ARP??????????????????OAL_NF_ACCEPT;
           3.main sta????root_ap??????ARP??????????OAL_NF_ACCEPT;
           4.main sta????root_ap??????ICMP ping??????OAL_NF_ACCEPT. */
        case WAL_PROXYSTA_PTYPE_MPVAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                "{wal_proxysta_handle_pre_route::received packet, start into main sta process.}");

            /* main sta?????????????????????????? */
            if (OAL_PROXYSTA_MARK_DROPPED == pst_netbuf->mark) {
                OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_pre_route::received packet which src address is equal with own address, "
                    "dropped.}");
                /* sk_buff mark???????? */
                pst_netbuf->mark = 0;
                return OAL_NF_DROP;
            }
            break;

        default:
            break;
    }

    return OAL_NF_ACCEPT;
}


OAL_STATIC oal_uint32  wal_proxysta_handle_forward(oal_uint32 ul_hooknum,
                                                   oal_netbuf_stru *pst_netbuf,
                                                   OAL_CONST oal_net_device_stru *pst_in,
                                                   OAL_CONST oal_net_device_stru *pst_out,
                                                   oal_int32 (*p_okfn_func)(oal_netbuf_stru *))
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_uint32 ul_ptype = ~0UL;
#else
    oal_uint32 ul_ptype = WAL_PROXYSTA_PTYPE(oal_br_get_port_type(pst_in->br_port));
#endif
    oal_bool_enum_uint8 en_ismcast;

    OAM_INFO_LOG1(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_forward:ul_ptype:%d}", ul_ptype);
    en_ismcast = ETHER_IS_MULTICAST(oal_eth_hdr(pst_netbuf)->h_dest);

    if (en_ismcast) {
        OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_forward::received multicast packet.}");
        return OAL_NF_ACCEPT;
    }

    switch (ul_ptype) {
        case WAL_PROXYSTA_PTYPE_ETH:
        /* AP0????????????????????pre_route?????????????????????????? */
        case WAL_PROXYSTA_PTYPE_VAP:
            if (OAL_PROXYSTA_MARK_DROPPED == pst_netbuf->mark) {
                /* sk_buff mark???????? */
                pst_netbuf->mark = 0;
                return OAL_NF_DROP;
            }

            break;

        default:
            break;
    }

    return OAL_NF_ACCEPT;
}


OAL_STATIC oal_uint32  wal_proxysta_handle_post_route(oal_uint32 ul_hooknum,
                                                      oal_netbuf_stru *pst_netbuf,
                                                      OAL_CONST oal_net_device_stru *pst_in,
                                                      OAL_CONST oal_net_device_stru *pst_out,
                                                      oal_int32 (*p_okfn_func)(oal_netbuf_stru *))
{
    mac_device_proxysta_stru *pst_dev_psta = OAL_PTR_NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 35)) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    oal_uint32 ul_ptype = ~0UL;
#else
    oal_uint32 ul_ptype = WAL_PROXYSTA_PTYPE(oal_br_get_port_type(pst_out->br_port));
#endif
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint8 *puc_h_source = OAL_PTR_NULL;
    oal_uint8 *puc_d_source = OAL_PTR_NULL;
    oal_net_device_stru *pst_net_device = OAL_PTR_NULL;
    oal_bool_enum_uint8 en_ismcast;

    ul_ret = wal_proxysta_get_psta_dev(pst_out, &pst_dev_psta);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret)) {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::get proxysta dev failed.}");
        return OAL_NF_DROP;
    }
    OAM_INFO_LOG1(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route:ul_ptype:%d}", ul_ptype);

    en_ismcast = ETHER_IS_MULTICAST(oal_eth_hdr(pst_netbuf)->h_dest);
    puc_h_source = oal_eth_hdr(pst_netbuf)->h_source;
    puc_d_source = oal_eth_hdr(pst_netbuf)->h_dest;

    /* ???????????????????????????????????????????? */
    OAM_INFO_LOG4(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route:h_source_mac:%02X:XX:XX:%02X:%02X:%02X}",
        puc_h_source[0], puc_h_source[3], puc_h_source[4], puc_h_source[5]);

    OAM_INFO_LOG4(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route:d_source_mac:%02X:XX:XX:%02X:%02X:%02X}",
        puc_d_source[0], puc_d_source[3], puc_d_source[4], puc_d_source[5]);

    switch (ul_ptype) {
        /* AP0????????????????????????????????:
           1.AP0????STA0??????DHCP??????????????;
           2.AP0????STA0??????ARP??????????????;
           3.AP0????STA1'??????ARP????????????????????????????proxysta??oma??????OAL_NF_ACCEPT;
           4.AP0????STA1'??????ICMP ping????????????????????????proxysta??oma??????OAL_NF_ACCEPT. */
        case WAL_PROXYSTA_PTYPE_VAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::start into AP0 process.}");

            pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_out);
            if (OAL_PTR_NULL == pst_mac_vap) {
                OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::get mac vap failed.}");
                return OAL_NF_DROP;
            }

            return OAL_NF_ACCEPT;

        case WAL_PROXYSTA_PTYPE_PETH:

        /* proxysta????????????????????????????????:
           1.proxysta????STA1??????DHCP????????????????????????????????????proxysta??oma??????OAL_NF_ACCEPT;
           2.proxysta????STA1??????ARP????????????????????????????????????proxysta??oma??????OAL_NF_ACCEPT;
           3.proxysta????STA1??????ARP????????????????????????????proxysta??oma??????OAL_NF_ACCEPT;
           4.proxysta????STA1??????ICMP ping????????????????????????proxysta??oma??????OAL_NF_ACCEPT. */
        case WAL_PROXYSTA_PTYPE_PVAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::start into proxysta process.}");

            pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_out);
            if (OAL_PTR_NULL == pst_mac_vap) {
                OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::get mac vap failed.}");
                return OAL_NF_DROP;
            }

            /* ??????sta1????????????????????(sta1)??sta1'??oma(sta1)??????????????????????
               ??????sta1??????????????root_ap????????wal_bridge_vap_xmit??????,????????????sta1??????sta1?? */
            if ((0 == oal_compare_mac_addr(puc_h_source, pst_mac_vap->st_vap_proxysta.auc_oma)) ||
                (0 == oal_compare_mac_addr(puc_h_source, mac_mib_get_StationID(pst_mac_vap)))) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_post_route::source mac == oma mac.}");
                return OAL_NF_ACCEPT;
            }

            if (en_ismcast || (pst_netbuf->mark == OAL_WRAP_BR_MARK_FLOOD)) {
                OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::multicast or flood, dropped.}");
                return OAL_NF_DROP;
            } else {
                OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::proxysta tx ucast data}");

                ul_ret = wal_proxysta_find_mainsta_net_dev(pst_dev_psta,
                                                           oal_eth_hdr(pst_netbuf)->h_source,
                                                           &pst_net_device);
                if (OAL_SUCC != ul_ret) {
                    return OAL_NF_DROP;
                }

                pst_netbuf->dev = pst_net_device;

                OAM_INFO_LOG0(0, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_post_route::send unicast packet by main sta in proxysta process.}");

                wal_proxysta_dev_xmit(pst_netbuf);

                return OAL_NF_STOLEN;
            }

        /* main sta????????????????????????????????:
           1.main sta????????????????(main sta)??????????. */
        case WAL_PROXYSTA_PTYPE_MPVAP:
            OAM_INFO_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::start into main sta process.}");

            pst_mac_vap = (mac_vap_stru *)OAL_NET_DEV_PRIV(pst_out);
            if (OAL_PTR_NULL == pst_mac_vap) {
                OAM_WARNING_LOG0(0, OAM_SF_PROXYSTA, "{wal_proxysta_handle_post_route::get mac vap failed.}");
                return OAL_NF_DROP;
            }

            /* ????????????????mac??main sta?????????????????????????????????????????????????? */
            if (0 == oal_compare_mac_addr(puc_h_source, mac_mib_get_StationID(pst_mac_vap))) {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_post_route::send packet, which src address is equal with mainsta's.}");
                return OAL_NF_ACCEPT;
            } else {
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PROXYSTA,
                    "{wal_proxysta_handle_post_route::dropped packet, which src address is not equal with mainsta's.}");
                return OAL_NF_DROP;
            }

        default:
            break;
    }

    return OAL_NF_ACCEPT;
}


OAL_STATIC oal_uint32  wal_proxysta_handle_local_in(oal_uint32 ul_hooknum,
                                                    oal_netbuf_stru *pst_netbuf,
                                                    OAL_CONST oal_net_device_stru *pst_in,
                                                    OAL_CONST oal_net_device_stru *pst_out,
                                                    oal_int32 (*p_okfn_func)(oal_netbuf_stru *))
{
    return OAL_NF_ACCEPT;
}

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
