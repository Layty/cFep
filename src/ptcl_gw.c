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
 
/**
 * ��չЭ��:
 * A1(0)
 * A2(0)
 * AFN(0xFE)
 * FN(0)PN(0)��ѯ�ն����ͨ�ž���������
 *
 * ��̨ --> ǰ�û���ʽ:
 * �ն˵�ַ(4�ֽ�)
 * 68
 *
 * ǰ�û� --> ��̨��ʽ:
 * �ն˵�ַ(4�ֽ�)
 * ���ͨ�ž���������(4�ֽ�BIN)(�������ߣ�ֵΪ0xFFFFFFFF)
 * ��������Զ���ϵͳ��վϵͳ����ǰ�û����ն�״̬��Э�飺
 *
 * ������
 *   ��վ�·���68 42 00 42 00 68 40 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 0A 16�������ѯ91010001�ն�����״̬��
 *   �ն˷���һ��68 52 00 52 00 68 C0 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 FF FF FF FF 86 16������91010001�ն˲����ߣ�
 *   �ն˷��ض���68 52 00 52 00 68 C0 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 10 00 00 00 9A 16������91010001�ն����һ�κ�ǰ�û�ͨ�ŵ�ʱ����16sǰ��
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
typedef struct FrameGBHeaderStruct
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
} gw_header_t;

//�¼��������ṹ
struct ECStruct
{
    unsigned char impEC;
    unsigned char norEC;
};

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

//AFN10_F1
struct TransparentSend
{
    unsigned char Comm;
};
#pragma pack(pop)



/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< ���ĳ�ʱʱ��Ĭ��10��  */
#define FRAME_NO_DATA_LEN               14   /**< ���������������ݱ�ʾ�ı���ͷ���� */

/* states for scanning incomming bytes from a bytestream */
#define GW_FRAME_STATES_NULL               0    /**< no synchronisation */
#define GW_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define GW_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define GW_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define GW_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define GW_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define GW_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define GW_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define GW_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define GW_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define GW_FRAME_STATES_CS                 10   /**< wait for the CS */
#define GW_FRAME_STATES_END                11   /**< wait for the 16H */
#define GW_FRAME_STATES_COMPLETE           12   /**< complete frame */

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
gw_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = GW_FRAME_STATES_NULL;
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
gw_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* ����Ѿ���ɵ��������¿�ʼ */
    if (pchk->frame_state == GW_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = GW_FRAME_STATES_NULL;
    }

    /* �����ʱ�����¿�ʼ */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = GW_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case GW_FRAME_STATES_NULL:

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
                    pchk->frame_state = GW_FRAME_STATES_LEN1_1;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case GW_FRAME_STATES_LEN1_1: /* ���L1�ĵ��ֽ� */
                pchk->frame_state = GW_FRAME_STATES_LEN1_2;/* Ϊ������վ������Լ���� */
                pchk->dlen = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case GW_FRAME_STATES_LEN1_2: /* ���L1�ĸ��ֽ� */
                pchk->dlen += ((unsigned int)*rxBuf << 6u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = GW_FRAME_STATES_LEN2_1;
                }
                break;

            case GW_FRAME_STATES_LEN2_1: /*���L2�ĵ��ֽ�*/
                pchk->frame_state = GW_FRAME_STATES_LEN2_2;
                pchk->cfm_len = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case GW_FRAME_STATES_LEN2_2: /*���L2�ĸ��ֽ�*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 6u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = GW_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = GW_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = GW_FRAME_STATES_A3;/* ���ܼ�ⷽ����Ϊ���������б��� */
                break;

            case GW_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 11)
                {
                    pchk->frame_state = GW_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case GW_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = GW_FRAME_STATES_CS;
                }
                break;

            case GW_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = GW_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = GW_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;
            default:
                break;
        }

        if (pchk->frame_state != GW_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* �������ģ����ô������ӿ� */
        if (pchk->frame_state == GW_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //���ﴦ��ҵ��
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = GW_FRAME_STATES_NULL;
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
gw_get_dir(const unsigned char* p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_frame_type(const unsigned char* p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
    else if (ph->AFN == 0xFE)
    {
        if ((ph->lenArea == 0x42) && (!ph->logicaddr) && ((*(int*)&ph[1]) == 0))
        {
            return ONLINE;
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
gw_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    gw_header_t *pin = (gw_header_t *)p;
    gw_header_t *psend = (gw_header_t *)po;
    int pos = sizeof(gw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x48 | (pin->lenArea & 0x03);
    psend->lenArea_ = 0x48 | (pin->lenArea & 0x03);;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    psend->logicaddr = pin->logicaddr;
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
gw_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_addr_str(const addr_t *paddr)
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
gw_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_is_msa_valid(const addr_t *paddr)
{
    return paddr->msa ? 1 : 0;
}

/**
 ******************************************************************************
 * @brief   ��ȡ��վ������ѯ�Ƿ������ն˵�ַ
 * @param[out] *paddr : ������վMSA��ַ
 * @param[in]  *p     : ���뱨��
 *
 * @retval  1 : �ɹ�
 * @retval  0 : ʧ��
 ******************************************************************************
 */
static int
gw_get_oline_addr(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;
    if (ph->lenArea == 0x42)
    {
        memset(paddr, 0x00, sizeof(addr_t));
        memcpy(paddr, &p[18], 4);
//        gw_addr_get(paddr, (unsigned char*)&p[1 + 4]);
        return 1;
    }
    return 0;
}

/**
 ******************************************************************************
 * @brief   ������ѯonline�ظ�����
 * @param[in]  *p  : ���뱨�Ļ���
 * @param[in]  *po : ������Ļ���
 * @param[in]  sec : ���һ��ͨ�ž���������
 *
 * @return  ������ĳ���
 ******************************************************************************
 */
static int
gw_build_online_packet(const unsigned char *p,
        unsigned char *po,
        int sec)
{
    gw_header_t *pin = (gw_header_t *)p;
    gw_header_t *psend = (gw_header_t *)po;
    int pos = sizeof(gw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x52;
    psend->lenArea_ = 0x52;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0xC0;
    psend->logicaddr = 0;
    psend->hostIDArea = pin->hostIDArea;
    psend->AFN = 0xFE;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //��֡
    psend->CON = 0; //�����յ��ı����У�CONλ�á�1������ʾ��Ҫ�Ը�֡���Ľ���ȷ�ϣ��á�0������ʾ����Ҫ�Ը�֡���Ľ���ȷ�ϡ�
    psend->frameSeq = pin->frameSeq;
    memcpy(&po[pos + 8], &sec, 4);
    po[pos + 12] = get_cs(po + 6, psend->lenDataArea); //cs
    po[pos + 13] = 0x16;

    return pos + 14;
}

static const ptcl_func_t the_gw_ptcl_func =
{
    "����1376.1",
    0,
    gw_chkfrm_init,
    gw_chkfrm,
    gw_get_dir,
    gw_frame_type,
    gw_build_reply_packet,
    gw_addr_cmp,
    gw_addr_get,
    gw_addr_str,
    gw_msa_cmp,
    gw_msa_get,
    gw_is_msa_valid,
    gw_get_oline_addr,
    gw_build_online_packet,
};
/**
 ******************************************************************************
 * @brief   ��ȡ����Э�鴦��ӿ�
 * @retval  ����Э�鴦��ӿ�
 ******************************************************************************
 */
const ptcl_func_t *
gw_ptcl_func_get(void)
{
    return &the_gw_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
