/* 
 * File: 
 *     usbredir_monitor.c
 *
 * Destription:
 *     hold on usbredir device monitor thread 
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
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "libusb.h"
#include "usbredirhost.h"
#include "usbredirfilter.h"
#include "usbredir_type.h"
#include "usbredir_om.h"
#include "usbredir_log.h"
#include "usbredir_event.h"

static BOOL_T monitor_running = FALSE;
static pthread_t monitor_thread;
static libusb_hotplug_callback_handle attach_handler = 0;
static libusb_hotplug_callback_handle detach_handler = 0;

static void *usbredir_monitor_main(void *opaque)
{
    USBREDIR_LOG_FATAL("usbredir monitor main, unsupportted now.");
    monitor_running = FALSE;
    return NULL;
}

int LIBUSB_CALL hotplug_attach_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *opaque)
{
    int ret;
    BOOL_T rc;

    BOOL_T service_enabled;
    BOOL_T service_available;

    UINT32_T rule_count;
    struct usbredirfilter_rule *rules;

    UINT16_T port;
    UINT32_T serverip;

    libusb_context *u_ctx;
    struct libusb_device_descriptor desc;

    if (NULL == ctx || NULL == dev)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback ctx or dev is null.");
        return 1;
    }

    ret = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != ret)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback libusb get device descriptor fail.");
        return 0;
    }

    USBREDIR_LOG_INFO("usbredir monitor, attachcallback attach device VendorId: %04x, ProductId: %04x",
                                                                desc.idVendor, desc.idProduct);

    usbredir_om_get_libusbctx((void **)&u_ctx);
    if (NULL == u_ctx){
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback get libusbctx NULL.");
        return 1;
    }

    if (u_ctx != ctx){
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback libusb ctx don't match.");
        return 1;
    }

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED != event)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback invalid event.");
        return 1;
    }

    usbredir_om_get_enabled(&service_enabled);
    if (FALSE == service_enabled)
    {
        USBREDIR_LOG_FATAL("usbredir monitor, attachcallback service disabled.");
        return 1;
    }

    usbredir_om_get_service_available(&service_available);
    if (FALSE == service_available)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback service unavailable.");
        return 0;
    }

    rc = usbredir_om_get_filtrules(&rules, &rule_count);
    if (FALSE == rc || 0 == rule_count || NULL == rules)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback get filtrules fail.");
        return 0;
    }

    if (LIBUSB_CLASS_HUB == desc.bDeviceClass)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback libusb get device descriptor fail.");
        return 0;
    }

    ret = usbredirhost_check_device_filter(rules, rule_count, dev, 0);
    if (0 != ret)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback check filterrule fail, device VendorId: %04x, ProductId: %04x", 
                                                                desc.idVendor, desc.idProduct);
        return 0;
    }

    usbredir_om_get_serverip(&serverip);
    if (0 == serverip)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback get serverip fail.");
        return 0;
    }

    rc = usbredir_om_get_serverport(&port);
    if (FALSE == rc || 0 == port)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback get port fail.");
        return 0;
    }

    ret = usbredir_event_create_thread(ctx, dev, serverip, port);
    if (ret < 0)
    {
        usbredir_om_set_serverport(port);
        USBREDIR_LOG_ERROR("usbredir monitor, attachcallback create event thread fail.");
        return 0;
    }

    USBREDIR_LOG_INFO("usbredir monitor, attachcallback attach device VendorId: %04x, ProductId: %04x success",
                                                                desc.idVendor, desc.idProduct);
    return 0;
}

int LIBUSB_CALL hotplug_detach_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *opaque)
{
    int ret;

    BOOL_T service_enabled;
    libusb_context *u_ctx;
    struct libusb_device_descriptor desc;

    if (NULL == ctx || NULL == dev)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback ctx or dev is null.");
        return 1;
    }

    ret = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != ret)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback libusb get device descriptor fail.");
        return 0;
    }

    USBREDIR_LOG_INFO("usbredir monitor, detachcallback detach device VendorId: %04x, ProductId: %04x",
                                                                desc.idVendor, desc.idProduct);

    usbredir_om_get_libusbctx((void **)&u_ctx);
    if (NULL == u_ctx){
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback get libusbctx NULL.");
        return 1;
    }

    if (u_ctx != ctx){
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback libusb ctx don't match.");
        return 1;
    }

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT != event)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback invalid event.");
        return 1;
    }

    usbredir_om_get_enabled(&service_enabled);
    if (FALSE == service_enabled)
    {
        USBREDIR_LOG_FATAL("usbredir monitor, detachcallback service disabled.");
        return 1;
    }

    ret = usbredir_event_destroy_thread(ctx, dev);
    if (ret < 0)
    {
        USBREDIR_LOG_ERROR("usbredir monitor, detachcallback destroy event thread fail.");
        return 0;
    }

    USBREDIR_LOG_INFO("usbredir monitor, detachcallback detach device VendorId: %04x, ProductId: %04x success",
                                                                desc.idVendor, desc.idProduct);
    return 0;
}

BOOL_T usbredir_monitor_create_thread()
{
    int ret;
    libusb_hotplug_flag flag;
    libusb_context *ctx = NULL;

    if (TRUE == monitor_running)
        return TRUE;

    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) == 0)   
    {
        ret = pthread_create(&monitor_thread, NULL, usbredir_monitor_main, NULL);
        if (ret)
        {
            monitor_running = FALSE;
            return FALSE;
        }

        USBREDIR_LOG_DEBUG("create usbredir monitor thread success.");
    } 
    else
    {
        usbredir_om_get_libusbctx((void **)&ctx);
        if (NULL == ctx){
            USBREDIR_LOG_ERROR("usbredir monitor create thread, get libusbctx NULL.");
            monitor_running = FALSE;
            return FALSE;
        }

        memset(&flag, 0, sizeof(flag));
#ifndef ENABLE_ENUMERATE_DEVICE
        flag = LIBUSB_HOTPLUG_ENUMERATE;
#endif
        ret = libusb_hotplug_register_callback(ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, flag,
                     LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                     hotplug_attach_callback, ctx, &attach_handler);
        if (LIBUSB_SUCCESS != ret){
            USBREDIR_LOG_ERROR("libusb register hotplug attach callback fail");
            monitor_running = FALSE;
            return FALSE;
        }  

        ret = libusb_hotplug_register_callback(ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, flag,
                     LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                     hotplug_detach_callback, ctx, &detach_handler);
        if (LIBUSB_SUCCESS != ret){
            USBREDIR_LOG_ERROR("libusb register hotplug detach callback fail");
            monitor_running = FALSE;
            return FALSE;
        }  

        USBREDIR_LOG_DEBUG("register libusb hotplug success.");
    }

    monitor_running = TRUE;
    
    return TRUE;
}

void usbredir_monitor_destroy_thread()
{
    libusb_context *ctx = NULL;

    if (FALSE == monitor_running)
        return;

    usbredir_om_get_libusbctx((void **)&ctx);
    if (NULL == ctx){
        USBREDIR_LOG_ERROR("usbredir monitor destroy thread, get libusbctx NULL.");
        return;
    }
        
    if (attach_handler && detach_handler && ctx){
        libusb_hotplug_deregister_callback(ctx, attach_handler);   
        attach_handler = 0;

        libusb_hotplug_deregister_callback(ctx, detach_handler);   
        detach_handler = 0;
        
        USBREDIR_LOG_DEBUG("deregister libusb hotplug success.");
    }
    else{
        pthread_join(monitor_thread, NULL);
        USBREDIR_LOG_DEBUG("destroy usbredir monitor thread success.");
    }

    monitor_running = FALSE;
}

#ifdef ENABLE_ENUMERATE_DEVICE
void usbredir_monitor_enumerate_device()
{
    int i, len;
    libusb_context *ctx = NULL;
    struct libusb_device **devs;

    usbredir_om_get_libusbctx((void **)&ctx);
    if (NULL == ctx){
        USBREDIR_LOG_ERROR("usbredir monitor enumerate device, get libusbctx NULL.");
        return;
    }

    len = (int) libusb_get_device_list(ctx, &devs);
    if (len < 0){
        USBREDIR_LOG_ERROR("usbredir monitor enumerate device, get device list fail.");
        return;
    }
  
    for(i=0; i<len; i++){
        hotplug_attach_callback(ctx, devs[i], LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, NULL);
    }

    libusb_free_device_list(devs, 1);
}
#endif

