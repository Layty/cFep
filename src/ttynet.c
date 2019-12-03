/**
 ******************************************************************************
 * @file      ttynet.c
 * @brief     C Source file of ttynet.c.
 * @details   This file including all API functions's implement of ttynet.c.
 * @copyright Copyrigth(C), 2008-2012,Sanxing Electric Co.,Ltd.
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "maths.h"
#include "socket.h"
#include "taskLib.h"
#include "listLib.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef struct
{
    struct ListNode node;
    int socket;             //socket���Ӿ��
    time_t connect_time;    //����ʱ��
    time_t last_time;       //��һ��ͨ��ʱ��
    int flag;               //��־: �Ƿ���Ҫ�����豸�б�
} client_t;

typedef struct
{
    struct ListNode node;
    client_t *pclient;
    int socket;             //socket���Ӿ��
    time_t connect_time;    //����ʱ��
    time_t last_time;       //��һ��ͨ��ʱ��
    time_t heart_time;      //��һ������ʱ��

    uint8_t type;           //�豸����: 0(�ն�) other
    uint8_t addr[16];       //���ַ
    uint8_t mac[6];         //Mac��ַ
    uint8_t ipaddr[4];      //IPv4��ַ
    uint8_t err_cnt;        //��������
    uint32_t err_val;       //���ϱ�־: �ܹ�32λ,ÿλ��Ӧ1������,��1��ʾ����
    uint32_t run_t;         //�ϵ�������ʱ��(��λs)
    uint8_t ver[32];        //����汾
    uint8_t date[32];       //�����������
} ttynet_dev_t;

typedef struct
{
    int listen;
    uint16_t telnet_port;
    struct ListNode dev_list;
    struct ListNode client_list;
} ttynet_run_t;

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifndef SXUDP_TTYUDP_PORT
# define SXUDP_TTYUDP_PORT      (19000)
#endif

#ifndef SXUDP_FACTORY_PORT
# define SXUDP_FACTORY_PORT     (19001)
#endif

#define SXUDP_CLIENT_PORT_MIN   (19000)
#define SXUDP_CLIENT_PORT_MAX   (20000)

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
extern unsigned char the_rbuf[2048]; //ȫ�ֻ���

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
static ttynet_run_t the_ttynet;

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
 * @brief   ɾ��һ�����Ӷ���
 * @param[in]  *pc : ��ɾ�������Ӷ���
 *
 * @return  None
 ******************************************************************************
 */
static void
delete_client(client_t *pc)
{
    socket_close(pc->socket);
    ListDelNode(&pc->node);
    free(pc);
}

/**
 ******************************************************************************
 * @brief   ���telnet�˿��Ƿ��������ͻ���
 * @param[in]  *prun : ttynet���в���
 *
 * @return  None
 ******************************************************************************
 */
static void
ttynet_accept_client(ttynet_run_t *prun)
{
    int conn_fd;

    while ((conn_fd = socket_accept(prun->listen)) != -1)
    {
        //�����������˿��µ�����
        client_t *pc = malloc(sizeof(client_t));
        if (pc)
        {
            memset(pc, 0x00, sizeof(client_t));
            pc->socket = conn_fd;
            pc->connect_time = time(NULL) - 1;
            pc->last_time = pc->connect_time;
            ListAddTail(&pc->node, &prun->client_list); //��������
        }
        else
        {
            log_print(L_ERROR, "out of memory!\n");
            socket_close(conn_fd);
        }
    }
}

/**
 ******************************************************************************
 * @brief   ����ͻ�������
 * @param[in]  *prun : ttynet���в���
 *
 * @retval     None
 ******************************************************************************
 */
static void
ttynet_recv_client_cmd(ttynet_run_t *prun)
{
    int len;
    client_t *pc;
    struct ListNode *ptmp;
    struct ListNode *piter;

    LIST_FOR_EACH_SAFE(piter, ptmp, &prun->client_list)
    {
        pc = MemToObj(piter, client_t, node);

        while ((len = socket_recv(pc->socket, the_rbuf, sizeof(the_rbuf))) > 0)
        {
            //todo: �������
        }
        if (len < 0)
        {
            log_print(L_DEBUG, "telnet client ��ȡʧ��!\n");
            piter = piter->pPrevNode;
            delete_client(pc);
        }
    }
}

/**
 ******************************************************************************
 * @brief   �����豸����
 * @param[in]  *prun : ttynet���в���
 *
 * @retval     None
 ******************************************************************************
 */
static void
ttynet_recv_device_list(ttynet_run_t *prun)
{

}

/**
 ******************************************************************************
 * @brief   ttynet��ʼ��
 * @param[in]  port : telnet�����˿�
 *
 * @retval  OK    : �ɹ�
 * @retval  ERROR : ʧ��
 ******************************************************************************
 */
status_t
ttynet_init(uint16_t port)
{
    memset(&the_ttynet, 0x00, sizeof(the_ttynet));
    the_ttynet.telnet_port = port;
    InitListHead(&the_ttynet.client_list);
    InitListHead(&the_ttynet.dev_list);

    the_ttynet.listen = socket_listen(the_ttynet.telnet_port, E_SOCKET_TCP);
    if (the_ttynet.listen == -1)
    {
        fprintf(stderr, "����Telnet��¼�˿�:%dʧ��!\n", the_ttynet.telnet_port);
        return ERROR;
    }

    return OK;
}

/**
 ******************************************************************************
 * @brief   ttynet��ʱ������
 * @return  None
 ******************************************************************************
 */
void
ttynet_do(void)
{
    ttynet_accept_client(&the_ttynet); //���ܿͻ���

    ttynet_recv_client_cmd(&the_ttynet); //����ͻ�������

    ttynet_recv_device_list(&the_ttynet);
}

/**
 ******************************************************************************
 * @brief   ttynetˢ���豸�б�
 * @return  None
 ******************************************************************************
 */
void
ttynet_refresh_dev(void)
{

}

/**
 ******************************************************************************
 * @brief   ttynet�����Ϣ
 * @return  None
 ******************************************************************************
 */
void
ttynet_print_info(void)
{

}

/*--------------------------------ttynet.c-----------------------------------*/
