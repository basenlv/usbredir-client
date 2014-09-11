/* 
 * File: 
 *     usbredir_mgr.c
 *
 * Destription:
 *     hold on usbredir manament API interface
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
#include <libusb.h>

#include "usbredirfilter.h"
#include "usbredir_type.h"
#include "usbredir_om.h"
#include "usbredir_log.h"
#include "usbredir_event.h"
#include "usbredir_monitor.h"

static libusb_context *libusb_ctx = NULL;
static pthread_mutex_t usbredir_mgr_mutex;

#define USBREDIR_MGR_LOCK()    \
            pthread_mutex_lock(&usbredir_mgr_mutex)
#define USBREDIR_MGR_UNLOCK()  \
            pthread_mutex_unlock(&usbredir_mgr_mutex)

BOOL_T usbredir_mgr_init()
{
    int ret;

    ret = libusb_init(&libusb_ctx);
    if (ret)
    {
        USBREDIR_LOG_ERROR("usbredir mgr libusb init fail.");
        return FALSE;
    }

    ret = pthread_mutex_init(&usbredir_mgr_mutex, NULL);
    if (ret != 0)
    {
        USBREDIR_LOG_ERROR("usbredir mgr init mutex fail.");
        libusb_exit(libusb_ctx);
        libusb_ctx = NULL;
        return FALSE;
    }

    usbredir_om_init();
    usbredir_om_set_libusbctx((void *)libusb_ctx); 

    USBREDIR_LOG_INFO("usbredir mgr initialized success.");
    return TRUE;
}

void usbredir_mgr_destroy()
{
    usbredir_om_reset(); 

    if (libusb_ctx){
        libusb_exit(libusb_ctx);
        libusb_ctx = NULL;
    }

    pthread_mutex_destroy(&usbredir_mgr_mutex);

    USBREDIR_LOG_INFO("usbredir mgr destroy success.");
}

void usbredir_mgr_set_enabled(BOOL_T enabled)
{
    USBREDIR_MGR_LOCK();

    USBREDIR_LOG_DEBUG("usbredir mgr set to %s.", enabled?"enabled":"disabled");

    usbredir_om_set_enabled(enabled);
    if (enabled){
        usbredir_monitor_create_thread();
    }
    else{
        usbredir_monitor_destroy_thread();
    }

    USBREDIR_MGR_UNLOCK();
}

void usbredir_mgr_get_enabled(BOOL_T *enabled)
{
    USBREDIR_MGR_LOCK();

    usbredir_om_get_enabled(enabled);

    USBREDIR_MGR_UNLOCK();
}

void usbredir_mgr_set_service_available(BOOL_T available)
{
    USBREDIR_MGR_LOCK();

    usbredir_om_set_service_available(available);
    USBREDIR_LOG_DEBUG("usbredir mgr set service to %s.", available?"available":"not available");

    USBREDIR_MGR_UNLOCK();
}

void usbredir_mgr_get_service_available(BOOL_T *available)
{
    USBREDIR_MGR_LOCK();

    usbredir_om_get_service_available(available);

    USBREDIR_MGR_UNLOCK();
}

void usbredir_mgr_set_serverip(UINT32_T serverip)
{
    USBREDIR_MGR_LOCK();

    usbredir_om_set_serverip(serverip);
    USBREDIR_LOG_DEBUG("usbredir mgr set server ip address.");

    USBREDIR_MGR_UNLOCK();
}

void usbredir_mgr_get_serverip(UINT32_T *serverip)
{
    USBREDIR_MGR_LOCK();

    usbredir_om_get_serverip(serverip);

    USBREDIR_MGR_UNLOCK();
}

BOOL_T usbredir_mgr_set_serverport(UINT16_T port)
{
    USBREDIR_MGR_LOCK();

    if (FALSE == usbredir_om_set_serverport(port))
    {
        USBREDIR_LOG_ERROR("usbredir mgr set port %d fail.", port);
        USBREDIR_MGR_UNLOCK();
        return FALSE;
    }

    USBREDIR_LOG_DEBUG("usbredir mgr set port %d success.", port);
    USBREDIR_MGR_UNLOCK();
    return TRUE;
}

BOOL_T usbredir_mgr_get_serverport(UINT16_T *port)
{
    USBREDIR_MGR_LOCK();

    if (FALSE == usbredir_om_get_serverport(port))
    {
        USBREDIR_LOG_ERROR("usbredir mgr get port fail.");
        USBREDIR_MGR_UNLOCK();
        return FALSE;
    }

    USBREDIR_LOG_DEBUG("usbredir mgr get port %d success.", *port);
    USBREDIR_MGR_UNLOCK();
    return TRUE;
}

BOOL_T usbredir_mgr_set_filtrules(struct usbredirfilter_rule *rules, UINT32_T rule_count)
{
    USBREDIR_MGR_LOCK();

    if (FALSE == usbredir_om_set_filtrules(rules, rule_count))
    {
        USBREDIR_LOG_ERROR("usbredir mgr set filter rules fail.");
        USBREDIR_MGR_UNLOCK();
        return FALSE;
    }

    USBREDIR_LOG_DEBUG("usbredir mgr set filter rules success.");
    USBREDIR_MGR_UNLOCK();
    return TRUE;
}

BOOL_T usbredir_mgr_get_filtrules(struct usbredirfilter_rule **rules, UINT32_T *rule_count)
{
    USBREDIR_MGR_LOCK();

    if (FALSE == usbredir_om_get_filtrules(rules, rule_count))
    {
        USBREDIR_LOG_ERROR("usbredir mgr get filter rules fail.");
        USBREDIR_MGR_UNLOCK();
        return FALSE;
    }

    USBREDIR_LOG_DEBUG("usbredir mgr get filter rules success.");
    USBREDIR_MGR_UNLOCK();
    return TRUE;
}

