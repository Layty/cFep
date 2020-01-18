/**
 ******************************************************************************
 * @file      ptcl_jl.c
 * @brief     C Source file of ptcl_jl.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_jl.c.
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
//����ͷ�ṹ��
typedef struct FrameJLHeaderStruct
{
    unsigned char frameStart;       //0x68
    union
    {
        struct
        {
            unsigned short  ptclFlag    :   2   ,   //D0--D1��Լ��ʾ
                                                    //D0=0��D1=0����ʾ�����ã�
                                                    //D0=1��D1=0����ʾ������Լʹ�ã�
                                                    //D0=0��1��D1=1��Ϊ������
                            lenDataArea :   14  ;   //D2--D15���ȣ������򡢵�ַ����·�û����ݳ���������
        };
        unsigned short      lenArea;
    };
    unsigned short  lenArea_;       //�ظ��ĳ�����
    unsigned char   dataAreaStart;  //0x68
    union
    {
        struct
        {
            unsigned char   FuncCode : 4 ,  //D0--D3����������
                            FCV      : 1 ,  //D4֡������Чλ
                            FCB_ACD  : 1 ,  //D5֡����λ ���� �������λ
                            PRM      : 1 ,  //D6������־λ
                            DIR      : 1 ;  //D7���䷽��λ
        };
        unsigned char ctrlCodeArea;
    };
    union
    {
        unsigned char deviceAddr[6];//�ն��߼���ַ
    };
    union
    {
        struct
        {
            unsigned char   groupAddr   :    1 ,//D0 �ն����ַ
                            hostID      :    7 ;//D1-D7��վ��ַ
        };
        unsigned char hostIDArea;
    };
    unsigned char AFN;
    union
    {
        struct
        {
            unsigned char    frameSeq   :    4 ,//D0--D3֡���
                                CON     :    1 ,//D4����ȷ�ϱ�־λ
                                FIN     :    1 ,//D5����֡��־
                                FIR     :    1 ,//D6��֡��־
                                TPV     :    1 ;//D7֡ʱ���ǩ��Чλ
        };
        unsigned char seqArea;
    };
} jl_header_t;

#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< ���ĳ�ʱʱ��Ĭ��10��  */
#define FRAME_NO_DATA_LEN               14   /**< ���������������ݱ�ʾ�ı���ͷ���� */

/* states for scanning incomming bytes from a bytestream */
#define JL_FRAME_STATES_NULL               0    /**< no synchronisation */
#define JL_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define JL_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define JL_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define JL_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define JL_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define JL_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define JL_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define JL_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define JL_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define JL_FRAME_STATES_CS                 10   /**< wait for the CS */
#define JL_FRAME_STATES_END                11   /**< wait for the 16H */
#define JL_FRAME_STATES_COMPLETE           12   /**< complete frame */

#define GB_PRPTOCOL_TYPE                0x01 /**< 2005��Լ��ʶ */
#define GW_PRPTOCOL_TYPE                0x02 /**< ������Լ��ʶ */

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
jl_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = JL_FRAME_STATES_NULL;
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
jl_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* ����Ѿ���ɵ��������¿�ʼ */
    if (pchk->frame_state == JL_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = JL_FRAME_STATES_NULL;
    }

    /* �����ʱ�����¿�ʼ */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = JL_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case JL_FRAME_STATES_NULL:

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
                    pchk->frame_state = JL_FRAME_STATES_LEN1_1;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case JL_FRAME_STATES_LEN1_1: /* ���L1�ĵ��ֽ� */
                pchk->frame_state = JL_FRAME_STATES_LEN1_2;/* Ϊ������վ������Լ���� */
                pchk->dlen = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case JL_FRAME_STATES_LEN1_2: /* ���L1�ĸ��ֽ� */
                pchk->dlen += ((unsigned int)*rxBuf << 6u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = JL_FRAME_STATES_LEN2_1;
                }
                break;

            case JL_FRAME_STATES_LEN2_1: /*���L2�ĵ��ֽ�*/
                pchk->frame_state = JL_FRAME_STATES_LEN2_2;
                pchk->cfm_len = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case JL_FRAME_STATES_LEN2_2: /*���L2�ĸ��ֽ�*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 6u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = JL_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = JL_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = JL_FRAME_STATES_A3;/* ���ܼ�ⷽ����Ϊ���������б��� */
                break;

            case JL_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 13)
                {
                    pchk->frame_state = JL_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case JL_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = JL_FRAME_STATES_CS;
                }
                break;

            case JL_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = JL_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = JL_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;
            default:
                break;
        }

        if (pchk->frame_state != JL_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* �������ģ����ô������ӿ� */
        if (pchk->frame_state == JL_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //���ﴦ��ҵ��
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = JL_FRAME_STATES_NULL;
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
jl_get_dir(const unsigned char* p)
{
    jl_header_t *ph = (jl_header_t *)p;

    return ph->DIR;
}

/**
 ******************************************************************************
 * @brief   ����pnfnƫ�ƻ�ȡfn
 * @param[in]  ����2�ֽ�fn
 * @retval  fn
 ******************************************************************************
 */
static unsigned short
getPnFn(const unsigned char* pFnPn)
{
    int i;
    unsigned short nFn;
    nFn = pFnPn[3]*8;

    for(i = 0; i < 8; i++)
    {
        if( ((pFnPn[2] >> i) & 0x01) == 0x01)
        {
            nFn += i+1;
        }
    }
    return nFn;
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
jl_frame_type(const unsigned char* p)
{
    jl_header_t *ph = (jl_header_t *)p;

    if (ph->AFN == 0x02) //
    {
        if (ph->lenArea >= 4)
        {
            switch (getPnFn((unsigned char*)&ph[1]))
            {
            case 1:
                return LINK_LOGIN;
            case 2:
                return LINK_EXIT;
            case 3:
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
jl_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    jl_header_t *pin = (jl_header_t *)p;
    jl_header_t *psend = (jl_header_t *)po;
    int pos = sizeof(jl_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x52;
    psend->lenArea_ = 0x52;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    memcpy(psend->deviceAddr, pin->deviceAddr, 6);
    psend->hostIDArea = 2; //msa = 2
    psend->AFN = 0;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //��֡
    psend->CON = 0; //�����յ��ı����У�CONλ�á�1������ʾ��Ҫ�Ը�֡���Ľ���ȷ�ϣ��á�0������ʾ����Ҫ�Ը�֡���Ľ���ȷ�ϡ�
    psend->frameSeq = pin->frameSeq;
    po[pos + 0] = 0x00;
    po[pos + 1] = 0x00;
    po[pos + 2] = 0x04;
    po[pos + 3] = 0x00;
    po[pos + 4] = 0x02;   //ȷ��afn
    memcpy(&po[pos + 5], &pin[1], 4);
    po[pos + 9] = 0;
    po[pos + 10] = get_cs(po + 6, psend->lenDataArea); //cs
    po[pos + 11] = 0x16;

    return pos + 12;
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
jl_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_addr_str(const addr_t *paddr)
{
    //9101-00000001
    static char buf[16];

    snprintf(buf, sizeof(buf), "%02X%02X-%02X%02X%02X%02X",
            paddr->addr_c6[1], paddr->addr_c6[0],
            paddr->addr_c6[5], paddr->addr_c6[4],
            paddr->addr_c6[3], paddr->addr_c6[2]);
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
jl_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_jl_ptcl_func =
{
    "���ֹ�Լ",
    0,
    jl_chkfrm_init,
    jl_chkfrm,
    jl_get_dir,
    jl_frame_type,
    jl_build_reply_packet,
    jl_addr_cmp,
    jl_addr_get,
    jl_addr_str,
    jl_msa_cmp,
    jl_msa_get,
    jl_is_msa_valid,
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
jl_ptcl_func_get(void)
{
    return &the_jl_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
