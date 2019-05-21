/**
 ******************************************************************************
 * @file       param.h
 * @brief      API include file of param.h.
 * @details    This file including all API functions's declare of param.h.
 * @copyright  
 *
 ******************************************************************************
 */
#ifndef PARAM_H_
#define PARAM_H_ 

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include "listLib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/* None */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef enum
{
    E_TYPE_APP = 0,
    E_TYPE_TERMINAL,
} conn_type_e;

typedef struct
{
    int support_front;                      /**< �Ƿ�����ǰ��ͨ�� */
    char front_ip[64];                      /**< ǰ��IP��ַ */
    unsigned short int front_tcp_port;      /**< ǰ��TCP�˿� */
    int front_timeout;                      /**< ǰ��ͨ�ų�ʱ(��λ: us) */

    unsigned short int app_tcp_port;        /**< ��̨TCP�˿� */
    unsigned short int terminal_tcp_port;   /**< �ն�TCP�˿� */
    unsigned short int terminal_udp_port;   /**< �ն�UDP�˿� */
    unsigned short int telnet_port;         /**< telnet�˿� */

    int timeout;                            /**< TCP���ӳ�ʱʱ��(��λ:����) */

    int ptcl_type;                          /**< Э������ */
    int support_compress;                   /**< ֧�����������㷨 */
    int support_cas;                        /**< ֧���������� */
    int support_cas_link;                   /**< ֧�������ն˵�½������ */
    int max_frame_bytes;                    /**< ����������ֽ��� */
    int support_comm_terminal;              /**< �����ն��ظ����� */
    int is_cfep_reply_heart;                /**< ����ǰ�û�ά���������� */

    int terminal_heartbeat_cycle_multiple;  /**< �ն�������ʱ����0��ʾ������� */
    int terminal_disconnect_mode;           /**< �ն˶Ͽ�����ģʽ: 0�ر�TCP; 1�ر�ת�� */

    /**
     * Ĭ�ϵ��Լ���
     * 0 : ��ӡ������Ϣ
     * 1 : ��ӡ������Ϣ + ������־
     * 2 : ��ӡ������Ϣ + ������־ + ���Դ�ӡ��Ϣ
     */
    int default_debug_level;
} pcfg_t;

/** socket list */
typedef struct
{
    struct ListNode node;
    int listen;
    conn_type_e type;   //���
} slist_t;

/** ���в��� */
typedef struct
{
    pcfg_t pcfg;
    slist_t app_tcp;
    slist_t terminal_tcp;
    slist_t terminal_udp;

    int front_socket;
} prun_t;

/** ���ӽṹ */
typedef struct _connect_t
{
    struct ListNode node;
    int socket;             /**< socket���Ӿ�� */
    //void *saddr;            /**< UDP����ʱʹ��struct sockaddr_in */
    time_t connect_time;    /**< ����ʱ�� */
    time_t last_time;       /**< ��һ��ͨ��ʱ�� */
    time_t last_heartbeat;  /**< ��һ������ʱ�� */
    int heartbeat_cycle;    /**< ��������(��λ:s) */
    chkfrm_t chkfrm;
    int is_closing;         /**< ׼���ر� */
    addr_t u;               /**< ��ַ */
    struct ListNode cas;    /**< �����ն��б� */
//    struct _connect_t *precon; //��ʱ����
} connect_t;

typedef struct
{
    struct ListNode node;
    addr_t u;               /**< ��ַ */
} cas_addr_t;

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */


#endif /* PARAM_H_ */
/*-----------------------------End of param.h--------------------------------*/
