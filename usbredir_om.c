/* 
 * File: 
 *     usbredir_om.c
 *
 * Destription:
 *     hold on usbredir ctrl block data structure 
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "usbredirfilter.h"

#include "usbredir_type.h"

#define USBREDIR_PORT_NUM 8

typedef struct USBREDIR_OM_CTRLBLK_S 
{
    BOOL_T usbredir_enabled;    /* usbredir enabled or not */
    BOOL_T service_available;   /* service is available or not */
    struct usbredirfilter_rule *rules; /* filter rules */
    UINT32_T rule_count;           /* rules count */
    UINT32_T serverip;             /* virtual host IP address */ 
    UINT16_T port[USBREDIR_PORT_NUM];      /* usbredir service port list */ 
    void *libusb_ctx;              /* libusb context */
}USBREDIR_OM_CTRLBLK_T;

static USBREDIR_OM_CTRLBLK_T usbredir_om_ctrlblk;
static pthread_mutex_t usbredir_om_mutex;

#define USBREDIR_OM_LOCK()    \
            pthread_mutex_lock(&usbredir_om_mutex)
#define USBREDIR_OM_UNLOCK()  \
            pthread_mutex_unlock(&usbredir_om_mutex)


BOOL_T usbredir_om_init()
{
    int ret;

    memset((void *)&usbredir_om_ctrlblk, 0, sizeof(usbredir_om_ctrlblk));

    ret = pthread_mutex_init(&usbredir_om_mutex, NULL);
    if (ret != 0)
    {
        fprintf(stderr, "Could not init usbredir om mutex\n");
        return FALSE;
    }

    return TRUE;
}

void usbredir_om_reset()
{
    if (NULL != usbredir_om_ctrlblk.rules)
        free(usbredir_om_ctrlblk.rules);

    memset((void *)&usbredir_om_ctrlblk, 0, sizeof(usbredir_om_ctrlblk));
    pthread_mutex_destroy(&usbredir_om_mutex);
}

void usbredir_om_set_libusbctx(void *ctx)
{
    USBREDIR_OM_LOCK();
    usbredir_om_ctrlblk.libusb_ctx = ctx;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_get_libusbctx(void **ctx)
{
    USBREDIR_OM_LOCK();
    *ctx = usbredir_om_ctrlblk.libusb_ctx;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_set_enabled(BOOL_T enabled)
{
    USBREDIR_OM_LOCK();
    usbredir_om_ctrlblk.usbredir_enabled = enabled;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_get_enabled(BOOL_T *enabled)
{
    USBREDIR_OM_LOCK();
    *enabled = usbredir_om_ctrlblk.usbredir_enabled;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_set_service_available(BOOL_T available)
{
    USBREDIR_OM_LOCK();
    usbredir_om_ctrlblk.service_available = available;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_get_service_available(BOOL_T *available)
{
    USBREDIR_OM_LOCK();
    *available = usbredir_om_ctrlblk.service_available;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_set_serverip(UINT32_T serverip)
{
    USBREDIR_OM_LOCK();
    usbredir_om_ctrlblk.serverip = serverip;
    USBREDIR_OM_UNLOCK();
}

void usbredir_om_get_serverip(UINT32_T *serverip)
{
    USBREDIR_OM_LOCK();
    *serverip = usbredir_om_ctrlblk.serverip;
    USBREDIR_OM_UNLOCK();
}

BOOL_T usbredir_om_set_serverport(UINT16_T port)
{
    int i;

    if (0 == port)
        return FALSE;

    USBREDIR_OM_LOCK();

    for (i=0; i<USBREDIR_PORT_NUM; i++)
    {
        if (usbredir_om_ctrlblk.port[i] == port)
        {
            USBREDIR_OM_UNLOCK();
            return TRUE;
        }
    }

    for (i=0; i<USBREDIR_PORT_NUM; i++)
    {
        if (usbredir_om_ctrlblk.port[i] == 0)
        {
            usbredir_om_ctrlblk.port[i] = port;
            USBREDIR_OM_UNLOCK();
            return TRUE;
	}
    }

    USBREDIR_OM_UNLOCK();
    return FALSE;
}

BOOL_T usbredir_om_get_serverport(UINT16_T *port)
{
    int i;

    if (NULL == port){
        return FALSE;
    }

    USBREDIR_OM_LOCK();

    for (i=0; i<USBREDIR_PORT_NUM; i++)
    {
        if (usbredir_om_ctrlblk.port[i] != 0)
        {
            *port = usbredir_om_ctrlblk.port[i];
            usbredir_om_ctrlblk.port[i] = 0;

            USBREDIR_OM_UNLOCK();
            return TRUE; 
	}
    }
  
    *port = 0;
    USBREDIR_OM_UNLOCK();

    return FALSE; 
}

BOOL_T usbredir_om_set_filtrules(struct usbredirfilter_rule *rules, UINT32_T rule_count)
{
    if (NULL == rules || 0 == rule_count)
        return FALSE;

    USBREDIR_OM_LOCK();

    if (NULL != usbredir_om_ctrlblk.rules)
        free(usbredir_om_ctrlblk.rules); 

    usbredir_om_ctrlblk.rules = calloc(rule_count, sizeof(struct usbredirfilter_rule));
    if (NULL == usbredir_om_ctrlblk.rules){
        USBREDIR_OM_UNLOCK();
        return FALSE;
    }
 
    usbredir_om_ctrlblk.rule_count = rule_count;
    memcpy(usbredir_om_ctrlblk.rules, rules, rule_count * sizeof(struct usbredirfilter_rule));

    USBREDIR_OM_UNLOCK();
    return TRUE;
}

BOOL_T usbredir_om_get_filtrules(struct usbredirfilter_rule **rules, UINT32_T *rule_count)
{
    if (NULL == rules || 0 == rule_count)
        return FALSE;

    USBREDIR_OM_LOCK();

    *rules = usbredir_om_ctrlblk.rules;
    *rule_count = usbredir_om_ctrlblk.rule_count;

    USBREDIR_OM_UNLOCK();
    return TRUE;
}

