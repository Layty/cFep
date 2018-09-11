/**
 ******************************************************************************
 * @file      ptcl_gw.c
 * @brief     C Source file of ptcl_gw.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_gw.c.	
 *
 * @copyright 
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lib/zip/CCEMan.h"
#include "param.h"
#include "lib.h"
#include "ptcl.h"
#include "log.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
//����ͷ�ṹ��
typedef struct FrameNWHeaderStruct
{
    unsigned char frameStart;       //0x68
    unsigned short lenArea;         //���ȣ������򡢵�ַ����·�û����ݳ���������
    unsigned short lenArea_;        //�ظ��ĳ�����
    unsigned char dataAreaStart;    //0x68
    union
    {
        struct
        {
            unsigned char FuncCode : 4 ,    //D0--D3����������
                          FCV      : 1 ,    //D4֡������Чλ
                          FCB_ACD  : 1 ,    //D5֡����λ ���� �������λ
                          PRM      : 1 ,    //D6������־λ
                          DIR      : 1 ;    //D7���䷽��λ
        };
        unsigned char ctrlCodeArea;
    };
    union
    {
        unsigned char deviceAddr[6];
//        unsigned long long logicaddr0 : 24;//�ն��߼���ַ0
//        unsigned long long logicaddr1 : 24;//�ն��߼���ַ1
        struct
        {
            unsigned char canton[3];
            unsigned char addr[3];
        };
    };
    unsigned char hostID;   //MSA��վ��ַ
    unsigned char AFN;
    union
    {
        struct
        {
            unsigned char frameSeq   :     4 ,//D0--D3֡���
                          CON        :     1 ,//D4����ȷ�ϱ�־λ
                          FIN        :     1 ,//D5����֡��־
                          FIR        :     1 ,//D6��֡��־
                          TPV        :     1 ;//D7֡ʱ���ǩ��Чλ
        };
        unsigned char seqArea;
    };
} nw_header_t;

//ʱ���ʶ�ṹ
struct TPStruct
{
    unsigned char PFC;
    unsigned char sec;
    unsigned char min;
    unsigned char hour;
    unsigned char day;
    unsigned char timeOut;
};
#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< ���ĳ�ʱʱ��Ĭ��10��  */
#define FRAME_NO_DATA_LEN               14   /**< ���������������ݱ�ʾ�ı���ͷ���� */

/* states for scanning incomming bytes from a bytestream */
#define NW_FRAME_STATES_NULL               0    /**< no synchronisation */
#define NW_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define NW_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define NW_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define NW_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define NW_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define NW_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define NW_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define NW_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define NW_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define NW_FRAME_STATES_CS                 10   /**< wait for the CS */
#define NW_FRAME_STATES_END                11   /**< wait for the 16H */
#define NW_FRAME_STATES_COMPLETE           12   /**< complete frame */

#define NW_COMPRESS_OPER                   13   /**< ���ܷ�ʽ */
#define NW_COMPRESS_LEN1                   14   /**< len */
#define NW_COMPRESS_LEN2                   15   /**< len */
#define NW_COMPRESS_DATA                   16   /**< data */
#define NW_COMPRESS_END                    17   /**< wait for 77H */

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
 * @brief   �������ļ���ʼ��
 * @param[in]  *pchk         : ���ļ�����
 * @param[in]  *pfn_frame_in : ���յ��Ϸ����ĺ�ִ�еĻص�����
 *
 * @return  None
 ******************************************************************************
 */
static void
nw_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = NW_FRAME_STATES_NULL;
    pchk->update_time = time(NULL);
    pchk->overtime = CHKFRAME_TIMEOUT;
    pchk->pfn_frame_in = pfn_frame_in;
}

/**
 ******************************************************************************
 * @brief   �������ļ��
 * @param[in]  *pc      : ���Ӷ���(fixme : �Ƿ���Ҫÿ�δ���?)
 * @param[in]  *pchk    : ���ļ�����
 * @param[in]  *rxBuf   : ��������
 * @param[in]  rxLen    : �������ݳ���
 *
 * @return  None
 ******************************************************************************
 */
static void
nw_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* ����Ѿ���ɵ��������¿�ʼ */
    if (pchk->frame_state == NW_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = NW_FRAME_STATES_NULL;
    }

    /* �����ʱ�����¿�ʼ */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = NW_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case NW_FRAME_STATES_NULL:

                if ((*rxBuf == 0x68) || (*rxBuf == 0x88))
                {
                    if (!pchk->pbuf)
                    {
                        pchk->pbuf = malloc(the_max_frame_bytes);
                        if (!pchk->pbuf)
                        {
                            return; //
                        }
                    }
                    pchk->pbuf_pos = 0;
                    if (*rxBuf == 0x88)
                    {
                        pchk->frame_state = NW_COMPRESS_OPER; //����
                    }
                    else
                    {
                        pchk->frame_state = NW_FRAME_STATES_LEN1_1;
                    }
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case NW_FRAME_STATES_LEN1_1: /* ���L1�ĵ��ֽ� */
                pchk->frame_state = NW_FRAME_STATES_LEN1_2;/* Ϊ������վ������Լ���� */
                pchk->dlen = *rxBuf;
                break;

            case NW_FRAME_STATES_LEN1_2: /* ���L1�ĸ��ֽ� */
                pchk->dlen += ((unsigned int)*rxBuf << 8u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = NW_FRAME_STATES_LEN2_1;
                }
                break;

            case NW_FRAME_STATES_LEN2_1: /*���L2�ĵ��ֽ�*/
                pchk->frame_state = NW_FRAME_STATES_LEN2_2;
                pchk->cfm_len = *rxBuf;
                break;

            case NW_FRAME_STATES_LEN2_2: /*���L2�ĸ��ֽ�*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 8u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = NW_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = NW_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = NW_FRAME_STATES_A3;/* ���ܼ�ⷽ����Ϊ���������б��� */
                break;

            case NW_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 13)
                {
                    pchk->frame_state = NW_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case NW_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = NW_FRAME_STATES_CS;
                }
                break;

            case NW_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = NW_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = NW_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_COMPRESS_OPER:
                if (*rxBuf == 0x01)
                {
                    pchk->frame_state = NW_COMPRESS_LEN1;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_COMPRESS_LEN1:
                pchk->frame_state = NW_COMPRESS_LEN2;
                pchk->dlen = ((unsigned int)*rxBuf << 8u);
                break;

            case NW_COMPRESS_LEN2:
                pchk->dlen += *rxBuf;
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = NW_COMPRESS_DATA;
                }
                break;

            case NW_COMPRESS_DATA:
                if (pchk->pbuf_pos == (3 + pchk->dlen))
                {
                    pchk->frame_state = NW_COMPRESS_END;
                }
                break;

            case NW_COMPRESS_END:
                if (*rxBuf == 0x77)
                {
                    int delen;
                    pchk->frame_state = NW_FRAME_STATES_COMPLETE;
                    pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
                    log_buf(L_DEBUG, "RM: ", pchk->pbuf, pchk->pbuf_pos + 1);
                    delen = DeData(pchk->pbuf, pchk->pbuf_pos + 1);
                    if (0 < delen)
                    {
                        free(pchk->pbuf);
                        pchk->pbuf = NULL;
                        pchk->frame_state = NW_FRAME_STATES_NULL;
                        log_buf(L_DEBUG, "JM: ", RecvBuf, delen);
                        nw_chkfrm(pc, pchk, RecvBuf, delen);
                    }
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            default:
                break;
        }

        if (pchk->frame_state != NW_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* �������ģ����ô������ӿ� */
        if (pchk->frame_state == NW_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //���ﴦ��ҵ��
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = NW_FRAME_STATES_NULL;
            pchk->pbuf_pos = 0;
            pchk->dlen = 0;
        }
        rxLen--;
        rxBuf++;
    }
}

/**
 ******************************************************************************
 * @brief   ��ȡ���Ĵ��䷽��,0:��վ-->�ն�, 1:�ն�-->��վ
 * @param[in]  *p : ���Ļ���
 *
 * @retval  0 : ��վ-->�ն�
 * @retval  1 : �ն�-->��վ
 ******************************************************************************
 */
static int
nw_get_dir(const unsigned char* p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return ph->DIR;
}

/**
 ******************************************************************************
 * @brief   ����pnfnƫ�ƻ�ȡfn
 * @param[in]  ����2�ֽ�fn
 * @retval  fn
 ******************************************************************************
 */
static unsigned int
getPnFn(const unsigned char* pFnPn)
{
    return *(unsigned int*) (pFnPn + 2);
}

/**
 ******************************************************************************
 * @brief   ��ȡ��������
 * @param[in]  *p : ���Ļ���
 *
 * @retval  ��������
 ******************************************************************************
 */
static func_type_e
nw_frame_type(const unsigned char* p)
{
    nw_header_t *ph = (nw_header_t *)p;

    if (ph->AFN == 0x02) //
    {
        if (ph->lenArea >= 6)
        {
            switch (getPnFn((unsigned char*)&ph[1]))
            {
            case 0xE0001000:
                return LINK_LOGIN;
            case 0xE0001002:
                return LINK_EXIT;
            case 0xE0001001:
                return LINK_HAERTBEAT;
            default:
                break;
            }
        }
    }
    return OTHER;
}

/**
 ******************************************************************************
 * @brief   ��������½����Ӧ
 * @param[in]  *p  : ���뱨�Ļ���
 * @param[in]  *po : ������Ļ���
 *
 * @return  ������ĳ���
 ******************************************************************************
 */
static int
nw_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    nw_header_t *pin = (nw_header_t *)p;
    nw_header_t *psend = (nw_header_t *)po;
    int pos = sizeof(nw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x11;
    psend->lenArea_ = 0x11;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    memcpy(psend->deviceAddr, pin->deviceAddr, 6);
    psend->hostID = pin->hostID; //msa = 2
    psend->AFN = 0;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //��֡
    psend->CON = 0; //�����յ��ı����У�CONλ�á�1������ʾ��Ҫ�Ը�֡���Ľ���ȷ�ϣ��á�0������ʾ����Ҫ�Ը�֡���Ľ���ȷ�ϡ�
    psend->frameSeq = pin->frameSeq;
    po[pos + 0] = 0x00;
    po[pos + 1] = 0x00;
    po[pos + 2] = 0x00;
    po[pos + 3] = 0x00;
    po[pos + 4] = 0x00;
    po[pos + 5] = 0xE0;
    po[pos + 6] = 0x00;
    po[pos + 7] = get_cs(po + 6, psend->lenArea); //cs
    po[pos + 8] = 0x16;

    return pos + 9;
}

/**
 ******************************************************************************
 * @brief   �ն˵�ַ�ͱ����е��ն˵�ַ�Ƚ�
 * @param[in]  *paddr : �����ն˵�ַ
 * @param[in]  *p     : ���뱨��
 *
 * @retval  1 : ����ͬ
 * @retval  0 :   ��ͬ
 ******************************************************************************
 */
static int
nw_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return memcmp(ph->deviceAddr, paddr, 6) ? 1 : 0;
}

/**
 ******************************************************************************
 * @brief   �ӱ�����ȡ���ն˵�ַ
 * @param[in]  *paddr : �����ն˵�ַ
 * @param[in]  *p     : ���뱨��
 *
 * @retval  1 : ����ͬ
 * @retval  0 :   ��ͬ
 ******************************************************************************
 */
static void
nw_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    memcpy(paddr, ph->deviceAddr, 6);   //6 bytes
}

/**
 ******************************************************************************
 * @brief   ��ȡ�ն˵�ַ�ַ���
 * @param[in]  *paddr : �ն˵�ַ
 *
 * @retval  �ն˵�ַ�ַ���
 ******************************************************************************
 */
static const char *
nw_addr_str(const addr_t *paddr)
{
    //910100-000100
    static char buf[14];

    sprintf(buf, "%02X%02X%02X-%02X%02X%02X",
            paddr->addr_c6[2], paddr->addr_c6[1], paddr->addr_c6[0],
            paddr->addr_c6[5], paddr->addr_c6[4], paddr->addr_c6[3]);
    return buf;
}

/**
 ******************************************************************************
 * @brief   ��վMSA��ַ�ͱ����е���վMSA��ַ�Ƚ�
 * @param[in]  *paddr : ������վMSA��ַ
 * @param[in]  *p     : ���뱨��
 *
 * @retval  1 : ����ͬ
 * @retval  0 :   ��ͬ
 ******************************************************************************
 */
static int
nw_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return (ph->hostID == paddr->msa) ? 0 : 1;
}

/**
 ******************************************************************************
 * @brief   �ӱ�����ȡ����վMSA��ַ
 * @param[in]  *paddr : ������վMSA��ַ
 * @param[in]  *p     : ���뱨��
 *
 * @retval  1 : ����ͬ
 * @retval  0 :   ��ͬ
 ******************************************************************************
 */
static void
nw_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->msa = ph->hostID;
}

/**
 ******************************************************************************
 * @brief   �ж���վ������MSA�Ƿ���Ч
 * @param[in]  *paddr : ������վMSA��ַ
 *
 * @retval  1 : ��Ч
 * @retval  0 : ��Ч
 ******************************************************************************
 */
static int
nw_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_nw_ptcl_func =
{
    "����2013",
    0,
    nw_chkfrm_init,
    nw_chkfrm,
    nw_get_dir,
    nw_frame_type,
    nw_build_reply_packet,
    nw_addr_cmp,
    nw_addr_get,
    nw_addr_str,
    nw_msa_cmp,
    nw_msa_get,
    nw_is_msa_valid,
    NULL,
    NULL,
};
/**
 ******************************************************************************
 * @brief   ��ȡ����Э�鴦��ӿ�
 * @retval  ����Э�鴦��ӿ�
 ******************************************************************************
 */
const ptcl_func_t *
nw_ptcl_func_get(void)
{
    return &the_nw_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
