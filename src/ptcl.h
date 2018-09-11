/**
 ******************************************************************************
 * @file       ptcl.h
 * @brief      API include file of ptcl.h.
 * @details    This file including all API functions's declare of ptcl.h.
 * @copyright  
 *
 ******************************************************************************
 */
#ifndef PTCL_H_
#define PTCL_H_ 

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef enum
{
    E_PTCL_GW,  /**< ����2013��Լ */
    E_PTCL_NW,   /**< �����¹�Լ */
    E_PTCL_END,
} ptcl_type_e;

/** �������� */
typedef enum FUNCTYPE
{
    LINK_LOGIN = 0,
    LINK_EXIT,
    LINK_HAERTBEAT,
    OTHER,
    FUNCERROR,
    ONLINE
} func_type_e;

#pragma pack(push, 1)

/** ��վ���ն�ͨ�õ�ַ�ṹ6�ֽ� */
typedef union
{
    int msa;
    int addr;                   /**< �ն˵�ַ */
    unsigned char addr_c4[4];
    unsigned char addr_c6[6];
    unsigned char addr_c16[16];
} addr_t;

typedef struct
{
    void (*pfn_frame_in)(void*, const unsigned char*, int);
    unsigned char frame_state;  /* ����״̬ */
    time_t update_time;         /* ����ʱ�� */
    unsigned char overtime;     /* ���ĵȴ���ʱʱ�� */
    unsigned int dlen;          /* �û����ݳ���,�����Ա��ĳ����� */
    unsigned int cfm_len;       /* �û�����ȷ�ϳ��ȣ����ڽ������ݳ�����ʱ�� */

    unsigned char *pbuf;        /* ���ĳ� */
    unsigned int pbuf_pos;      /* ���Ľ����ֽ�ƫ��λ�� */
    unsigned char cs;           /* У��� */
} chkfrm_t;

/** Э�鴦��ӿ� */
typedef struct
{
    const char *pname; /**< Э������ */

    char support_app_heart; /**< �Ƿ�֧�ֺ�̨�����½������ */

    /**
     * ���ļ���ʼ��
     */
    void (*pfn_chkfrm_init)(chkfrm_t *,
            void (*pfn_frame_in)(void*, const unsigned char*, int));

    /**
     * ���ļ��
     */
    void (*pfn_chfrm)(void*, chkfrm_t*, const unsigned char*, int);

    /**
     *  ��ȡ���Ĵ��䷽��,0:��վ-->�ն�, 1:�ն�-->��վ
     */
    int (*pfn_get_dir)(const unsigned char*);

    /**
     * ��ȡ��������
     */
    func_type_e (*pfn_frame_type)(const unsigned char*);

    /**
     * �����½�������ظ���
     */
    int (*pfn_build_reply_packet)(const unsigned char*, unsigned char*);

    /**
     * �ն˵�ַ�Ƚ�
     */
    int (*pfn_addr_cmp)(const addr_t*, const unsigned char*);

    /**
     * �ӱ�����ȡ���ն˵�ַ
     */
    void (*pfn_addr_get)(addr_t*, const unsigned char*);

    /**
     * ��ȡ�ն��ַ���
     */
    const char* (*pfn_addr_str)(const addr_t*);

    /**
     * ��վMSA��ַ�Ƚ�
     */
    int (*pfn_msa_cmp)(const addr_t*, const unsigned char*);

    /**
     * �ӱ�����ȡ����վMSA��ַ
     */
    void (*pfn_msa_get)(addr_t*, const unsigned char*);

    /**
     * �ж���վ������msa�Ƿ���Ч
     */
    int (*pfn_is_msa_valid)(const addr_t*);

    /**
     * ��ȡ��վ������ѯ�Ƿ������ն˵�ַ
     */
    int (*pfn_get_oline_addr)(addr_t*, const unsigned char *);

    /**
     * ������ѯonline�ظ�����
     */
    int (*pfn_build_online_packet)(const unsigned char*, unsigned char*, int);
} ptcl_func_t;

#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
extern int the_max_frame_bytes;

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern const ptcl_func_t *
gw_ptcl_func_get(void);

extern const ptcl_func_t *
nw_ptcl_func_get(void);

extern const ptcl_func_t *
zj_ptcl_func_get(void);

extern const ptcl_func_t *
jl_ptcl_func_get(void);

extern const ptcl_func_t *
p47_ptcl_func_get(void);

extern const ptcl_func_t *
p698_ptcl_func_get(void);

#endif /* PTCL_H_ */
/*------------------------------End of ptcl.h--------------------------------*/
