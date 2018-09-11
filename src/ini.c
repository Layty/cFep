/**
 ******************************************************************************
 * @file      ini.c
 * @brief     C Source file of ini.c.
 * @details   This file including all API functions's 
 *            implement of ini.c.	
 *
 * @copyright
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "iniparser.h"
#include "ini.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifndef DEFAULT_INI_FILE
# define DEFAULT_INI_FILE   "./cFep.ini"
#endif

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   ����Ĭ�������ļ�
 * @return  None
 ******************************************************************************
 */
static void
create_example_ini_file(void)
{
    FILE * ini;

    ini = fopen(DEFAULT_INI_FILE, "w");
    fprintf(ini,
            "#cFep����By LiuNing\n"
            "[cfg]\n"
            "#��̨TCP�˿�\n"
            "app_tcp_port           = 20014\n\n"
            "#�ն�TCP�˿�\n"
            "terminal_tcp_port      = 20013\n\n"
            "#�ն�UDP�˿�\n"
            "terminal_udp_port      = 20013\n\n"
            "#TCP���ӳ�ʱʱ��(��λ:����)\n"
            "timeout                = 30\n\n"
            "#Э������,0:���� 1:���� 2:�㶫���㽭��Լ 3:���ֹ�Լ 4:62056-47 5:698����\n"
            "ptcl_type              = 0\n\n"
            "#�Ƿ�֧�ּ���(����ʹ��)\n"
            "support_compress       = 0\n\n"
            "#�Ƿ���(����ʹ��)\n"
            "support_cas            = 0\n\n"
            "#�Ƿ����ն˵�½������(����ʹ��)\n"
            "support_cas_link       = 0\n\n"
            "#����������ֽ���(ͨ��1k~2k)\n"
            "max_frame_bytes        = 2048\n\n"
            "#�Ƿ�֧���ն��ظ���½(Y/N)\n"
            "support_comm_terminal  = N\n\n"
            "#�Ƿ�֧��ǰ�û�ά������(Y/N)\n"
            "is_cfep_reply_heart    = Y\n\n"
            "#Ĭ�ϵ��Լ���\n"
            "#  0 : ��ӡ��Ҫ��Ϣ\n"
            "#  1 : ��ӡ��Ҫ��Ϣ + ������־\n"
            "#  2 : ��ӡ��Ҫ��Ϣ + ������־ + ������Ϣ\n"
            "default_debug_level    = 0\n"
            );
    fclose(ini);
}

/**
 ******************************************************************************
 * @brief   �������ļ��л�ȡ�ļ��ϲ���Ϣ
 * @param[out] *pinfo   : ����info
 *
 * @retval     -1 ʧ��
 * @retval      0 �ɹ�
 ******************************************************************************
 */
int
ini_get_info(pcfg_t *pinfo)
{
    dictionary  *   ini ;
    int vtmp;

    memset(pinfo, 0x00, sizeof(*pinfo));

    ini = iniparser_load(DEFAULT_INI_FILE);
    if (NULL == ini)
    {
        create_example_ini_file();
        ini = iniparser_load(DEFAULT_INI_FILE);
        if (ini == NULL)
        {
            return -1;
        }
    }

    iniparser_dump(ini, NULL);//stderr

    //��̨TCP�˿�
    vtmp = iniparser_getint(ini, "cfg:app_tcp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "��̨TCP�˿�[%d]�Ƿ�!\n", vtmp);
        pinfo->app_tcp_port = 20014;
    }
    else
    {
        pinfo->app_tcp_port = vtmp;
    }

    //�ն�TCP�˿�
    vtmp = iniparser_getint(ini, "cfg:terminal_tcp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "�ն�TCP�˿�[%d]�Ƿ�!\n", vtmp);
        pinfo->terminal_tcp_port = 20013;
    }
    else
    {
        pinfo->terminal_tcp_port = vtmp;
    }

    //�ն�UDP�˿�
    vtmp = iniparser_getint(ini, "cfg:terminal_udp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "�ն�UDP�˿�[%d]�Ƿ�!\n", vtmp);
        pinfo->terminal_udp_port = 20013;
    }
    else
    {
        pinfo->terminal_udp_port = vtmp;
    }

    //TCP���ӳ�ʱʱ��(��λ:����)
    vtmp = iniparser_getint(ini, "cfg:timeout", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1440))
    {
        fprintf(stderr, "TCP���ӳ�ʱʱ��[%d]�Ƿ�!\n", vtmp);
        pinfo->timeout = 30;    //default 30 min
    }
    else
    {
        pinfo->timeout = vtmp;
    }

    //Э������
    vtmp = iniparser_getint(ini, "cfg:ptcl_type", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 5))
    {
        fprintf(stderr, "Э������[%d]�Ƿ�!����Ĭ��ֵ0\n", vtmp);
        pinfo->ptcl_type = 0;
    }
    else
    {
        pinfo->ptcl_type = vtmp;
    }

    //�Ƿ����
    vtmp = iniparser_getint(ini, "cfg:support_compress", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "�Ƿ����[%d]�Ƿ�!����Ĭ��ֵ0\n", vtmp);
        pinfo->support_compress = 0;
    }
    else
    {
        pinfo->support_compress = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //�Ƿ�֧�ּ���
    vtmp = iniparser_getint(ini, "cfg:support_cas", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "�Ƿ����[%d]�Ƿ�!����Ĭ��ֵ0\n", vtmp);
        pinfo->support_cas = 0;
    }
    else
    {
        pinfo->support_cas = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //�Ƿ�֧�ּ����ն˵�½������
    vtmp = iniparser_getint(ini, "cfg:support_cas_link", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "�Ƿ����[%d]�Ƿ�!����Ĭ��ֵ0\n", vtmp);
        pinfo->support_cas_link = 0;
    }
    else
    {
        pinfo->support_cas_link = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //����������ֽ���
    vtmp = iniparser_getint(ini, "cfg:max_frame_bytes", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 512) || (vtmp > 8192))
    {
        fprintf(stderr, "����������ֽ���[%d]�Ƿ�!\n", vtmp);
        pinfo->max_frame_bytes = 2048;
    }
    else
    {
        pinfo->max_frame_bytes = vtmp;
    }

    //�����ն��ظ�����
    vtmp = iniparser_getboolean(ini, "cfg:support_comm_terminal", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "�����ն��ظ�����[%d]�Ƿ�!\n", vtmp);
        pinfo->support_comm_terminal = 1;
    }
    else
    {
        pinfo->support_comm_terminal = vtmp;
    }

    //����ǰ�û�ά����������
    vtmp = iniparser_getboolean(ini, "cfg:is_cfep_reply_heart", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "����ǰ�û�ά����������[%d]�Ƿ�!\n", vtmp);
        pinfo->is_cfep_reply_heart = 1;
    }
    else
    {
        pinfo->is_cfep_reply_heart = vtmp;
    }

    //Ĭ�ϵ��Լ���
    vtmp = iniparser_getint(ini, "cfg:default_debug_level", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 2))
    {
        fprintf(stderr, "Ĭ�ϵ��Լ���[%d]�Ƿ�!\n", vtmp);
        pinfo->default_debug_level = 0;
    }
    else
    {
        pinfo->default_debug_level = vtmp;
    }

    iniparser_freedict(ini);

    return 0;
}

/*---------------------------------ini.c-------------------------------------*/
