

/* ͷ�ļ����� */
#include "mac_data.h"
#include "oal_net.h"
#include "wlan_types.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "mac_resource.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DATA_HOST_C

/* 2 ȫ�ֱ������� */
/* 3 ����ʵ�� */

oal_bool_enum_uint8 mac_is_dhcp_port(mac_ip_header_stru *pst_ip_hdr)
{
    udp_hdr_stru *pst_udp_hdr = OAL_PTR_NULL;
    /* DHCP�жϱ�׼: udpЭ�飬ipͷ��fragment offset�ֶ�Ϊ0��Ŀ�Ķ˿ں�Ϊ67��68 */
    if (pst_ip_hdr->uc_protocol == MAC_UDP_PROTOCAL && ((OAL_NET2HOST_SHORT(pst_ip_hdr->us_frag_off) & 0x1FFF) == 0)) {
        pst_udp_hdr = (udp_hdr_stru *)(pst_ip_hdr + 1);

        if (OAL_NET2HOST_SHORT(pst_udp_hdr->us_des_port) == 67 ||
            OAL_NET2HOST_SHORT(pst_udp_hdr->us_des_port) == 68) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 mac_is_nd(oal_ipv6hdr_stru  *pst_ipv6hdr)
{
    oal_icmp6hdr_stru      *pst_icmp6hdr = OAL_PTR_NULL;

    if (pst_ipv6hdr->nexthdr == OAL_IPPROTO_ICMPV6) {
        pst_icmp6hdr = (oal_icmp6hdr_stru *)(pst_ipv6hdr + 1);

        if ((pst_icmp6hdr->icmp6_type == MAC_ND_RSOL) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_RADVT) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_NSOL) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_NADVT) ||
            (pst_icmp6hdr->icmp6_type == MAC_ND_RMES)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 mac_is_dhcp6(oal_ipv6hdr_stru  *pst_ipv6hdr)
{
    udp_hdr_stru           *pst_udp_hdr = OAL_PTR_NULL;

    if (pst_ipv6hdr->nexthdr == MAC_UDP_PROTOCAL) {
        pst_udp_hdr = (udp_hdr_stru *)(pst_ipv6hdr + 1);

        if (pst_udp_hdr->us_des_port == OAL_HOST2NET_SHORT(MAC_IPV6_UDP_DES_PORT) ||
            pst_udp_hdr->us_des_port == OAL_HOST2NET_SHORT(MAC_IPV6_UDP_SRC_PORT)) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC mac_data_type_enum_uint8 mac_get_arp_type_by_arphdr(oal_eth_arphdr_stru  *pst_rx_arp_hdr)
{
    if (OAL_NET2HOST_SHORT(pst_rx_arp_hdr->us_ar_op) == MAC_ARP_REQUEST) {
        return MAC_DATA_ARP_REQ;
    } else if (OAL_NET2HOST_SHORT(pst_rx_arp_hdr->us_ar_op) == MAC_ARP_RESPONSE) {
        return MAC_DATA_ARP_RSP;
    }

    return MAC_DATA_BUTT;
}


oal_uint8 mac_get_data_type_from_8023(oal_uint8 *puc_frame_hdr, mac_netbuff_payload_type uc_hdr_type)
{
    mac_ip_header_stru     *pst_ip = OAL_PTR_NULL;
    oal_uint8              *puc_frame_body = OAL_PTR_NULL;
    oal_uint16              us_ether_type;
    oal_uint8               uc_datatype = MAC_DATA_BUTT;

    if (puc_frame_hdr == OAL_PTR_NULL) {
        return uc_datatype;
    }

    if (uc_hdr_type == MAC_NETBUFF_PAYLOAD_ETH) {
        us_ether_type  = ((mac_ether_header_stru *)puc_frame_hdr)->us_ether_type;
        puc_frame_body = puc_frame_hdr + (oal_uint16)OAL_SIZEOF(mac_ether_header_stru);
    } else if (uc_hdr_type == MAC_NETBUFF_PAYLOAD_SNAP) {
        us_ether_type = ((mac_llc_snap_stru *)puc_frame_hdr)->us_ether_type;
        puc_frame_body = puc_frame_hdr + (oal_uint16)OAL_SIZEOF(mac_llc_snap_stru);
    } else {
        return uc_datatype;
    }

    switch (us_ether_type) {
        /*lint -e778*/ /* ����Info -- Constant expression evaluates to 0 in operation '&' */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):
            /* ��IP TOS�ֶ�Ѱ�����ȼ� */
            /*----------------------------------------------------------------------
                tosλ����
             ----------------------------------------------------------------------
            | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
            | �����ȼ�  | ʱ�� | ������ | �ɿ��� | ����ɱ� | ���� |
             ----------------------------------------------------------------------*/
            pst_ip = (mac_ip_header_stru *)puc_frame_body;      /* ƫ��һ����̫��ͷ��ȡipͷ */

            if (mac_is_dhcp_port(pst_ip) == OAL_TRUE) {
                uc_datatype = MAC_DATA_DHCP;
            }
            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6):
            /* ��IPv6 traffic class�ֶλ�ȡ���ȼ� */
            /*----------------------------------------------------------------------
                IPv6��ͷ ǰ32Ϊ����
             -----------------------------------------------------------------------
            | �汾�� | traffic class   | ������ʶ |
            | 4bit   | 8bit(ͬipv4 tos)|  20bit   |
            -----------------------------------------------------------------------*/
            /* �����ND֡�������VO���з��� */
            if (mac_is_nd((oal_ipv6hdr_stru *)puc_frame_body) == OAL_TRUE) {
                uc_datatype = MAC_DATA_ND;
            /* �����DHCPV6֡ */
            } else if (mac_is_dhcp6((oal_ipv6hdr_stru *)puc_frame_body) == OAL_TRUE) {
                uc_datatype = MAC_DATA_DHCPV6;
            }

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_PAE):
            /* �����EAPOL֡�������VO���з��� */
            uc_datatype = MAC_DATA_EAPOL; /* eapol */
            break;

        /* TDLS֡��������������������ȼ�TID���� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_TDLS):
            uc_datatype = MAC_DATA_TDLS;
            break;

        /* PPPOE֡������������(���ֽ׶�, �Ự�׶�)��������ȼ�TID���� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC):
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES):
            uc_datatype = MAC_DATA_PPPOE;
            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_WAI):
            uc_datatype = MAC_DATA_WAPI;
            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN):
            uc_datatype = MAC_DATA_VLAN;

            break;

        case OAL_HOST2NET_SHORT(ETHER_TYPE_ARP):
            /* �����ARP֡�������VO���з��� */
            uc_datatype =  mac_get_arp_type_by_arphdr((oal_eth_arphdr_stru *)puc_frame_body);
            break;

        /*lint +e778*/
        default:
            uc_datatype = MAC_DATA_BUTT;
            break;
    }

    return uc_datatype;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_uint8 mac_get_data_type_from_80211(oal_netbuf_stru *pst_netbuff, oal_uint16 us_mac_hdr_len)
{
    oal_uint8               uc_datatype;
    mac_llc_snap_stru      *pst_snap = OAL_PTR_NULL;

    if (pst_netbuff == OAL_PTR_NULL) {
        return MAC_DATA_BUTT;
    }

    pst_snap = (mac_llc_snap_stru *)(OAL_NETBUF_DATA(pst_netbuff) + us_mac_hdr_len);

    uc_datatype = mac_get_data_type_from_8023((oal_uint8 *)pst_snap, MAC_NETBUFF_PAYLOAD_SNAP);

    return uc_datatype;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)


oal_uint8 mac_get_data_type(oal_netbuf_stru *pst_netbuff)
{
    oal_uint8               uc_datatype;
    mac_llc_snap_stru       *pst_snap = OAL_PTR_NULL;

    if (pst_netbuff == OAL_PTR_NULL) {
        return MAC_DATA_BUTT;
    }

    pst_snap = (mac_llc_snap_stru *)oal_netbuf_payload(pst_netbuff);
    if (pst_snap == OAL_PTR_NULL) {
        return MAC_DATA_BUTT;
    }

    uc_datatype = mac_get_data_type_from_8023((oal_uint8 *)pst_snap, MAC_NETBUFF_PAYLOAD_SNAP);

    return uc_datatype;
}


oal_uint16 mac_get_eapol_keyinfo(oal_netbuf_stru *pst_netbuff)
{
    oal_uint8                      uc_datatype;
    oal_uint8                     *puc_payload = OAL_PTR_NULL;

    uc_datatype = mac_get_data_type(pst_netbuff);
    if (uc_datatype != MAC_DATA_EAPOL) {
        return 0;
    }

    puc_payload = oal_netbuf_payload(pst_netbuff);
    if (puc_payload == OAL_PTR_NULL) {
        return 0;
    }

    return *(oal_uint16 *)(puc_payload + OAL_EAPOL_INFO_POS);
}


oal_uint8 mac_get_eapol_type(oal_netbuf_stru *pst_netbuff)
{
    oal_uint8                      uc_datatype;
    oal_uint8                     *puc_payload = OAL_PTR_NULL;

    uc_datatype = mac_get_data_type(pst_netbuff);
    if (uc_datatype != MAC_DATA_EAPOL) {
        return 0;
    }

    puc_payload = oal_netbuf_payload(pst_netbuff);
    if (puc_payload == OAL_PTR_NULL) {
        return 0;
    }

    return *(puc_payload + OAL_EAPOL_TYPE_POS);
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

oal_bool_enum_uint8 mac_is_eapol_key_ptk(mac_eapol_header_stru  *pst_eapol_header)
{
    mac_eapol_key_stru *pst_key = OAL_PTR_NULL;

    if (pst_eapol_header->uc_type == IEEE802_1X_TYPE_EAPOL_KEY) {
        if ((oal_uint16)(OAL_NET2HOST_SHORT(pst_eapol_header->us_length)) >=
            (oal_uint16)OAL_SIZEOF(mac_eapol_key_stru)) {
            pst_key = (mac_eapol_key_stru *)(pst_eapol_header + 1);
            if (pst_key->auc_key_info[1] & WPA_KEY_INFO_KEY_TYPE) {
                return OAL_TRUE;
            }
        }
    }
    return OAL_FALSE;
}
#endif // #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1131C_HOST)

/*lint -e19*/
oal_module_symbol(mac_is_dhcp_port);
oal_module_symbol(mac_is_dhcp6);
oal_module_symbol(mac_is_nd);
oal_module_symbol(mac_get_data_type_from_8023);
oal_module_symbol(mac_is_eapol_key_ptk);
oal_module_symbol(mac_get_data_type_from_80211);
oal_module_symbol(mac_get_data_type);
/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

