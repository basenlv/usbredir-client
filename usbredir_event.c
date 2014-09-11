/* 
 * File: 
 *     usbredir_event.c
 *
 * Destription:
 *     hold on usbredir event thread API
 *
 * Revision history:
 *     Reason     Date        Owner
 *     Create     2018-8-25   Guangjun.lv
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <libusb.h>

#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "usbredirhost.h"
#include "usbredirfilter.h"
#include "usbredir_type.h"
#include "usbredir_om.h"
#include "usbredir_log.h"
#include "usbredir_monitor.h"

#define USBREDIR_EVENT_NUM 8
#define USBREDIR_CLIENT_VERSION "usbredir_client"

enum{
    NOT_RUNNING=0, 
    RUNNING=1,
};

typedef struct USBREDIR_EVENT_CTLBLK_S 
{
    pthread_t pthread_id;                /* thread id */
    UINT16_T port;                       /* service port */
    UINT32_T serverip;                   /* server ip address */
    UINT32_T vendor_id;                  /* device vendor id */
    UINT32_T product_id;                 /* device product id */
    int socket_fd;                       /* socket fd */
    struct usbredirhost *host;           /* usbredir host */
    BOOL_T running_status;               /* thread running status */
    BOOL_T used;                         /* used or not */
    pthread_mutex_t mutex;               /* mutext lock */
}USBREDIR_EVENT_CTLBLK_T;


static int verbose = usbredirparser_warning;
USBREDIR_EVENT_CTLBLK_T usbredir_event_ctrlblk[USBREDIR_EVENT_NUM];

#define USBREDIR_EVENT_LOCK(mutex)        \
            pthread_mutex_lock(mutex)
#define USBREDIR_EVENT_UNLOCK(mutex)      \
            pthread_mutex_unlock(mutex)

static void usbredir_event_release_ctrlblk(USBREDIR_EVENT_CTLBLK_T *event_ctrlblk);

BOOL_T usbredir_event_init()
{
    int i;
    int ret;

    memset((void *)usbredir_event_ctrlblk, 0, sizeof(usbredir_event_ctrlblk));

    for (i=0; i<USBREDIR_EVENT_NUM; i++)
    {
        ret = pthread_mutex_init(&usbredir_event_ctrlblk[i].mutex, NULL);
        if (ret != 0)
	{ 
            USBREDIR_LOG_DEBUG("usbredir event mutex init fail.");
	    return FALSE;
	}
    }

    USBREDIR_LOG_INFO("usbredir event initialized success.");
    return TRUE;
}

void usbredir_event_destroy()
{
    int i;

    for (i=0; i<USBREDIR_EVENT_NUM; i++)
    {
        if (TRUE == usbredir_event_ctrlblk[i].used && RUNNING == usbredir_event_ctrlblk[i].running_status)
        {
            USBREDIR_EVENT_LOCK(&usbredir_event_ctrlblk[i].mutex);

            usbredir_event_ctrlblk[i].running_status = NOT_RUNNING;

            /* ctrlblk is release while exit event main loop
            usbredir_event_release_ctrlblk(&usbredir_event_ctrlblk[i]);
            */
            USBREDIR_EVENT_UNLOCK(&usbredir_event_ctrlblk[i].mutex);
        }
    }

    /* exit until all thread terminate */
    while(1){
        for (i=0; i<USBREDIR_EVENT_NUM; i++)
        {
            if (TRUE == usbredir_event_ctrlblk[i].used)
	    {
               usleep(1000);
               break;
	    }
        }
  
        if (USBREDIR_EVENT_NUM == i)
            break;
    }

    for (i=0; i<USBREDIR_EVENT_NUM; i++)
    {
        pthread_mutex_destroy(&usbredir_event_ctrlblk[i].mutex);
    }

    /* ctrlblk is reset in release procedure
    memset((void *)usbredir_event_ctrlblk, 0, sizeof(usbredir_event_ctrlblk));
    */
}

static BOOL_T usbredir_event_allocate_ctrlblk(USBREDIR_EVENT_CTLBLK_T **event_ctrlblk)
{
    int i;

    if (NULL == event_ctrlblk)
        return FALSE;

    for (i=0; i<USBREDIR_EVENT_NUM; i++)
    {
        if (FALSE == usbredir_event_ctrlblk[i].used)
        {
            usbredir_event_ctrlblk[i].used = TRUE;
            *event_ctrlblk = &usbredir_event_ctrlblk[i];
            USBREDIR_LOG_DEBUG("usbredir event allocate ctrlblk.");
            return TRUE;
        }
    }
    
    *event_ctrlblk = NULL;
    return FALSE; 
}

static void usbredir_event_release_ctrlblk(USBREDIR_EVENT_CTLBLK_T *event_ctrlblk)
{
    if (RUNNING == event_ctrlblk->running_status){
        event_ctrlblk->running_status = NOT_RUNNING;
        pthread_join(event_ctrlblk->pthread_id, NULL);
        USBREDIR_LOG_DEBUG("usbredir event release ctrlblk, pthread join.");
    }

    if (event_ctrlblk->host){
        /* todo: how to handle device disconnect before close host */
        usbredirhost_close(event_ctrlblk->host);
        event_ctrlblk->host = NULL;
        USBREDIR_LOG_DEBUG("usbredir event release ctrlblk, usbredirhost close.");
    }

    if (event_ctrlblk->socket_fd > 0){
        close(event_ctrlblk->socket_fd);
        event_ctrlblk->socket_fd = -1;
        USBREDIR_LOG_DEBUG("usbredir event release ctrlblk, socket close.");
    }

    if (event_ctrlblk->port > 0){
        usbredir_om_set_serverport(event_ctrlblk->port);
        USBREDIR_LOG_DEBUG("usbredir event release ctrlblk, set server port %d.", event_ctrlblk->port);
    }

    memset(event_ctrlblk, 0, sizeof(USBREDIR_EVENT_CTLBLK_T));
}

static void usbredir_event_log(void *priv, int level, const char *msg)
{
    switch(level)
    {
        case usbredirparser_error:
            USBREDIR_LOG_ERROR("%s", msg);
            break;
        case usbredirparser_warning:
            USBREDIR_LOG_WARN("%s", msg);
            break;
        case usbredirparser_debug:
            USBREDIR_LOG_DEBUG("%s", msg);
            break;
        case usbredirparser_info:
            USBREDIR_LOG_INFO("%s", msg);
            break;
    }
}

static void *usbredir_event_alloc_lock(void)
{
    pthread_mutex_t *mutex;

    mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);

    return mutex;
}

static void usbredir_event_free_lock(void *user_data)
{
    pthread_mutex_t *mutex;

    mutex = (pthread_mutex_t *) user_data;

    pthread_mutex_destroy(mutex);

    free(mutex);
}

static void usbredir_event_lock_lock(void *user_data)
{
    pthread_mutex_t *mutex;

    mutex = (pthread_mutex_t *) user_data;

    pthread_mutex_lock(mutex);
}

static void usbredir_event_unlock_lock(void *user_data)
{
    pthread_mutex_t *mutex;

    mutex = (pthread_mutex_t *) user_data;

    pthread_mutex_unlock(mutex);
}


static int usbredir_event_read(void *priv, uint8_t *data, int count)
{
    USBREDIR_EVENT_CTLBLK_T *event_ctrlblk;

    if (NULL == priv || NULL == data){
        USBREDIR_LOG_ERROR("usbredir event read invalid input.");
        return -1;
    }

    event_ctrlblk = (USBREDIR_EVENT_CTLBLK_T *)priv;
    int r = read(event_ctrlblk->socket_fd, data, count);
    if (r < 0) {
        if (errno == EAGAIN)
            return 0;

        USBREDIR_LOG_ERROR("usbredir event read err: %d", errno);
        return -1;
    }

    if (r == 0) { /* server disconnected */
        USBREDIR_LOG_INFO("usbredir event read, server disconnect.");
        return -1;
    }

    return r;
}

static int usbredir_event_write(void *priv, uint8_t *data, int count)
{
    USBREDIR_EVENT_CTLBLK_T *event_ctrlblk;

    if (NULL == priv || NULL == data){
        USBREDIR_LOG_ERROR("usbredir event write invalid input.");
        return -1;
    }

    event_ctrlblk = (USBREDIR_EVENT_CTLBLK_T *)priv;
    int r = write(event_ctrlblk->socket_fd, data, count);
    if (r < 0) {
        if (errno == EAGAIN)
            return 0;
        if (errno == EPIPE) { /* server disconnected */
            USBREDIR_LOG_INFO("usbredir event write, server disconnect.");
            return -1;
        }

        USBREDIR_LOG_ERROR("usbredir event write err: %d", errno);
        return -1;
    }

    return r;
}

void *usbredir_event_channel(void *opaque)
{
    int i;
    const struct libusb_pollfd **pollfds = NULL;
    fd_set readfds, writefds;
    int n, nfds, client_fd;
    struct timeval timeout, *timeout_p;

    volatile BOOL_T used, running;

    struct usbredirhost *host;
    libusb_context *ctx = NULL;
    USBREDIR_EVENT_CTLBLK_T *event_ctrlblk;

    if (NULL == opaque)
    {
        USBREDIR_LOG_FATAL("usbredir event channel, invalid input.");
        return NULL;
    }

    usbredir_om_get_libusbctx((void *)&ctx);
    if (NULL == ctx)
    {
        USBREDIR_LOG_FATAL("usbredir event channel, invalid libusb context.");
        return NULL;
    }

    event_ctrlblk = (USBREDIR_EVENT_CTLBLK_T *)opaque;


    while(TRUE)
    {
        USBREDIR_EVENT_LOCK(&event_ctrlblk->mutex);
        used = event_ctrlblk->used;
        running = event_ctrlblk->running_status;
        host = event_ctrlblk->host;
        client_fd = event_ctrlblk->socket_fd;
        USBREDIR_EVENT_UNLOCK(&event_ctrlblk->mutex);

        if (TRUE != used || RUNNING != running){
            USBREDIR_LOG_INFO("usbredir event channel, not running, exit main loop.");
            break;
        }
        else
        {
            if (client_fd <= 0 || NULL == host){
                USBREDIR_LOG_ERROR("usbredir event channel, invalid socket fd, or invalid usbredir host.");
                break;
            }
            else{
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);

                FD_SET(client_fd, &readfds);
                if (usbredirhost_has_data_to_write(host)){
                    FD_SET(client_fd, &writefds);
                }
                nfds = client_fd + 1;

                free(pollfds); 
                pollfds = libusb_get_pollfds(ctx);
                for(i=0; pollfds && pollfds[i]; i++){
                    if (pollfds[i]->events && POLLIN)
                        FD_SET(pollfds[i]->fd, &readfds);
                    if (pollfds[i]->events && POLLOUT)
                        FD_SET(pollfds[i]->fd, &writefds);
                    if (pollfds[i]->fd >= nfds)
                        nfds = pollfds[i]->fd + 1;
                }
         
                /* ???, why use libusb_get_next_timeout */
                if (libusb_get_next_timeout(ctx, &timeout) == 1)
                    timeout_p = &timeout;
                else
                    timeout_p = NULL;

                n = select(nfds, &readfds, &writefds, NULL, timeout_p);
                if (n == -1){
                    if (errno == EINTR)
                        continue;
                    else{
                        USBREDIR_LOG_ERROR("usbredir event channel, socket select fail.");
                        break;
                    }
                }

                memset(&timeout, 0, sizeof(timeout));
                if (n == 0){
                    libusb_handle_events_timeout(ctx, &timeout);
                    continue;
                }

                if (FD_ISSET(client_fd, &readfds)){
                    if (usbredirhost_read_guest_data(host)){
                        USBREDIR_LOG_DEBUG("usbredir event channel, read guest data exit.");
                        break;
                    }
                }

                if (FD_ISSET(client_fd, &writefds)){
                    if (usbredirhost_write_guest_data(host)){
                        USBREDIR_LOG_DEBUG("usbredir event channel, write guest data exit.");
                        break;
                    }
                }

                memset(&timeout, 0, sizeof(timeout));
                for(i=0; pollfds && pollfds[i]; i++){
                    if (FD_ISSET(pollfds[i]->fd, &readfds) ||
                        FD_ISSET(pollfds[i]->fd, &writefds) ){
                        libusb_handle_events_timeout(ctx, &timeout);
                        USBREDIR_LOG_DEBUG("usbredir event channel, handle event time exit.");
                        break;
                    }
                }
            }
        }
    } 

    USBREDIR_EVENT_LOCK(&event_ctrlblk->mutex);
    event_ctrlblk->running_status = NOT_RUNNING;
    USBREDIR_LOG_INFO("usbredir event channel, terminate, release ctrblk.");
    usbredir_event_release_ctrlblk(event_ctrlblk);
    USBREDIR_EVENT_UNLOCK(&event_ctrlblk->mutex);

    return NULL;
}

int usbredir_event_create_thread(libusb_context *ctx, libusb_device *dev, UINT32_T serverip, UINT16_T port)
{
    int fd;
    int ret;
    int flags;
    int log_level;
    pthread_t pid;

    struct sockaddr_in seraddr;
    libusb_device_handle *handle = NULL;
    struct libusb_device_descriptor desc;
    struct usbredirhost *host = NULL;
    USBREDIR_EVENT_CTLBLK_T *event_ctrlblk;

    if (NULL == ctx || NULL == dev || 0 == serverip || 0 == port)
    {
        USBREDIR_LOG_FATAL("usbredir event, create thread invalid input.");
        return -1;
    }

    ret = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != ret) 
    {
        USBREDIR_LOG_ERROR("usbredir event create thread, get device descriptor fail.");
        return -1;
    }

    memset(&seraddr, 0, sizeof(seraddr));
    //seraddr.sin_len = sizeof(seraddr);
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(port);
    seraddr.sin_addr.s_addr = serverip;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd <= 0)
    {
        USBREDIR_LOG_ERROR("usbredir event create thread, create socket fail.");
        return -1;
    }
   
    if (connect(fd, (struct sockaddr *)&seraddr, sizeof(seraddr)) < 0) 
    {
        USBREDIR_LOG_ERROR("usbredir event create thread, connect to server %s, port %d fail.", inet_ntoa(seraddr.sin_addr), port);
        close(fd);
        return -1;
    }else{
        USBREDIR_LOG_INFO("usbredir event create thread, connect to server %s, port %d success.", inet_ntoa(seraddr.sin_addr), port);
    }

    flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        USBREDIR_LOG_ERROR("usbredir event create thread, fcntl get fail.");
        close(fd);
        return -1;
    }

    flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (flags == -1) {
        USBREDIR_LOG_ERROR("usbredir event create thread, fcntl set fail.");
        close(fd);
        return -1;
    }

    if (FALSE == usbredir_event_allocate_ctrlblk(&event_ctrlblk)){
        USBREDIR_LOG_ERROR("usbredir event create thread, allocate ctrlblk fail.");
        close(fd);
        return -1;
    }

    if (libusb_open(dev, &handle)){
        USBREDIR_LOG_ERROR("usbredir event create thread, libusb open handler fail.");
        usbredir_event_release_ctrlblk(event_ctrlblk);
        close(fd);
        return -1;
    } 

    USBREDIR_EVENT_LOCK(&event_ctrlblk->mutex);

    event_ctrlblk->socket_fd = fd;
    event_ctrlblk->serverip = serverip;
    event_ctrlblk->port = port;
    event_ctrlblk->vendor_id = desc.idVendor;
    event_ctrlblk->product_id = desc.idProduct;

    usbredir_log_getlevel(&log_level);
    if (LOG_DEBUG == log_level)
        verbose = usbredirparser_debug;

    host = usbredirhost_open_full(ctx, handle, usbredir_event_log,
                                usbredir_event_read, usbredir_event_write, NULL,
                                usbredir_event_alloc_lock, usbredir_event_lock_lock,
                                usbredir_event_unlock_lock, usbredir_event_free_lock,
                                event_ctrlblk, USBREDIR_CLIENT_VERSION, verbose, 0, -1, 0);
    if (NULL == host){
        USBREDIR_LOG_ERROR("usbredir event create thread, usbredirhost open fail.");
        usbredir_event_release_ctrlblk(event_ctrlblk);
        USBREDIR_EVENT_UNLOCK(&event_ctrlblk->mutex);
        return -1;
    }
    else{
        event_ctrlblk->host = host;
    }

    ret = pthread_create(&pid, NULL, usbredir_event_channel, (void *)event_ctrlblk);
    if (ret){
        USBREDIR_LOG_ERROR("usbredir event create thread, usbredirhost create event main thread fail.");
        usbredir_event_release_ctrlblk(event_ctrlblk);
        USBREDIR_EVENT_UNLOCK(&event_ctrlblk->mutex);
        return -1;
    }
    else{
        event_ctrlblk->pthread_id = pid;
        event_ctrlblk->running_status = RUNNING;
    }

    USBREDIR_EVENT_UNLOCK(&event_ctrlblk->mutex);
    return 0;
}

int usbredir_event_destroy_thread(libusb_context *ctx, libusb_device *dev)
{
    int i;
    int ret;

    struct libusb_device_descriptor desc;

    if (NULL == ctx || NULL == dev)
    {
        USBREDIR_LOG_FATAL("usbredir event destroy thread, destroy thread invalid input.");
        return -1;
    }

    ret = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != ret) 
    {
        USBREDIR_LOG_ERROR("usbredir event destroy thread, get device descriptor fail.");
        return -1;
    }

    for (i=0; i<USBREDIR_EVENT_NUM; i++)
    {
        if (TRUE == usbredir_event_ctrlblk[i].used && RUNNING == usbredir_event_ctrlblk[i].running_status)
        {
            if (desc.idVendor == usbredir_event_ctrlblk[i].vendor_id && 
                desc.idProduct == usbredir_event_ctrlblk[i].product_id){

                USBREDIR_EVENT_LOCK(&usbredir_event_ctrlblk[i].mutex);
                usbredir_event_ctrlblk[i].running_status = NOT_RUNNING;
                USBREDIR_EVENT_UNLOCK(&usbredir_event_ctrlblk[i].mutex);
                return 0;
            }
        }
    }

    USBREDIR_LOG_ERROR("usbredir event destroy thread, no matched event ctlblk, device vendor_id 0x%4x, product_id 0x%4x", 
                                 desc.idVendor, desc.idProduct);
    return -1;
}

#ifdef ENABLE_POLLING_THREAD
const char *usbredir_event_libusb_strerror(int err_code)
{
    switch (err_code) {
    case LIBUSB_SUCCESS:
        return "Success";
    case LIBUSB_ERROR_IO:
        return "Input/output error";
    case LIBUSB_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case LIBUSB_ERROR_ACCESS:
        return "Access denied (insufficient permissions)";
    case LIBUSB_ERROR_NO_DEVICE:
        return "No such device (it may have been disconnected)";
    case LIBUSB_ERROR_NOT_FOUND:
        return "Entity not found";
    case LIBUSB_ERROR_BUSY:
        return "Resource busy";
    case LIBUSB_ERROR_TIMEOUT:
        return "Operation timed out";
    case LIBUSB_ERROR_OVERFLOW:
        return "Overflow";
    case LIBUSB_ERROR_PIPE:
        return "Pipe error";
    case LIBUSB_ERROR_INTERRUPTED:
        return "System call interrupted (perhaps due to signal)";
    case LIBUSB_ERROR_NO_MEM:
        return "Insufficient memory";
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return "Operation not supported or unimplemented on this platform";
    case LIBUSB_ERROR_OTHER:
        return "Other error";
    }
    return "Unknown error";
}

void *usbredir_event_poll(void *opaque)
{
    int rc;
    volatile BOOL_T running;

    libusb_context *ctx = NULL;

    if (NULL == opaque)
    {
        USBREDIR_LOG_FATAL("usbredir event, polling thread, invalid input.");
        return NULL;
    }

    ctx = (libusb_context *)opaque;

    while(1)
    {
        running = polling_status;
        if (running){
            rc = libusb_handle_events(ctx);
            if (rc && rc != LIBUSB_ERROR_INTERRUPTED){
                const char *desc = usbredir_event_libusb_strerror(rc);
                USBREDIR_LOG_ERROR("usbredir event, polling thread, error libusb handle events: %s.", desc);
            }
        }else{
            break;
        }
    }

    USBREDIR_LOG_INFO("usbredir event poll, terminate.");
    return NULL;
}

void usbredir_event_start_polling()
{
    int ret;
    pthread_t pid;
    libusb_context *ctx = NULL;

    polling_listeners++;

    usbredir_om_get_libusbctx((void *)&ctx);
    if (NULL == ctx)
    {
        USBREDIR_LOG_FATAL("usbredir event, start polling, invalid libusb context.");
        return;
    }

    if (TRUE == polling_status){
        USBREDIR_LOG_DEBUG("usbredir event, polling has been in running state.");
        return;
    }
   
    ret = pthread_create(&pid, NULL, usbredir_event_poll, (void *)ctx);
    if (ret){
        polling_status = FALSE;
        USBREDIR_LOG_FATAL("usbredir event, create polling thread fail.");
        return;
    }else{
        polling_pid = pid;
        polling_status = TRUE;
        USBREDIR_LOG_INFO("usbredir event, create polling thread success.");
    }

    return; 
}

void usbredir_event_stop_polling()
{
    polling_listeners--;

    if (0 == polling_listeners){
        polling_status = FALSE;
        USBREDIR_LOG_INFO("usbredir event, stop polling thread.");
    }

}
#endif

