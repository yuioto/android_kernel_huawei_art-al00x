

#include "oal_net.h"
#include "mac_frame.h"
#include "mac_data.h"
#include "oal_cfg80211.h"
#include "oam_wdk.h"
#include "securec.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <net/genetlink.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_NET_C

#define OAL_HIGH_HALF_BYTE(a) (((a) & 0xF0) >> 4)
#define OAL_LOW_HALF_BYTE(a)  ((a) & 0x0F)

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

#define QUEUE_CONDITION2(_tos) ((((_tos) == 4) || ((_tos) == 5)) ? WLAN_UDP_VI_QUEUE : WLAN_UDP_VO_QUEUE)
#define QUEUE_CONDITION1(_tos) ((((_tos) == 1) || ((_tos) == 2)) ? WLAN_UDP_BK_QUEUE : QUEUE_CONDITION2(_tos))
#define WLAN_TOS_TO_HCC_QUEUE(_tos) ((((_tos) == 0) || ((_tos) == 3)) ? WLAN_UDP_BE_QUEUE : QUEUE_CONDITION1(_tos))

#define WLAN_DATA_VIP_QUEUE (WLAN_HI_QUEUE)
#endif

/*****************************************************************************
  2 ȫ�ֱ�������
*****************************************************************************/
#if ((_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION))

oal_net_device_stru  *g_past_net_device[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT] = {OAL_PTR_NULL};

oal_net_stru  g_st_init_net;

oal_sock_stru g_st_sock;
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
oal_wiphy_stru* g_pst_wiphy;
#endif
#endif

/*****************************************************************************
  3 ����ʵ��
*****************************************************************************/

oal_bool_enum_uint8 oal_netbuf_is_dhcp_port(oal_udp_header_stru *pst_udp_hdr)
{
    if (((OAL_HOST2NET_SHORT(pst_udp_hdr->source) == 68)
         && (OAL_HOST2NET_SHORT(pst_udp_hdr->dest) == 67))
        || ((OAL_HOST2NET_SHORT(pst_udp_hdr->source) == 67)
            && (OAL_HOST2NET_SHORT(pst_udp_hdr->dest) == 68))) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}



oal_bool_enum_uint8 oal_netbuf_is_nd(oal_ipv6hdr_stru  *pst_ipv6hdr)
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


oal_bool_enum_uint8 oal_netbuf_is_dhcp6(oal_ipv6hdr_stru  *pst_ipv6hdr)
{
    oal_udp_header_stru           *pst_udp_hdr = OAL_PTR_NULL;

    if (pst_ipv6hdr->nexthdr == MAC_UDP_PROTOCAL) {
        pst_udp_hdr = (oal_udp_header_stru *)(pst_ipv6hdr + 1);

        if (((OAL_HOST2NET_SHORT(pst_udp_hdr->source) == MAC_IPV6_UDP_SRC_PORT)
             && (OAL_HOST2NET_SHORT(pst_udp_hdr->dest) == MAC_IPV6_UDP_DES_PORT))
            || ((OAL_HOST2NET_SHORT(pst_udp_hdr->source) == MAC_IPV6_UDP_DES_PORT)
               && (OAL_HOST2NET_SHORT(pst_udp_hdr->dest) == MAC_IPV6_UDP_SRC_PORT))) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


#ifdef _PRE_WLAN_FEATURE_FLOWCTL

oal_void  oal_netbuf_get_txtid(oal_netbuf_stru *pst_buf, oal_uint8 *puc_tos)
{
    oal_ether_header_stru  *pst_ether_header;
    oal_ip_header_stru     *pst_ip = OAL_PTR_NULL;
    oal_vlan_ethhdr_stru   *pst_vlan_ethhdr = OAL_PTR_NULL;
    oal_uint32              ul_ipv6_hdr, ul_pri;
    oal_uint16              us_vlan_tci;
    oal_uint8               uc_tid;
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
    oal_tcp_header_stru    *pst_tcp = OAL_PTR_NULL;
#endif
    /* ��ȡ��̫��ͷ */
    pst_ether_header = (oal_ether_header_stru *)oal_netbuf_data(pst_buf);
    switch (pst_ether_header->us_ether_type) {
        /*lint -e778*/ /* ����Info -- Constant expression evaluates to 0 in operation '&' */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):
            /* ��IP TOS�ֶ�Ѱ�����ȼ� */
            /* ----------------------------------------------------------------------
                tosλ����
             ----------------------------------------------------------------------
            | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
            | �����ȼ�  | ʱ�� | ������ | �ɿ��� | ����ɱ� | ���� |
             ---------------------------------------------------------------------- */
            pst_ip = (oal_ip_header_stru *)(pst_ether_header + 1);      /* ƫ��һ����̫��ͷ��ȡipͷ */
            uc_tid = pst_ip->uc_tos >> WLAN_IP_PRI_SHIFT;
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
            /* ����chariot����Ľ������⴦����ֹ���� */
            if (pst_ip->uc_protocol == MAC_TCP_PROTOCAL) {
                pst_tcp = (oal_tcp_header_stru *)(pst_ip + 1);
                if ((OAL_HOST2NET_SHORT(MAC_CHARIOT_NETIF_PORT) == pst_tcp->us_dport)
                    || (OAL_HOST2NET_SHORT(MAC_CHARIOT_NETIF_PORT) == pst_tcp->us_sport)) {
                    uc_tid = WLAN_TIDNO_VOICE;
                }
            }
#endif
            break;
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6):
            /* ��IPv6 traffic class�ֶλ�ȡ���ȼ� */
            /* ----------------------------------------------------------------------
                IPv6��ͷ ǰ32Ϊ����
             -----------------------------------------------------------------------
            | �汾�� | traffic class   | ������ʶ |
            | 4bit   | 8bit(ͬipv4 tos)|  20bit   |
            ----------------------------------------------------------------------- */
            ul_ipv6_hdr = *((oal_uint32 *)(pst_ether_header + 1));  /* ƫ��һ����̫��ͷ��ȡipͷ */
            ul_pri = (OAL_NET2HOST_LONG(ul_ipv6_hdr) & WLAN_IPV6_PRIORITY_MASK) >> WLAN_IPV6_PRIORITY_SHIFT;
            uc_tid = (oal_uint8)(ul_pri >> WLAN_IP_PRI_SHIFT);
            break;
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PAE):
            uc_tid = WLAN_DATA_VIP_TID;
            break;
#ifdef _PRE_WLAN_FEATURE_WAPI
        case OAL_HOST2NET_SHORT(ETHER_TYPE_WAI):
            uc_tid = WLAN_DATA_VIP_TID;
            break;
#endif
        case OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN):
            /* ��ȡvlan tag�����ȼ� */
            pst_vlan_ethhdr = (oal_vlan_ethhdr_stru *)oal_netbuf_data(pst_buf);
            /* ------------------------------------------------------------------
                802.1Q(VLAN) TCI(tag control information)λ����
             -------------------------------------------------------------------
            |Priority | DEI  | Vlan Identifier |
            | 3bit    | 1bit |      12bit      |
             ------------------------------------------------------------------ */
            us_vlan_tci = OAL_NET2HOST_SHORT(pst_vlan_ethhdr->h_vlan_TCI);
            uc_tid = us_vlan_tci >> OAL_VLAN_PRIO_SHIFT;    /* ����13λ����ȡ��3λ���ȼ� */
            break;
        /*lint +e778*/
        default:
            break;
    }
    /* ���θ�ֵ */
    *puc_tos = uc_tid;
    return;
}
#endif


oal_bool_enum_uint8 oal_netbuf_is_tcp_ack(oal_ip_header_stru  *pst_ip_hdr)
{
    oal_tcp_header_stru    *pst_tcp_hdr;
    oal_uint32              ul_ip_pkt_len;
    oal_uint32              ul_ip_hdr_len;
    oal_uint32              ul_tcp_hdr_len;

    pst_tcp_hdr     = (oal_tcp_header_stru *)(pst_ip_hdr + 1);
    ul_ip_pkt_len   = OAL_NET2HOST_SHORT(pst_ip_hdr->us_tot_len);
    ul_ip_hdr_len   = (OAL_LOW_HALF_BYTE(pst_ip_hdr->us_ihl)) << 2;
    ul_tcp_hdr_len  = (OAL_HIGH_HALF_BYTE(pst_tcp_hdr->uc_offset)) << 2;

    if (ul_tcp_hdr_len + ul_ip_hdr_len == ul_ip_pkt_len) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 oal_netbuf_is_icmp(oal_ip_header_stru  *pst_ip_hdr)
{
    oal_uint8  uc_protocol;
    uc_protocol = pst_ip_hdr->uc_protocol;

    /* ICMP���ļ�� */
    if (uc_protocol == MAC_ICMP_PROTOCAL) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL


oal_bool_enum_uint8 oal_netbuf_is_tcp_ack6(oal_ipv6hdr_stru  *pst_ipv6hdr)
{
    oal_tcp_header_stru    *pst_tcp_hdr;
    oal_uint32              ul_ip_pkt_len;
    oal_uint32              ul_tcp_hdr_len;

    pst_tcp_hdr     = (oal_tcp_header_stru *)(pst_ipv6hdr + 1);
    ul_ip_pkt_len   = OAL_NET2HOST_SHORT(pst_ipv6hdr->payload_len); /* ipv6 ���غ�, ipv6����ͷ���̶�Ϊ40�ֽ� */
    ul_tcp_hdr_len  = (OAL_HIGH_HALF_BYTE(pst_tcp_hdr->uc_offset)) << 2;

    if (ul_tcp_hdr_len == ul_ip_pkt_len) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_uint16 oal_netbuf_select_queue(oal_netbuf_stru *pst_buf)
{
    oal_ether_header_stru  *pst_ether_header;
    oal_ip_header_stru     *pst_ip = OAL_PTR_NULL;
    oal_ipv6hdr_stru       *pst_ipv6 = OAL_PTR_NULL;
    oal_udp_header_stru    *pst_udp_hdr = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_SCHEDULE
    oal_tcp_header_stru    *pst_tcp_hdr;
#endif
    oal_vlan_ethhdr_stru   *pst_vlan_ethhdr = OAL_PTR_NULL;
    oal_uint32              ul_ipv6_hdr;
    oal_uint32              ul_pri;
    oal_uint16              us_vlan_tci;
    oal_uint8               uc_tos;
    oal_uint8               us_queue = WLAN_NORMAL_QUEUE;
    /* ��ȡ��̫��ͷ */
    pst_ether_header = (oal_ether_header_stru *)oal_netbuf_data(pst_buf);
    ul_ipv6_hdr = 0;
    switch (pst_ether_header->us_ether_type) {
        /*lint -e778*/
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IP):
            pst_ip = (oal_ip_header_stru *)(pst_ether_header + 1);      /* ƫ��һ����̫��ͷ��ȡipͷ */
            /* ��udp��������qos��� */
            if (pst_ip->uc_protocol == MAC_UDP_PROTOCAL) {
                /* ��IP TOS�ֶ�Ѱ�����ȼ� */
                /* ----------------------------------------------------------------------
                    tosλ����
                 ----------------------------------------------------------------------
                | bit7~bit5 | bit4 |  bit3  |  bit2  |   bit1   | bit0 |
                | �����ȼ�  | ʱ�� | ������ | �ɿ��� | ����ɱ� | ���� |
                 ---------------------------------------------------------------------- */
                uc_tos = pst_ip->uc_tos >> WLAN_IP_PRI_SHIFT;
                us_queue = WLAN_TOS_TO_HCC_QUEUE(uc_tos);

                /* �����DHCP֡�������DATA_HIGH_QUEUE */
                pst_udp_hdr = (oal_udp_header_stru *)(pst_ip + 1);
                if (((pst_ip->us_frag_off & 0xFF1F) == 0) && (oal_netbuf_is_dhcp_port(pst_udp_hdr) == OAL_TRUE)) {
                    us_queue = WLAN_DATA_VIP_QUEUE;
                }
            } else if (pst_ip->uc_protocol == MAC_TCP_PROTOCAL) { /* ����TCP ack��TCP data���� */
                if (oal_netbuf_is_tcp_ack(pst_ip) == OAL_TRUE) {
                    us_queue = WLAN_TCP_ACK_QUEUE;
                } else {
                    us_queue = WLAN_TCP_DATA_QUEUE;
                }
            }
            break;
        case OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6):
            /* ��IPv6 traffic class�ֶλ�ȡ���ȼ� */
            /* ----------------------------------------------------------------------
                IPv6��ͷ ǰ32Ϊ����
             -----------------------------------------------------------------------
            | �汾�� | traffic class   | ������ʶ |
            | 4bit   | 8bit(ͬipv4 tos)|  20bit   |
            ----------------------------------------------------------------------- */
            pst_ipv6    = (oal_ipv6hdr_stru *)(pst_ether_header + 1); /* ƫ��һ����̫��ͷ��ȡipͷ */
            if (memcpy_s(&ul_ipv6_hdr, sizeof(oal_uint32), pst_ipv6, sizeof(oal_uint32)) != EOK) {
                OAL_IO_PRINT("memcpy_s failed!\n");
            }
            if (pst_ipv6->nexthdr == MAC_UDP_PROTOCAL)  { /* UDP���� */
                ul_pri = OAL_NET2HOST_LONG(ul_ipv6_hdr);
                ul_pri = (ul_pri & WLAN_IPV6_PRIORITY_MASK) >> WLAN_IPV6_PRIORITY_SHIFT;
                uc_tos = (oal_uint8)(ul_pri >> WLAN_IP_PRI_SHIFT);
                us_queue = WLAN_TOS_TO_HCC_QUEUE(uc_tos);
            } else if (pst_ipv6->nexthdr == MAC_TCP_PROTOCAL) { /* TCP���� */
                if (oal_netbuf_is_tcp_ack6(pst_ipv6) == OAL_TRUE) {
                    us_queue = WLAN_TCP_ACK_QUEUE;
                } else {
                    us_queue = WLAN_TCP_DATA_QUEUE;
                }
            } else if (oal_netbuf_is_dhcp6((oal_ipv6hdr_stru *)(pst_ether_header + 1)) == OAL_TRUE) {
                /* �����DHCPV6֡�������WLAN_DATA_VIP_QUEUE���л��� */
                us_queue = WLAN_DATA_VIP_QUEUE;
            }
            break;
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PAE): /* �����EAPOL֡�������VO���з��� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_TDLS): /* TDLS֡��������������������ȼ�TID���� */
            us_queue = WLAN_DATA_VIP_QUEUE;
            break;
        /* PPPOE֡������������(���ֽ׶�, �Ự�׶�)��������ȼ�TID���� */
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC):
        case OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES):
            us_queue = WLAN_DATA_VIP_QUEUE;
            break;
#ifdef _PRE_WLAN_FEATURE_WAPI
        case OAL_HOST2NET_SHORT(ETHER_TYPE_WAI):
            us_queue = WLAN_DATA_VIP_QUEUE;
            break;
#endif
        case OAL_HOST2NET_SHORT(ETHER_TYPE_VLAN):
            /* ��ȡvlan tag�����ȼ� */
            pst_vlan_ethhdr = (oal_vlan_ethhdr_stru *)oal_netbuf_data(pst_buf);
            /* ------------------------------------------------------------------
                802.1Q(VLAN) TCI(tag control information)λ����
             -------------------------------------------------------------------
            |Priority | DEI  | Vlan Identifier |
            | 3bit    | 1bit |      12bit      |
             ------------------------------------------------------------------ */
            us_vlan_tci = OAL_NET2HOST_SHORT(pst_vlan_ethhdr->h_vlan_TCI);

            uc_tos = us_vlan_tci >> OAL_VLAN_PRIO_SHIFT;    /* ����13λ����ȡ��3λ���ȼ� */
            us_queue = WLAN_TOS_TO_HCC_QUEUE(uc_tos);
            break;
        /*lint +e778*/
        default:
            us_queue = WLAN_NORMAL_QUEUE;
            break;
    }
    return us_queue;
}

/*lint -e19*/
oal_module_symbol(oal_netbuf_is_tcp_ack6);
oal_module_symbol(oal_netbuf_select_queue);
/*lint +e19*/
#endif

#if 0 /* hi1102 ����Generic�������ɹ����˷��������ٶ�λ */

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/* netlink attributes ����ͨ��ö�������ҵ���Ӧ������
*�û��ռ�Ӧ�ó���Ҫ������������Ϣ */
enum {
    DOC_EXMPL_A_UNSPEC,
    DOC_EXMPL_A_MSG,
    __DOC_EXMPL_A_MAX,
};
#define DOC_EXMPL_A_MAX (__DOC_EXMPL_A_MAX - 1)

/* atribute policy���Ƕ���������Եľ������ͣ��μ�net/netlink.h */
static struct nla_policy doc_exmpl_genl_policy[DOC_EXMPL_A_MAX + 1] = {
    [DOC_EXMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

#define VERSION_NR 1

struct genl_info g_st_info;

// generic netlink family ����
static struct genl_family doc_exmpl_genl_family = { // 1 ����family
    .id = GENL_ID_GENERATE,
    .hdrsize = 0,
    .name = "HiSi_WIFI_EXCP",
    // .name = "CONTROL_EXMPL",
    .version = VERSION_NR,
    .maxattr = DOC_EXMPL_A_MAX,
};

/* �����������ͣ��û��ռ��Դ���������Ҫִ�е����� */
enum {
    DOC_EXMPL_C_UNSPEC,
    DOC_EXMPL_C_ECHO,
    __DOC_EXMPL_C_MAX,
};
#define DOC_EXMPL_C_MAX (__DOC_EXMPL_C_MAX - 1)

// echo command handler,����һ��msg���ظ�
int doc_exmpl_echo(struct sk_buff *skb2, struct genl_info *info)
{
    struct nlattr *na;
    struct sk_buff *skb;
    int rc;
    void *msg_hdr;
    char *data;
    if (info == NULL)
        goto error;

    oal_memcopy(&g_st_info, info, OAL_SIZEOF(*info));

    // ����ÿ�����ԣ�genl_info����attrs��������������ṹ��������payload
    na = info->attrs[DOC_EXMPL_A_MSG];
    if (na) {
        data = (char *)nla_data(na);
        if (!data)
            printk("apr--->Receive data error!\n");
        else
            printk("apr--->Recv:%s\n", data);
    } else {
        printk("No info->attrs %d\n", DOC_EXMPL_A_MSG);
    }

    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb)
        goto error;

    /* ������Ϣͷ������ԭ����
    genlmsg_put(struct sk_buff *,int pid,int seq_number,
            struct genl_family *,int flags,u8 command_index);
    */
    msg_hdr = oal_genlmsg_put(skb, 0, info->snd_seq + 1, &doc_exmpl_genl_family, 0, DOC_EXMPL_C_ECHO);
    if (msg_hdr == NULL) {
        rc = -ENOMEM;
        goto error;
    }

    // �������netlink attribute:DOC_EXMPL_A_MSG������ʵ��Ҫ��������
    rc = nla_put_string(skb, DOC_EXMPL_A_MSG, "This is a msg from kernel space!");
    if (rc != OAL_SUCC)
        goto error;

    genlmsg_end(skb, msg_hdr); // ��Ϣ�������
    // �������͸��û��ռ��ĳ������
    rc = genlmsg_unicast(genl_info_net(info), skb, info->snd_portid);
    if (rc != OAL_SUCC) {
        printk("apr--->Unicast to process:%d failed!\n", info->snd_portid);
        goto error;
    }
    printk("apr--->Unicast to process:%d failed!\n", info->snd_portid);
    return OAL_SUCC;

error:
    printk("apr--->Send:%s.\n", "This is a msg from kernel space!");
    return OAL_SUCC;
}

// ������command echo�;����handler��Ӧ����
static struct genl_ops doc_exmpl_genl_ops_echo = { // 2 ����operation
    .cmd = DOC_EXMPL_C_ECHO,
    .flags = 0,
    .policy = doc_exmpl_genl_policy,
    .doit = doc_exmpl_echo,
    .dumpit = NULL,
};

// �ں���ڣ�ע��generic netlink family/operations
oal_long genKernel_init(oal_void)
{
    int rc;
    printk("apr--->Generic Netlink Example Module inserted.\n");

    rc = genl_register_family(&doc_exmpl_genl_family); // 3 ע��family
    if (rc != OAL_SUCC) {
        goto failure;
    }
    rc = genl_register_ops(&doc_exmpl_genl_family, &doc_exmpl_genl_ops_echo); // 4 ע��operation
    if (rc != OAL_SUCC) {
        printk("apr--->Register ops: %i\n", rc);
        genl_unregister_family(&doc_exmpl_genl_family);
        goto failure;
    }
    return OAL_SUCC;

failure:
    printk("apr--->Error occured while inserting generic netlink example module\n");
    return -1;
}

oal_void genKernel_exit(oal_void)
{
    int ret;
    printk("apr--->Generic Netlink Example Module unloaded.\n");

    ret = genl_unregister_ops(&doc_exmpl_genl_family, &doc_exmpl_genl_ops_echo);
    if (ret != OAL_SUCC) {
        printk("apr--->Unregister ops failed: %i\n", ret);
        return;
    }
    ret = genl_unregister_family(&doc_exmpl_genl_family);
    if (ret != OAL_SUCC) {
        printk("apr--->Unregister family failed:%i\n", ret);
        return;
    }
}


/**
* genl_msg_send_to_user - ͨ��generic netlink�������ݵ�netlink
*
* @data: �������ݻ���
* @len:  ���ݳ��� ��λ��byte
* @pid:  ���͵��Ŀͻ���pid <span style="color:#FF0000;"><strong>���pidҪ���û��ռ䷢�����ݴ�����doit�е�info->snd_pid�������</strong></span>
*
* return:
*    0:       �ɹ�
*    -1:      ʧ��
*/
oal_long genl_msg_send_to_user(oal_void *data, oal_long i_len)
{
    struct sk_buff *skb;
    int rc;
    void *msg_hdr;

    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb)
        goto error;

    /* ������Ϣͷ������ԭ����
    genlmsg_put(struct sk_buff *,int pid,int seq_number,
            struct genl_family *,int flags,u8 command_index);
    */
    msg_hdr = oal_genlmsg_put(skb, g_st_info.snd_portid, 0, &doc_exmpl_genl_family, 0, DOC_EXMPL_C_ECHO);
    if (msg_hdr == NULL) {
        rc = -ENOMEM;
        goto error;
    }

    // �������netlink attribute:DOC_EXMPL_A_MSG������ʵ��Ҫ��������
    rc = nla_put_string(skb, DOC_EXMPL_A_MSG, data);
    if (rc != OAL_SUCC)
        goto error;

    genlmsg_end(skb, msg_hdr); // ��Ϣ�������
    // �������͸��û��ռ��ĳ������
    rc = genlmsg_unicast(genl_info_net(&g_st_info), skb, g_st_info.snd_portid);
    if (rc != OAL_SUCC) {
        printk("apr--->Unicast to process:%d failed!\n", g_st_info.snd_portid);
        goto error;
    }
    printk("apr--->Unicast to process:%d OK!\n", g_st_info.snd_portid);
    return OAL_SUCC;

error:
    printk("apr--->Send:%s.\n", "This is a msg from kernel space!");
    return OAL_SUCC;
    return OAL_SUCC;
}
#else
oal_long genKernel_init(oal_void)
{
    return OAL_SUCC;
}

oal_void genKernel_exit(oal_void)
{
    return;
}
oal_long genl_msg_send_to_user(oal_void *data, oal_long i_len)
{
    return OAL_SUCC;
}

#endif

oal_module_symbol(genKernel_init);
oal_module_symbol(genKernel_exit);

#endif /* hi1102 ����Generic�������ɹ����˷��������ٶ�λ */

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined (_PRE_WLAN_FEATURE_DFR)

struct dev_excp_globals {
    oal_sock_stru *nlsk;
    oal_uint8      mode;
    oal_uint8     *data;
    oal_uint32     usepid;
};

struct dev_netlink_msg_hdr_stru {
    oal_uint32       cmd;
    oal_uint32       len;
};

struct dev_excp_globals dev_excp_handler_data;

#define OAL_EXCP_DATA_BUF_LEN (64)
/*
 * Prototype    : dev_netlink_rev
 * Description  : receive netlink date from app
 * Input        : void
 * Return Value : void
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/10/8
 *     Author       : xudayong
 *     Modification : Created function
 *
 */
void dev_netlink_rev(oal_netbuf_stru *skb)
{
    oal_netbuf_stru                *pst_skb;
    oal_nlmsghdr_stru              *pst_nlh;
    struct dev_netlink_msg_hdr_stru st_msg_hdr = {0};
    oal_uint32                      ul_len;

    if (skb == NULL) {
        return;
    }

    OAL_IO_PRINT("WIFI DFR:dev_kernel_netlink_recv.\n");

    pst_skb = OAL_PTR_NULL;
    pst_nlh = OAL_PTR_NULL;

    memset_s(dev_excp_handler_data.data, OAL_EXCP_DATA_BUF_LEN, 0, OAL_EXCP_DATA_BUF_LEN);

    pst_skb = oal_netbuf_get(skb);
    if (pst_skb->len >= OAL_NLMSG_SPACE(0)) {
        pst_nlh = oal_nlmsg_hdr(pst_skb);
        /* ��ⱨ�ĳ�����ȷ�� */
        if (!OAL_NLMSG_OK(pst_nlh, pst_skb->len)) {
            OAL_IO_PRINT("[ERROR]invaild netlink buff data packge data len = :%u,skb_buff data len = %u\n",
                         pst_nlh->nlmsg_len, pst_skb->len);
            kfree_skb(pst_skb);
            return;
        }
        ul_len   = OAL_NLMSG_PAYLOAD(pst_nlh, 0);
        /* ������Ҫ����sizeof(st_msg_hdr),���ж�֮ */
        if (ul_len < OAL_EXCP_DATA_BUF_LEN && ul_len >= sizeof(st_msg_hdr)) {
            if (memcpy_s(dev_excp_handler_data.data, OAL_EXCP_DATA_BUF_LEN,
                         OAL_NLMSG_DATA(pst_nlh), ul_len) != EOK) {
                OAL_IO_PRINT("memcpy_s failed!\n");
                kfree_skb(pst_skb);
                return;
            }
        } else {
            OAL_IO_PRINT("[ERROR]invaild netlink buff len:%u,max len:%u\n", ul_len, OAL_EXCP_DATA_BUF_LEN);
            kfree_skb(pst_skb);
            return;
        }
        if (memcpy_s((void *)&st_msg_hdr, sizeof(st_msg_hdr),
                     dev_excp_handler_data.data, sizeof(st_msg_hdr)) != EOK) {
            OAL_IO_PRINT("memcpy_s failed!\n");
            kfree_skb(pst_skb);
            return;
        }

        if (st_msg_hdr.cmd == 0) {
            dev_excp_handler_data.usepid = pst_nlh->nlmsg_pid;   /* pid of sending process */
            OAL_IO_PRINT("WIFI DFR:pid is [%d], msg is [%s]\n",
                         dev_excp_handler_data.usepid, &dev_excp_handler_data.data[sizeof(st_msg_hdr)]);
        }
    }

    kfree_skb(pst_skb);
    return;
}


/*
 * Prototype    : dev_netlink_create
 * Description  : create netlink for device exception
 * Input        : void
 * Return Value : int
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/10/8
 *     Author       : xudayong
 *     Modification : Created function
 *
 */

oal_int32 dev_netlink_create(void)
{
    dev_excp_handler_data.nlsk = oal_netlink_kernel_create(&OAL_INIT_NET, NETLINK_DEV_ERROR, 0,
                                                           dev_netlink_rev, OAL_PTR_NULL, OAL_THIS_MODULE);
    if (dev_excp_handler_data.nlsk == OAL_PTR_NULL) {
        OAL_IO_PRINT("WIFI DFR:fail to create netlink socket \n");
        return -OAL_EFAIL;
    }

    OAL_IO_PRINT("WIFI DFR:suceed to create netlink socket, %p \n", dev_excp_handler_data.nlsk);
    return OAL_SUCC;
}


/*
 * Prototype    : dev_netlink_send
 * Description  : send netlink data
 * Input        : void
 * Return Value : int
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/10/8
 *     Author       : xudayong
 *     Modification : Created function
 *
 */
oal_int32 dev_netlink_send (const oal_uint8 *data, oal_long data_len)
{
    oal_netbuf_stru        *skb;
    oal_nlmsghdr_stru      *nlh;
    oal_uint32              ret;
    oal_uint32              len;

    if (data_len < 0) {
        OAL_IO_PRINT("WIFI DFR:dev error:data_len[%lu].\n", data_len);
        return -OAL_EFAIL;
    }

    len = OAL_NLMSG_SPACE((oal_ulong)data_len);
    skb = alloc_skb(len, GFP_KERNEL);
    if (skb == NULL) {
        OAL_IO_PRINT("WIFI DFR:dev error: allocate failed, len[%d].\n", len);
        return -OAL_EFAIL;
    }
    nlh = oal_nlmsg_put(skb, 0, 0, 0, data_len, 0);

    OAL_IO_PRINT("WIFI DFR: data[%s].\n", data);

    if (data_len > 0) {
        if (memcpy_s(OAL_NLMSG_DATA(nlh), data_len, data, data_len) != EOK) {
            OAL_IO_PRINT("memcpy_s failed!\n");
            kfree_skb(skb);
            return -OAL_EFAIL;
        }
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))// ��ǰ�ں�û��portid����ֶ�
    OAL_NETLINK_CB(skb).portid = 0;                 /* from kernel */
#endif
    if (dev_excp_handler_data.nlsk == OAL_PTR_NULL) {
        OAL_IO_PRINT("WIFI DFR: NULL Pointer_sock.\n");
        kfree_skb(skb);
        return -OAL_EFAIL;
    }

    ret = oal_netlink_unicast(dev_excp_handler_data.nlsk, skb, dev_excp_handler_data.usepid, MSG_DONTWAIT);
    if (ret <= 0) {
        OAL_IO_PRINT("WIFI DFR:send dev error netlink msg, ret = %d \n", ret);
    }

    return ret;
}


/*
 * Prototype    : init_dev_excp_handler
 * Description  : init dev exception handler
 * Input        : void
 * Return Value : int
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/10/8
 *     Author       : xudayong
 *     Modification : Created function
 *
 */
oal_int32 init_dev_excp_handler(oal_void)
{
    oal_int32   ret;

    OAL_IO_PRINT("DFR: into init_exception_enable_handler\n");

    memset_s((oal_uint8 *)&dev_excp_handler_data, OAL_SIZEOF(dev_excp_handler_data), 0,
             OAL_SIZEOF(dev_excp_handler_data));

    dev_excp_handler_data.data = (oal_uint8 *)kzalloc(OAL_EXCP_DATA_BUF_LEN, GFP_KERNEL);
    if (dev_excp_handler_data.data == OAL_PTR_NULL) {
        OAL_IO_PRINT("DFR: alloc dev_excp_handler_data.puc_data fail, len = %d.\n", OAL_EXCP_DATA_BUF_LEN);
        dev_excp_handler_data.data = OAL_PTR_NULL;
        return -OAL_EFAIL;
    }

    memset_s(dev_excp_handler_data.data, OAL_EXCP_DATA_BUF_LEN, 0, OAL_EXCP_DATA_BUF_LEN);

    ret = dev_netlink_create();
    if (ret < 0) {
        kfree(dev_excp_handler_data.data);
        OAL_IO_PRINT("init_dev_err_kernel init is ERR!\n");
        return -OAL_EFAIL;
    }

    OAL_IO_PRINT("DFR: init_exception_enable_handler init ok.\n");

    return OAL_SUCC;
}

/*
 * Prototype    : deinit_dev_excp_handler
 * Description  : deinit dev exception handler
 * Input        : void
 * Return Value : void
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/10/8
 *     Author       : xudayong
 *     Modification : Created function
 *
 */
oal_void deinit_dev_excp_handler(oal_void)
{
    if (dev_excp_handler_data.nlsk != OAL_PTR_NULL) {
        oal_netlink_kernel_release(dev_excp_handler_data.nlsk);
        dev_excp_handler_data.usepid = 0;
    }

    if (dev_excp_handler_data.data != OAL_PTR_NULL) {
        kfree(dev_excp_handler_data.data);
    }

    OAL_IO_PRINT("DFR: deinit ok.\n");

    return;
}
#else
oal_int32 dev_netlink_create(void)
{
    return OAL_SUCC;
}

oal_int32 dev_netlink_send (const oal_uint8 *data, oal_long data_len)
{
    return OAL_SUCC;
}
oal_int32 init_dev_excp_handler(oal_void)
{
    return OAL_SUCC;
}
oal_void deinit_dev_excp_handler(oal_void)
{
    return;
}

#endif


/*lint -e19*/
oal_module_symbol(oal_netbuf_is_dhcp_port);
oal_module_symbol(oal_netbuf_is_dhcp6);
oal_module_symbol(oal_netbuf_is_nd);
oal_module_symbol(oal_netbuf_is_tcp_ack);
oal_module_symbol(oal_netbuf_is_icmp);

#ifdef _PRE_WLAN_FEATURE_FLOWCTL
oal_module_symbol(oal_netbuf_get_txtid);
#endif


oal_module_symbol(init_dev_excp_handler);
oal_module_symbol(deinit_dev_excp_handler);


/*lint +e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

