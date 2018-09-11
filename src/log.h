/**
 ******************************************************************************
 * @file       log.h
 * @brief      ��־��¼ģ��
 * @details    This file including all API functions's declare of log.h.
 * @copyright
 *
 ******************************************************************************
 */
#ifndef LOG_H_
#define LOG_H_ 

#ifdef __cplusplus             /* Maintain C++ compatibility */
extern "C" {
#endif /* __cplusplus */
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/**
 * Ĭ�ϵ��Լ���
 * 0 : ��ӡ������Ϣ
 * 1 : ��ӡ������Ϣ + ������־
 * 2 : ��ӡ������Ϣ + ������־ + ���Դ�ӡ��Ϣ
 */
#define L_ERROR     0
#define L_NORMAL    1
#define L_DEBUG     2

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
extern int the_log_level;

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern void
log_set_level(int level);

extern int
log_init(void);

extern void
log_exit(void);

extern void
log_buf(int level,
        const char *pformat,
        const unsigned char *pbuffer,
        int len);

void
log_print(int level,
        const char *fmt, ...);

#ifdef __cplusplus      /* Maintain C++ compatibility */
}
#endif /* __cplusplus */
#endif /* LOG_H_ */
/*--------------------------End of log.h-----------------------------*/
