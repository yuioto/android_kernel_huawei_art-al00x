/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*****************************************************************************
  1 ͷ�ļ�����
*****************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include "osl_sem.h"
#include "osl_spinlock.h"
#include "bsp_dump.h"
#include "bsp_trace.h"
#include "bsp_print.h"
#include "bsp_socp.h"
#include "bsp_ipc.h"
#include "bsp_hardtimer.h"
#include <bsp_modem_log.h>
#include <securec.h>
#include "bsp_om_log.h"

#define OLD_MATCH	((u32)1)
#define NEW_MATCH       ((u32)0)
/*new print start*/
#define THIS_MODU mod_print
u32 g_print_close = 0;
bsp_syslevel_ctrl g_print_sys_level = {BSP_P_ERR,BSP_P_INFO};

bsp_print_tag g_print_tag[MODU_MAX] ={
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
    {BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},{BSP_P_INFO},
	};
/*new print end*/

bsp_log_swt_cfg_s  g_mod_peint_level_info[BSP_MODU_MAX]    =
{
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR},
    {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}, {BSP_LOG_LEVEL_ERROR}
};

/*****************************************************************************
* �� �� ��  : bsp_log_module_cfg_get
*
* ��������  : ��ѯģ�鵱ǰ���õĴ�ӡ����
*
* �������  : mod_id:����ѯģ��ID
*
* �������  : ��
*
* �� �� ֵ  : ��ӡ����
*****************************************************************************/
u32 bsp_log_module_cfg_get(bsp_module_e mod_id)
{
    if(mod_id >= BSP_MODU_MAX )
    {
        return BSP_ERR_LOG_INVALID_MODULE;
    }

    return g_mod_peint_level_info[mod_id].print_level;
}

/*****************************************************************************
* �� �� ��  : bsp_mod_level_set
*
* ��������  : ���õ���ģ��Ĵ�ӡ����
*
* �������  : mod_id:ģ��ID
*             print_level: ��ӡ����
* �������  : ��
*
* �� �� ֵ  : BSP_OK ���óɹ������� ����ʧ��
*****************************************************************************/
u32 bsp_mod_level_set(bsp_module_e  mod_id ,u32 print_level)
{
    if(mod_id >= BSP_MODU_MAX )
    {
        return BSP_ERR_LOG_INVALID_MODULE;
    }

    if(print_level >BSP_LOG_LEVEL_MAX)
    {
        return BSP_ERR_LOG_INVALID_LEVEL;
    }

    g_mod_peint_level_info[mod_id].print_level = print_level;

    return BSP_OK;
}

/*****************************************************************************
* �� �� ��  : bsp_log_level_set
*
* ��������  : ��������ģ��Ĵ�ӡ����
*
* �������  : print_level: ��ӡ����
*
* �������  : ��
*
* �� �� ֵ  : BSP_OK ���óɹ������� ����ʧ��
*****************************************************************************/
u32 bsp_log_level_set(bsp_log_level_e log_level)
{
    u32 mod_id = 0;

    if(log_level > BSP_LOG_LEVEL_MAX)
    {
        return BSP_ERR_LOG_INVALID_LEVEL;
    }

    for(mod_id = 0; mod_id < BSP_MODU_MAX; mod_id++)
    {
        g_mod_peint_level_info[mod_id].print_level = log_level;
    }

    return BSP_OK;
}

/*****************************************************************************
* �� �� ��  : bsp_log_level_reset
*
* ��������  : ������ģ��Ĵ�ӡ��������ΪĬ��ֵ
*
* �������  : ��
*
* �������  : ��
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_log_level_reset(void)
{
    u32 i;

    for(i = 0; i < BSP_MODU_MAX;i++)
    {
        g_mod_peint_level_info[i].print_level= BSP_LOG_LEVEL_ERROR;
    }

}

/*****************************************************************************
* �� �� ��  : bsp_trace
*
* ��������  : �����ӡ�������ӿ�
*
* �������  :  mod_id: ���ģ��
*              log_level: ��ӡ����
*              fmt :��ӡ�������
*
* �������  : ��
*
* �� �� ֵ  : ��
*****************************************************************************/
void bsp_trace(bsp_log_level_e log_level,bsp_module_e mod_id,char *fmt,...)
{
    char    bsp_print_buffer[BSP_PRINT_BUF_LEN]={'\0',};
    va_list arglist;
    u32 modid_u32 = (u32)mod_id,level_u32=(u32)log_level;
    char *print_p = bsp_print_buffer;
    /*��ӡ�������*/
    if(mod_id >= BSP_MODU_MAX )
    {
        return ;
    }

    if(g_mod_peint_level_info[mod_id].print_level > log_level )
    {
        return ;
    }
    /*lint -save -e530*/
    va_start(arglist, fmt);
    /*lint -restore +e530*/
    vsnprintf(bsp_print_buffer, BSP_PRINT_BUF_LEN,fmt, arglist);/* unsafe_function_ignore: vsnprintf */ /* [false alarm]:����Fortify���� */
    va_end(arglist);

    bsp_print_buffer[BSP_PRINT_BUF_LEN - 1] = '\0';

    printk(KERN_ERR"%s", bsp_print_buffer);

    if(g_bsp_print_hook)
    {
        g_bsp_print_hook(modid_u32,level_u32,OLD_MATCH,print_p);
    }

    return ;
}
EXPORT_SYMBOL_GPL(bsp_trace);
/*new print start*/
/*****************************************************************************
* �� �� ��  : bsp_print_control
*
* ��������  : ��ѯģ�鵱ǰ���õĴ�ӡ����
*
* �������  : sel:sel = 0 close print; sel = 1 open print
*
* �������  : ��
*
* �� �� ֵ  : ��ӡ����
*****************************************************************************/
void bsp_print_control(u32 sel)
{
    g_print_close = sel;
    return;
}
/*****************************************************************************
* �� �� ��  : bsp_print_control
*
* ��������  : ��ȡ����״̬
*
* �������  : NA
*
* �������  : ��
*
* �� �� ֵ  : ��ӡ����
*****************************************************************************/
u32 bsp_get_print_status(void)
{
    return g_print_close;
}
/*****************************************************************************
* �� �� ��  : logs
*
* ��������  : ϵͳ��������
*
* �������  : u32 console������̨��ӡ����u32 logbuf��default for acore
*
* �������  : BSP_OK/BSP_ERROR
*
* �� �� ֵ  :
*****************************************************************************/
s32 logs( u32 console,u32 logbuf)
{
    if(console >= BSP_LEVEL_SUM || logbuf >= BSP_LEVEL_SUM)
        return BSP_ERROR;
    g_print_sys_level.logbuf_level = logbuf;
	g_print_sys_level.con_level = console;
    return BSP_OK;
}
/*****************************************************************************
* �� �� ��  : set_all_module
*
* ��������  : set all modules' level
*
* �������  : u32 level: mod_level
*
* �������  : NA
*
* �� �� ֵ  :
*****************************************************************************/
void set_all_module(u32 level)
{
	int i = 0;
    if(level >= BSP_LEVEL_SUM)
        return;
	for(i = 0; i < MODU_MAX; i++)
		g_print_tag[i].modlevel= level;
    
	return ;
}
/*****************************************************************************
* �� �� ��	: logm
*
* ��������	: set module's level according to modid
*
* �������	: u32 modid: module's id, u32 level: mod_level
*
* �������	: BSP_OK/BSP_ERROR
*
* �� �� ֵ	: �ɹ�/ʧ��
*****************************************************************************/
s32 logm(u32 modid, u32 level)
{
	if(MODU_MAX <= modid){
		bsp_err("modid is error!\n");
		return BSP_ERROR;
		}
	if(level >= BSP_LEVEL_SUM){
		bsp_err("level can't over 5!\n");
		return BSP_ERROR;
		}
	if( mod_all == modid){
		set_all_module(level);
		}
	else
		g_print_tag[modid].modlevel= level;
	return BSP_OK;


}
/*****************************************************************************
* �� �� ��	: logc
*
* ��������	: inquire module's level according to modid
*
* �������	: u32 modid: module's id
*
* �������	: NA
*
* �� �� ֵ	:
*****************************************************************************/
void logc(u32 modid)
{
	if(MODU_MAX <= modid){
		bsp_err("modid is error!\n");
		return ;
		}
	bsp_err("con_level:%d logbuf_level:%d mod_level:%d\n\n",g_print_sys_level.con_level,g_print_sys_level.logbuf_level,g_print_tag[modid].modlevel);
	return ;
}
/*****************************************************************************
* �� �� ��	: bsp_print
*
* ��������	: print
*
* �������	: u32 modid: module's id, BSP_LOG_LEVEL level: print level, char *fmt: string
*
* �������	:
*
* �� �� ֵ	:
*****************************************************************************/
void bsp_print(module_tag modid, BSP_LOG_LEVEL level, char *fmt, ...)
{

    char print_buffer[BSP_PRINT_BUF_LEN]={'\0',};
    va_list arglist;
    u32 modid_u32 = (u32)modid,level_u32=(u32)level;
    char *print_p = print_buffer;
    if(modid >= MODU_MAX || BSP_PRINT_OFF == level)
    {
        return ;
    }

    if(g_print_tag[modid].modlevel < level)  //���뼶�����ģ��Ĭ�ϼ��𣬷���
    {
        return;
    }

	va_start(arglist, fmt);
    vsnprintf(print_buffer, (BSP_PRINT_BUF_LEN-1), fmt, arglist);/* unsafe_function_ignore: vsnprintf */
    va_end(arglist);
	print_buffer[BSP_PRINT_BUF_LEN - 1] = '\0';

	if(g_print_sys_level.con_level >= level)
		(void)printk(KERN_ERR"%s", print_buffer);

	if(g_print_sys_level.logbuf_level < level)
		return ;
	
    if(g_bsp_print_hook)
    {
        g_bsp_print_hook(modid_u32,level_u32,NEW_MATCH,print_p);
    }

}
EXPORT_SYMBOL_GPL(bsp_print);
EXPORT_SYMBOL_GPL(logs);
EXPORT_SYMBOL_GPL(logm);
EXPORT_SYMBOL_GPL(logc);
/*new print end*/

#ifdef ENABLE_BUILD_OM

/*debug �ӿ�*/
void bsp_log_show(void)
{
    bsp_err("trace level = %d\n",g_mod_peint_level_info[0].print_level);
}

void log_buff_info(void)
{
    log_mem_stru * log = NULL;

    log    = (log_mem_stru *)bsp_dump_get_field_addr(DUMP_CP_DMESG);
    if(log != NULL)
    {
        bsp_err("BUFFER MAGIC    : 0x%x\n", log->log_info.magic);
        bsp_err("READ POINTER    : 0x%x\n", log->log_info.read);
        bsp_err("WRITE POINTER   : 0x%x\n", log->log_info.write);
        bsp_err("BUFFER LENGTH   : 0x%x\n", log->log_info.size);
        bsp_err("APP STATE       : 0x%x\n", log->log_info.app_is_active);
    }
}

#ifndef BSP_CONFIG_PHONE_TYPE
static inline u32 om_log_writable_size_get(u32 write, u32 read, u32 ring_buffer_size)
{
	return (read > write)? (read - write): (ring_buffer_size - write + read);
}

/*****************************************************************************
* �� �� ��  : do_read_data_to_user
*
* ��������  : �����������û�̬
*
* �������  : char * src, ����Դbuffer
*             char * dst������Ŀ��buffer
*             u32 len, ������ȡ�����ݳ���
*
* �������  : ��
*
* �� �� ֵ  : ʵ�ʶ�ȡ�����ݳ���
*****************************************************************************/
int do_read_data_to_user(char * src, char * dst, u32 len)
{
    u32 ret;

    /* ����δ�ܿ��������ݳ��� */
    ret = (u32)copy_to_user(dst, src, len);

    return (int)(len - ret);
}

int om_log_open(struct log_usr_info *usr_info)
{
    DUMP_SAVE_MOD_ENUM mod_id;
    u8 * addr = NULL;
	log_mem_stru *dump_mem = NULL;

    if(!strncmp(usr_info->dev_name, CCORE_LOG_DEV_NAME, sizeof(CCORE_LOG_DEV_NAME)))
    {
        mod_id = DUMP_CP_DMESG;
    }
    else if(!strncmp(usr_info->dev_name, MCORE_LOG_DEV_NAME, sizeof(MCORE_LOG_DEV_NAME)))
    {
        mod_id = DUMP_M3_LOG;
    }
    else
    {
        /* invalid mod id */
        mod_id = DUMP_MODEMAP_FIELD_END;
    }

    addr = bsp_dump_get_field_addr(mod_id);
    if(addr == NULL)
    {
		bsp_info("%s get memory fail\n", __func__);
        return -1;
    }

	dump_mem            = (log_mem_stru *)addr;
    usr_info->mem       = &dump_mem->log_info;
	usr_info->ring_buf  = (char *)(dump_mem) + sizeof(log_mem_stru);
	usr_info->mem_is_ok = 1;
	bsp_info("%s entry\n", __func__);

	return 0;
}

int om_log_read(struct log_usr_info *usr_info, char *buf, u32 count)
{
	s32 ret     = 0;
	u32 msg_len = 0;
	u32 write_p = usr_info->mem->write;
	u32 read_p  = usr_info->mem->read;

    if(om_log_writable_size_get(write_p, read_p, usr_info->mem->size) <= LOG_BUFFER_FULL_THRESHOLD)
    {
        usr_info->mem->read = usr_info->mem->write;
        msg_len = (u32)strlen(LOG_DROPPED_MESSAGE);
        count   = min(msg_len, count);
        ret     = do_read_data_to_user(LOG_DROPPED_MESSAGE, buf, count);
		bsp_info("%s entry\n", __func__);
	}
	return ret;
}

void om_log_dump_handle(void)
{
    bsp_modem_log_fwrite_trigger(&g_om_log.ccore_log_info);
    bsp_modem_log_fwrite_trigger(&g_om_log.mcore_log_info);
}

static int modem_om_log_init(void)
{
	s32 ret = 0;
    if(DUMP_MBB == dump_get_product_type())
    {
		memset_s((void *)&g_om_log, sizeof(g_om_log), 0, sizeof(g_om_log));

        /* ccore device */
		g_om_log.ccore_log_info.dev_name = CCORE_LOG_DEV_NAME;
		g_om_log.ccore_log_info.mem      = NULL;
		g_om_log.ccore_log_info.read_func = (USR_READ_FUNC)om_log_read;
		g_om_log.ccore_log_info.open_func = (USR_OPEN_FUNC)om_log_open;
		ret = bsp_modem_log_register(&g_om_log.ccore_log_info);
        if(ret)
        {
            bsp_err("log_ccore register fail\n");
            return BSP_ERROR;
        }

        /* mcore device */
		g_om_log.mcore_log_info.dev_name = MCORE_LOG_DEV_NAME;
		g_om_log.mcore_log_info.mem      = NULL;
		g_om_log.mcore_log_info.read_func = (USR_READ_FUNC)om_log_read;
		g_om_log.mcore_log_info.open_func = (USR_OPEN_FUNC)om_log_open;
		ret = bsp_modem_log_register(&g_om_log.mcore_log_info);
        if(ret)
        {
            bsp_err("log_mcore register fail\n");
            return BSP_ERROR;
        }

        ret= bsp_dump_register_hook("KERNEL_LOG", om_log_dump_handle);
        if(BSP_ERROR == ret)
        {
            bsp_err("register to dump failed!\n");
            return BSP_ERROR;
        }

        bsp_err("[init] ok\n");
    }

	return 0;
}

module_init(modem_om_log_init);
#endif
#endif

EXPORT_SYMBOL(bsp_log_module_cfg_get);
EXPORT_SYMBOL(bsp_mod_level_set);
EXPORT_SYMBOL(bsp_log_level_set);
EXPORT_SYMBOL(bsp_log_level_reset);
EXPORT_SYMBOL(bsp_log_show);
EXPORT_SYMBOL(log_buff_info);


