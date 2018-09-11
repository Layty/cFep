/**
 ******************************************************************************
 * @file      ptcl_zj.c
 * @brief     C Source file of ptcl_zj.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_zj.c.
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
#include "param.h"
#include "lib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
/*  structure of normal frame from the Data Link Layer

    Flag          ( 1 Byte 68H)
    Rtua          ( 4 Byte rtu logical address )
    MSTA          ( 2 Byte master station address)
    Flag          ( 1 Byte 68H)
    Control       ( 1 Byte )
    Length        ( 2 Bytes)
    DATA          ( n Bytes, depend only from the length)
    CS            ( 1 Byte )
    Flag          ( 1 Byte 16H)
*/
//����ͷ�ṹ��
typedef struct FrameZJHeaderStruct
{
    unsigned char frameStart;//0x68
    union
    {
        unsigned char deviceAddr[4];
        unsigned int logicaddr;//�ն��߼���ַ
        struct
        {
            unsigned char canton[2];
            unsigned short addr;
        };
    };
    union
    {
        struct
        {
            unsigned short int    hostID        :    6 ,
                                  frameSeq      :    7 ,
                                  seqInFrame    :    3 ;
        };
        unsigned short int hostIDArea;//D0--D5��վ��ַ  D6--D12֡��ź� D13-D15֡����
    };
    unsigned char dataAreaStart;//0x68
    union
    {
        struct
        {
            unsigned char    AFN                :    6 ,
                             exception          :    1 ,
                             DIR                :    1 ;
        };
        unsigned char ctrlCodeArea;//D0--D5������  D6�쳣��־ D7���䷽��
    };
    unsigned short int lenDataArea;//֡ͷ֮�󣬵�����֮ǰ�������򳤶�
} zj_header_t;

struct PSWStruct
{
    unsigned char level;
    unsigned char psw[3];
};
#pragma pack(pop)



/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< ���ĳ�ʱʱ��Ĭ��10��  */
#define FRAME_NO_DATA_LEN               14   /**< ���������������ݱ�ʾ�ı���ͷ���� */

/* states for scanning incomming bytes from a bytestream */
#define ZJ_FRAME_STATES_NULL               0    /**< no synchronisation */
#define ZJ_FRAME_STATES_FLAG_FIR           1    /**< 1֡��ʼ�� */
#define ZJ_FRAME_STATES_RTUA               2    /**< 4�ն��߼���ַ */
#define ZJ_FRAME_STATES_MSTA               3    /**< 2��վ��ַ��������� */
#define GW_FRAME_STATES_FLAG_SEC           4    /**< 0x16 */
#define ZJ_FRAME_STATES_CONTROL            5    /**< 1������ */
#define ZJ_FRAME_STATES_LEN1               6    /**< ���ݳ���1 */
#define ZJ_FRAME_STATES_LEN2               7    /**< ���ݳ���2 */
#define ZJ_FRAME_STATES_LINK_USER_DATA     8    /**< ������ */
#define ZJ_FRAME_STATES_CS                 9    /**< wait for the CS */
#define ZJ_FRAME_STATES_END                10   /**< wait for the 16H */
#define ZJ_FRAME_STATES_COMPLETE           11   /**< complete frame */

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
zj_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = ZJ_FRAME_STATES_NULL;
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
zj_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* ����Ѿ���ɵ��������¿�ʼ */
    if (pchk->frame_state == ZJ_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = ZJ_FRAME_STATES_NULL;
    }

    /* �����ʱ�����¿�ʼ */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = ZJ_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case ZJ_FRAME_STATES_NULL:
                if (*rxBuf == 0x68)
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
                    pchk->frame_state = ZJ_FRAME_STATES_RTUA;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                    pchk->cs = 0x68;
                }
                break;

            case ZJ_FRAME_STATES_RTUA:
            case ZJ_FRAME_STATES_MSTA:
            case GW_FRAME_STATES_FLAG_SEC:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 7)
                {
                    if (*rxBuf == 0x68)
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_CONTROL;
                    }
                    else
                    {
                        free(pchk->pbuf);
                        pchk->pbuf = NULL;
                        pchk->frame_state = ZJ_FRAME_STATES_NULL;
                    }
                }
                break;

            case ZJ_FRAME_STATES_CONTROL:
                pchk->cs += *rxBuf;
                pchk->frame_state = ZJ_FRAME_STATES_LEN1;/* ���ܼ�ⷽ����Ϊ���������б��� */
                break;

            case ZJ_FRAME_STATES_LEN1: /* ���L1�ĵ��ֽ� */
                pchk->cs += *rxBuf;
                pchk->frame_state = ZJ_FRAME_STATES_LEN2;/* Ϊ������վ������Լ���� */
                pchk->dlen = *rxBuf;
                break;

            case ZJ_FRAME_STATES_LEN2: /* ���L1�ĸ��ֽ� */
                pchk->dlen += ((unsigned int)*rxBuf << 8u);
                if (pchk->dlen > (the_max_frame_bytes - 13)) //fixme:
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                else
                {
                    if (pchk->dlen)
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_LINK_USER_DATA;
                    }
                    else
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_CS;
                    }
                }
                break;

            case ZJ_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (10 + pchk->dlen))
                {
                    pchk->frame_state = ZJ_FRAME_STATES_CS;
                }
                break;

            case ZJ_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = ZJ_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                break;

            case ZJ_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = ZJ_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                break;

            default:
                break;
        }

        if (pchk->frame_state != ZJ_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* �������ģ����ô������ӿ� */
        if (pchk->frame_state == ZJ_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->pbuf_pos); //���ﴦ��ҵ��
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = ZJ_FRAME_STATES_NULL;
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
zj_get_dir(const unsigned char* p)
{
    zj_header_t *ph = (zj_header_t *)p;

    return ph->DIR;
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
zj_frame_type(const unsigned char* p)
{
    zj_header_t *ph = (zj_header_t *)p;

    switch (ph->AFN)
    {
        case 0x21:
            if ((ph->lenDataArea == 3) || (ph->lenDataArea == 8))
            {
                return LINK_LOGIN;
            }
            break;

        case 0x22:
            if (!ph->lenDataArea)
            {
                return LINK_EXIT;
            }
            break;

        case 0x24:
            if (!ph->lenDataArea)
            {
                return LINK_HAERTBEAT;
            }
            break;

        default:
            break;
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
zj_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    zj_header_t *pin = (zj_header_t *)p;
    zj_header_t *psend = (zj_header_t *)po;
    int pos = sizeof(zj_header_t);

    psend->frameStart = 0x68;
    psend->logicaddr = pin->logicaddr;
    psend->seqInFrame = 0;
    psend->frameSeq = pin->frameSeq;
    psend->hostIDArea = 40;

    psend->dataAreaStart = 0x68;

    psend->ctrlCodeArea = pin->ctrlCodeArea;
    psend->exception = 0;
    psend->DIR = 0; //0 : ��վ-->�ն�

    psend->lenDataArea = 0;

    po[pos + 0] = get_cs(po, psend->lenDataArea + 11); //cs;
    po[pos + 1] = 0x16;

    return pos + 2;
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
zj_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

    return (ph->logicaddr == paddr->addr) ? 0 : 1;
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
zj_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->addr = ph->logicaddr;
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
zj_addr_str(const addr_t *paddr)
{
    //9101-0001
    static char buf[10];

    sprintf(buf, "%02X%02X-%02X%02X", paddr->addr_c4[1], paddr->addr_c4[0],
            paddr->addr_c4[3], paddr->addr_c4[2]);
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
zj_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

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
zj_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

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
zj_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_zj_ptcl_func =
{
    "�㶫���㽭",
    1,
    zj_chkfrm_init,
    zj_chkfrm,
    zj_get_dir,
    zj_frame_type,
    zj_build_reply_packet,
    zj_addr_cmp,
    zj_addr_get,
    zj_addr_str,
    zj_msa_cmp,
    zj_msa_get,
    zj_is_msa_valid,
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
zj_ptcl_func_get(void)
{
    return &the_zj_ptcl_func;
}
/*-------------------------------ptcl_zj.c-----------------------------------*/
